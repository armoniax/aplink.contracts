#include <eosio.token/eosio.token.hpp>
#include <otcbook/mgp_math.hpp>
#include <otcbook/otcbook.hpp>
#include <otcbook/utils.hpp>


using namespace eosio;
using namespace std;
using std::string;

namespace mgp {

using namespace std;
using namespace eosio;
using namespace wasm::safemath;


inline asset otcbook::_calc_order_stakes(const asset &quantity, const asset &price) {
    // calc order quantity value by price
    auto value = multiply_decimal64( quantity.amount, price.amount, calc_precision(price.symbol.precision()) );
    // TODO: if (price.symbol != CNY) { convert to cny value }
    CHECK(price.symbol == CNY, "Only support CNY");
    
    // adjust precision
    if (quantity.symbol.precision() != STAKE_SYMBOL.precision()) {
       value = multiply_decimal64(value, STAKE_SYMBOL.precision(), quantity.symbol.precision());
    }

    int64_t amount = divide_decimal64(value, order_stake_pct, percent_boost);
    
    return asset(amount, STAKE_SYMBOL);
}

void otcbook::_init() {
    _gstate.transaction_fee_receiver 		= "devshare"_n;
    // _gstate.min_buy_order_quantity.amount 	= 10'0000;
    // _gstate.min_sell_order_quantity.amount 	= 10'0000;
    // _gstate.min_pos_stake_quantity.amount 	= 2000'0000; //close to $200
    _gstate.withhold_expire_sec 			= 900;
    // _gstate.pos_staking_contract 			= "addressbookt"_n;
    // _gstate.cs_contact_title				= "Custom Service Contact";
    // _gstate.cs_contact						= "cs_contact";

    // _gstate2.admin = "mwalletadmin"_n;
}


void otcbook::init() {
    // _global.remove();
    
    check( false, "init completed!" );

    //_init();

}

void otcbook::setadmin(const name& admin) {
    require_auth( _self );
    _gstate.admin = admin;
}

void otcbook::setmerchant(const name& owner, const set<uint8_t> &pay_methods, const string& email, const string& memo) {
    require_auth( owner );

    check(email.size() < 64, "email size too large: " + to_string(email.size()) );
    check(memo.size() < max_memo_size, "memo size too large: " + to_string(memo.size()) );

    for (auto& method : pay_methods) {
        check( (pay_type_t) method < pay_type_t::PAYMAX, "pay method illegal: " + to_string(method) );
        check( (pay_type_t) method > pay_type_t::PAYMIN, "pay method illegal: " + to_string(method) );
    }

    merchant_t merchant(owner);
    if (!_dbc.get(merchant)) {
        // first register, init
        merchant.status = (uint8_t)merchant_status_t::REGISTERED;
    }

    merchant.email = email;
    merchant.memo = memo;
    merchant.accepted_payments = pay_methods;

    _dbc.set( merchant );

}

void otcbook::enablemer(const name& owner, bool is_enabled) {
    merchant_t merchant(owner);
    check( _dbc.get(merchant), "merchant not found: " + owner.to_string() );
    if (is_enabled) {
        check((merchant_status_t)merchant.status != merchant_status_t::ENABLED,
            "merchant has been enabled");
        merchant.status = (uint8_t)merchant_status_t::ENABLED;
    } else {
        check((merchant_status_t)merchant.status != merchant_status_t::DISABLED,
            "merchant has been disabled");
        merchant.status = (uint8_t)merchant_status_t::DISABLED;
    }
    _dbc.set( merchant );
}

/**
 * only merchant allowed to open orders
 */
void otcbook::openorder(const name& owner, uint8_t side, const asset& quantity, const asset& price, 
    const asset& min_accept_quantity, const string &memo
){
    require_auth( owner );
    
    // check( _gstate.usd_exchange_rate > asset(0, USD_SYMBOL), "The exchange rate is incorrect");
    check( (order_side_t)side == order_side_t::BUY || (order_side_t)side == order_side_t::SELL, "Invalid order side" );
    check( quantity.is_valid(), "Invalid quantity");
    check( quantity.is_valid(), "Invalid price");
    if ((order_side_t)side == order_side_t::BUY) {
        auto itr = _gstate.coin_to_fiat_list.find(quantity.symbol);
        check(itr != _gstate.coin_to_fiat_list.end(), "quantity symbol not allowed for buying");
        check(itr->second.count(price.symbol) > 0, "price symbol not allowed for buying");
    } else {
         auto itr = _gstate.fiat_to_coin_list.find(price.symbol);
        check(itr != _gstate.fiat_to_coin_list.end(), "price symbol not allowed for selling");
        check(itr->second.count(quantity.symbol) > 0, "quantity symbol not allowed for selling");       
    }

    check( quantity.amount > 0, "quantity must be positive");
    // TODO: min order quantity
    check( price.amount > 0, "price must be positive" );
    // TODO: price range
    check( min_accept_quantity.symbol == quantity.symbol, "min_accept_quantity Symbol mismatch with quantity" );
    check( min_accept_quantity.amount >= 0 && min_accept_quantity.amount <= quantity.amount, 
        "invalid min_accept_quantity amount" );

    merchant_t merchant(owner);
    check( _dbc.get(merchant), "merchant not found: " + owner.to_string() );
    check((merchant_status_t)merchant.status == merchant_status_t::ENABLED,
        "merchant not enabled");
    
    auto stake_quantity = _calc_order_stakes(quantity, price); // TODO: process 70% used-rate of stake
    check( merchant.stake_quantity >= stake_quantity, "merchant stake quantity insufficient, expected: " + stake_quantity.to_string() );
    merchant.stake_quantity -= stake_quantity;
    _dbc.set( merchant );

    // TODO: check pos_staking_contract
    // if (_gstate.min_pos_stake_quantity.amount > 0) {
    // 	auto staking_con = _gstate.pos_staking_contract;
    // 	balances bal(staking_con, staking_con.value);
    // 	auto itr = bal.find(owner.value);
    // 	check( itr != bal.end(), "POS staking not found for: " + owner.to_string() );
    // 	check( itr->remaining >= _gstate.min_pos_stake_quantity, "POS Staking requirement not met" );
    // }

    order_table_t orders(_self, _self.value);
    auto order_id = orders.available_primary_key();
    orders.emplace( _self, [&]( auto& row ) {
        row.id 					= order_id;
        row.owner 				= owner;
        row.side				= side;
        row.price				= price;
        // row.price_usd			= asset( price.amount * 10000 / _gstate2.usd_exchange_rate.amount , USD_SYMBOL);
        row.quantity			= quantity;
        row.stake_quantity      = stake_quantity;
        row.min_accept_quantity = min_accept_quantity;
        row.memo = memo;
        row.closed				= false;
        row.created_at			= time_point_sec(current_time_point());
        row.frozen_quantity = asset(0, quantity.symbol);
        row.fulfilled_quantity = asset(0, quantity.symbol);
        row.accepted_payments = merchant.accepted_payments;
    });
}

void otcbook::closeorder(const name& owner, const uint64_t& order_id) {
    require_auth( owner );

    merchant_t merchant(owner);
    check( _dbc.get(merchant), "merchant not found: " + owner.to_string() );

    order_table_t orders(_self, _self.value);
    auto itr = orders.find(order_id);
    check( itr != orders.end(), "sell order not found: " + to_string(order_id) );
    check( !itr->closed, "order already closed" );
    check( itr->frozen_quantity.amount == 0, "order being processed" );
    check( itr->quantity >= itr->fulfilled_quantity, "Err: insufficient quanitty" );

    // 撤单后币未交易完成的币退回
    merchant.stake_quantity += itr->stake_quantity;
    _dbc.set( merchant );

    orders.modify( *itr, _self, [&]( auto& row ) {
        row.closed = true;
        row.closed_at = time_point_sec(current_time_point());
    });

}

void otcbook::opendeal(const name& taker, const uint64_t& order_id, const asset& deal_quantity, const uint64_t& order_sn,
    const string& memo
) {
    require_auth( taker );

    // check( deal_quantity >= _gstate.min_buy_order_quantity, "min buy order quantity not met: " +  _gstate.min_buy_order_quantity.to_string() );

    order_table_t orders(_self, _self.value);
    auto itr = orders.find(order_id);
    check( itr != orders.end(), "Order not found: " + to_string(order_id) );
    check( itr->owner != taker, "taker can not be equal to maker");
    check( deal_quantity.symbol == itr->quantity.symbol, "Token Symbol mismatch" );
    check( !itr->closed, "Order already closed" );
    check( itr->quantity >= itr->frozen_quantity + itr->fulfilled_quantity + deal_quantity, 
        "Order's quantity insufficient" );
    // check( itr->price.amount * deal_quantity.amount >= itr->min_accept_quantity.amount * 10000, "Order's min accept quantity not met!" );
    check( deal_quantity >= itr->min_accept_quantity, "Order's min accept quantity not met!" );
    
    asset order_price = itr->price;
    // asset order_price_usd = itr->price_usd;
    name order_maker = itr->owner;

    deal_t::idx_t deals(_self, _self.value);
    auto ordersn_index 			= deals.get_index<"ordersn"_n>();
    auto lower_itr 				= ordersn_index.lower_bound(order_sn);
    auto upper_itr 				= ordersn_index.upper_bound(order_sn);

    check( ordersn_index.find(order_sn) == ordersn_index.end() , "order_sn already existing!" );

    auto created_at = time_point_sec(current_time_point());
    auto deal_id = deals.available_primary_key();
    deals.emplace( _self, [&]( auto& row ) {
        row.id 					= deal_id;
        row.order_id 			= order_id;
        row.order_price			= order_price;
        // row.order_price_usd		= order_price_usd;
        row.deal_quantity		= deal_quantity;
        row.order_maker			= order_maker;
        row.order_taker			= taker;
        row.closed				= false;
        row.status				= (uint8_t)deal_status_t::CREATED;
        row.created_at			= created_at;
        row.order_sn 			= order_sn;
        row.expired_at 			= time_point_sec(created_at.sec_since_epoch() + _gstate.withhold_expire_sec);
        // row.restart_taker_num 	= 0;
        // row.restart_maker_num 	= 0;
        row.memos.push_back({taker, (uint8_t)deal_status_t::NONE, (uint8_t)deal_action_t::CREATE, memo});
    });

    // 添加交易到期表数据
    deal_expiry_tbl deal_expiries(_self, _self.value);
    deal_expiries.emplace( _self, [&]( auto& row ){
        row.deal_id = deal_id;
        row.expired_at 			= time_point_sec(created_at.sec_since_epoch() + _gstate.withhold_expire_sec);
    });

    orders.modify( *itr, _self, [&]( auto& row ) {
        row.frozen_quantity 	+= deal_quantity;
    });
}

/**
 * actively close the deal by order taker
 */
void otcbook::closedeal(const name& account, const uint8_t& account_type, const uint64_t& deal_id, const string& memo) {
    require_auth( account );
    
    deal_t::idx_t deals(_self, _self.value);
    auto deal_itr = deals.find(deal_id);
    check( deal_itr != deals.end(), "deal not found: " + to_string(deal_id) );
    check( !deal_itr->closed, "deal already closed: " + to_string(deal_id) );

    switch ((account_type_t) account_type) {
    case account_type_t::MERCHANT: 
        check( deal_itr->order_maker == account, "maker account mismatched");
        break;
    case account_type_t::USER:
        check( deal_itr->order_taker == account, "taker account mismatched");
        break;
    case account_type_t::ADMIN:
        check( deal_itr->order_taker == account, "admin account mismatched");
        break;
    default:
        check(false, "account type not supported: " + to_string(account_type));
        break;
    }

    auto order_id = deal_itr->order_id;
    order_table_t orders(_self, _self.value);
    auto order_itr = orders.find(order_id);
    check( order_itr != orders.end(), "sell order not found: " + to_string(order_id) );
    check( !order_itr->closed, "order already closed" );

    auto action = deal_action_t::CLOSE;
    auto status = (deal_status_t)deal_itr->status;
    if ((account_type_t) account_type != account_type_t::ADMIN) {
        check(deal_status_t::CREATED == status || deal_status_t::TAKER_RECEIVED == status, 
            "can not process deal action:" + to_string((uint8_t)action) 
                + " at status: " + to_string((uint8_t)status) );
    }
    

    auto deal_quantity = deal_itr->deal_quantity;
    check( order_itr->frozen_quantity >= deal_quantity, "Err: order frozen quantity smaller than deal quantity" );

    orders.modify( *order_itr, _self, [&]( auto& row ) {
        row.frozen_quantity -= deal_quantity;
        row.fulfilled_quantity += deal_quantity;
    });

    deals.modify( *deal_itr, _self, [&]( auto& row ) {
        row.status = (uint8_t)deal_status_t::CLOSED;
        row.closed_at = time_point_sec(current_time_point());
        row.memos.push_back({account, (uint8_t)status, (uint8_t)action, memo});
    });

}

void otcbook::processdeal(const name& account, const uint8_t& account_type, const uint64_t& deal_id, 
    uint8_t action, const string& memo
) {
    require_auth( account );

    deal_t::idx_t deals(_self, _self.value);
    auto deal_itr = deals.find(deal_id);
    check( deal_itr != deals.end(), "deal not found: " + to_string(deal_id) );

    order_table_t orders(_self, _self.value);
    auto order_itr = orders.find(deal_itr->order_id);
    check( order_itr != orders.end(), "order not found: " + to_string(deal_itr->order_id) );
    // check( order_itr -> accepted_payments.count(pay_type) , "pay method illegal: " + to_string(pay_type) );

    auto now = time_point_sec(current_time_point());

    switch ((account_type_t) account_type) {
    case account_type_t::MERCHANT: 
        check( deal_itr->order_maker == account, "maker account mismatched");
        break;
    case account_type_t::USER:
        check( deal_itr->order_taker == account, "taker account mismatched");
        break;
    default:
        check(false, "account type not supported: " + to_string(account_type));
        break;
    }

    auto status = (deal_status_t)deal_itr->status;
    deal_status_t limited_status = deal_status_t::NONE;
    account_type_t limited_account_type = account_type_t::NONE;
    deal_status_t next_status = deal_status_t::NONE;
    check( status != deal_status_t::CLOSED, "deal already closed: " + to_string(deal_id) ); // TODO:...

#define DEAL_ACTION_CASE(_action, _limited_account_type, _limited_status, _next_status) \
    case deal_action_t::_action:                                                        \
        limited_account_type = account_type_t::_limited_account_type;                                   \
        limited_status = deal_status_t::_limited_status;                                \
        next_status = deal_status_t::_next_status;                                      \
        break;

    switch ((deal_action_t)action)
    {
    // /*               action      account_type  limited_status   next_status  */
    DEAL_ACTION_CASE(MAKER_ACCEPT,  MERCHANT,     CREATED,         MAKER_ACCEPTED)
    DEAL_ACTION_CASE(TAKER_SEND,    USER,         MAKER_ACCEPTED,  TAKER_SENT)
    DEAL_ACTION_CASE(MAKER_RECEIVE, MERCHANT,     TAKER_SENT,      MAKER_RECEIVED)
    DEAL_ACTION_CASE(MAKER_SEND,    MERCHANT,     MAKER_RECEIVED,  MAKER_SENT)
    DEAL_ACTION_CASE(TAKER_RECEIVE, USER,         MAKER_SENT,      TAKER_RECEIVED) 
    DEAL_ACTION_CASE(ADD_MEMO,      NONE,         NONE,            NONE) 
    default: 
        check(false, "unsupported process deal action:" + to_string((uint8_t)action));
        break;
    }

    if (limited_status != deal_status_t::NONE)
        check(limited_status == status, "can not process deal action:" + to_string((uint8_t)action) 
             + " at status: " + to_string((uint8_t)status) );
    if (limited_account_type != account_type_t::NONE)
        check(limited_account_type == (account_type_t)account_type, 
            "can not process deal action:" + to_string((uint8_t)action) 
             + " by account_type: " + to_string((uint8_t)account_type) );


    deals.modify( *deal_itr, _self, [&]( auto& row ) {
        if (next_status != deal_status_t::NONE) {
            row.status = (uint8_t)next_status;
        }
        row.memos.push_back({account, (uint8_t)status, action, memo});
    });

}

/**
 *
 * 款项异常，回退到：待付款状态
 *
 */
void otcbook::reversedeal(const name& arbiter,const uint64_t& deal_id){

    #ifdef __comment
    require_auth( arbiter );

    check( _gstate.otc_arbiters.count(arbiter), "not an arbiter: " + arbiter.to_string() );

    deal_t::idx_t deals(_self, _self.value);
    auto deal_itr = deals.find(deal_id);
    check( deal_itr != deals.end(), "deal not found: " + to_string(deal_id) );
    check( !deal_itr->closed, "deal already closed: " + to_string(deal_id) );
    check( deal_itr->taker_passed_at != time_point_sec(), "No operation required" );

    auto expired_at = time_point_sec(current_time_point().sec_since_epoch() + _gstate.withhold_expire_sec);
    deals.modify( *deal_itr, _self, [&]( auto& row ) {
        row.arbiter = arbiter;
        row.arbiter_passed = false;
        row.arbiter_passed_at = time_point_sec(current_time_point());;
        row.taker_passed = false;
        row.taker_passed_at = time_point_sec();
        row.pay_type = 0;
        row.maker_expired_at = time_point_sec();
        row.maker_passed = false;
        row.maker_passed_at = time_point_sec();
        row.expired_at = expired_at;
        row.restart_taker_num = 0;
        row.restart_maker_num = 0;
    });

    deal_expiry_tbl exp_time(_self,_self.value);
    auto exp_itr = exp_time.find(deal_id);

    if ( exp_itr != exp_time.end() ) {
        exp_time.modify( *exp_itr, _self, [&]( auto& row ) {
            row.expired_at = expired_at;
        });
    } else {
        exp_time.emplace( _self, [&]( auto& row ){
            row.deal_id = deal_id;
            row.expired_at = expired_at;
           });
    }
    #endif
}


/**
 *  提取
 *
 */
void otcbook::withdraw(const name& owner, asset quantity){
    require_auth( owner );

    check( quantity.amount > 0, "quanity must be positive" );
    check( quantity.symbol.is_valid(), "Invalid quantity symbol name" );
    check( quantity.symbol == STAKE_SYMBOL, "Token Symbol not allowed" );

    merchant_t merchant(owner);
    check( _dbc.get(merchant), "merchant not found: " + owner.to_string() );
    check((merchant_status_t)merchant.status == merchant_status_t::ENABLED,
    "merchant not enabled");
    check( merchant.stake_quantity >= quantity, "The withdrawl amount must be less than the balance" );
    merchant.stake_quantity -= quantity;
    _dbc.set(merchant);

    TRANSFER( SYS_BANK, owner, quantity, "withdraw" )

}

/**
 * 超时检测
 *
 */
void otcbook::timeoutdeal() {

    auto check_time = current_time_point().sec_since_epoch() - seconds_per_day;

    deal_expiry_tbl exp_time(_self,_self.value);
    auto exp_index = exp_time.get_index<"expiry"_n>();
    auto lower_itr = exp_index.find(check_time);
    // auto itr = exp_time.begin()
    bool processed = false;

    for (auto itr = exp_index.begin(); itr != lower_itr; ) {
        if (itr->expired_at <= time_point_sec(check_time)) {
            deal_t::idx_t deals(_self, _self.value);
            auto deal_itr = deals.find(itr -> deal_id);

             // 订单处于买家未操作状态进行关闭
            if (deal_itr != deals.end() && 
                ((deal_status_t)deal_itr->status == deal_status_t::CREATED 
                    ||  (deal_status_t)deal_itr->status == deal_status_t::MAKER_ACCEPTED) ) {

                auto order_id = deal_itr->order_id;
                order_table_t orders(_self, _self.value);
                auto order_itr = orders.find(order_id);
                check( order_itr != orders.end(), "sell order not found: " + to_string(order_id) );
                check( !order_itr->closed, "order already closed" );

                auto deal_quantity = deal_itr->deal_quantity;
                check( order_itr->frozen_quantity >= deal_quantity, "Err: order frozen quantity smaller than deal quantity" );

                orders.modify( *order_itr, _self, [&]( auto& row ) {
                    row.frozen_quantity -= deal_quantity;
                });

                deals.modify( *deal_itr, _self, [&]( auto& row ) {
                    row.closed = true;
                    row.closed_at = time_point_sec(current_time_point());
                });

                processed = true;
            }

            itr = exp_index.erase(itr);
        } else {
            itr ++;
        }
    }

}

/**
 *
 * 超时重启
 */
void otcbook::restart(const name& owner,const uint64_t& deal_id,const uint8_t& user_type){
    #ifdef __comment
    require_auth( owner );

    check( _gstate.otc_arbiters.count(owner), "not an arbiter: " + owner.to_string() );

    deal_t::idx_t deals(_self, _self.value);
    auto deal_itr = deals.find(deal_id);
    check( deal_itr != deals.end(), "deal not found: " + to_string(deal_id) );
    check( !deal_itr->closed, "deal already closed: " + to_string(deal_id) );

    order_table_t orders(_self, _self.value);
    auto order_itr = orders.find(deal_itr->order_id);
    check( order_itr != orders.end(), "order not found: " + to_string(deal_itr->order_id) );

    auto restart_time = time_point_sec(current_time_point().sec_since_epoch() + _gstate.withhold_expire_sec);
    auto check_time = time_point_sec(current_time_point().sec_since_epoch() - seconds_per_day);
    auto now = time_point_sec(current_time_point());

    switch ((account_type_t) user_type) {
        case MERCHANT:
        {
            check( deal_itr -> restart_maker_num == 0 , "It has been rebooted more than 1 time.");
            check( deal_itr -> maker_expired_at <= now ,"Did not time out.");
            check( deal_itr -> maker_expired_at > check_time ,"The order has timed out by 24 hours.");
            check( deal_itr -> maker_passed_at == time_point_sec() , "No operation required" );

            deals.modify( *deal_itr, _self, [&]( auto& row ) {
                row.restart_maker_num ++;
                row.maker_expired_at = restart_time;
            });

            break;
        }
        case USER:
        {

            check( deal_itr -> restart_taker_num == 0 , "It has been rebooted more than 1 time.");
            check( deal_itr -> expired_at <= now ,"Did not time out.");
            check( deal_itr -> expired_at > check_time ,"The order has timed out by 24 hours.");
            check( deal_itr -> taker_passed_at == time_point_sec() , "No operation required" );

            deals.modify( *deal_itr, _self, [&]( auto& row ) {
                row.restart_taker_num ++;
                row.expired_at = restart_time;
            });

            deal_expiry_tbl exp_time(_self,_self.value);
            auto exp_itr = exp_time.find(deal_id);
            check( exp_itr != exp_time.end() ,"the order has timed out");
            check( exp_itr -> expired_at > check_time,"The order has timed out by 24 hours.");

            exp_time.modify( *exp_itr, _self, [&]( auto& row ) {
                row.expired_at = restart_time;
            });

            break;
        }
        default:
            break;
    }
    #endif
}

/**
 * 更新汇率及mgp价格
 */ 
void otcbook::setrate(const name& owner, const asset& mgp_price, const asset& usd_exchange_rate){
    // require_auth( owner );
    
    // check( owner == _gstate.admin || owner == _self, "None-admin access denied" );
    // check( mgp_price.symbol == USD_SYMBOL , "MGP price is must be in USD" );
    // check( usd_exchange_rate.symbol == CNY_SYMBOL , "The exchange rate is CNY" );

    // _gstate2.mgp_price = mgp_price;
    // _gstate2.usd_exchange_rate = usd_exchange_rate;
}

/*************** Begin of eosio.token transfer trigger function ******************/
/**
 * This happens when a merchant decides to open sell orders
 */
void otcbook::deposit(name from, name to, asset quantity, string memo) {
    if (to != _self) return;

    if (get_first_receiver() == SYS_BANK && quantity.symbol == CNYD_SYMBOL){
        merchant_t merchant(from);
        if (_dbc.get( merchant )) {
            check((merchant_status_t)merchant.status == merchant_status_t::ENABLED,
                "merchant not enabled");
            merchant.stake_quantity += quantity;
            _dbc.set( merchant );
        }
    }
}

/**
 * 进行数据清除，测试用，发布正式环境去除
 */
void otcbook::deltable(){
    require_auth( _self );

    order_table_t sellorders(_self,_self.value);
    auto itr = sellorders.begin();
    while(itr != sellorders.end()){
        itr = sellorders.erase(itr);
    }

    deal_t::idx_t deals(_self,_self.value);
    auto itr1 = deals.begin();
    while(itr1 != deals.end()){
        itr1 = deals.erase(itr1);
    }

    merchant_t::idx_t merchants(_self,_self.value);
    auto itr2 = merchants.begin();
    while(itr2 != merchants.end()){
        itr2 = merchants.erase(itr2);
    }

    deal_expiry_tbl exp(_self,_self.value);
    auto itr3 = exp.begin();
    while(itr3 != exp.end()){
        itr3 = exp.erase(itr3);
    }



}


}  //end of namespace:: mgpbpvoting
