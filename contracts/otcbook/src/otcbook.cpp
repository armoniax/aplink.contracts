#include <eosio.token/eosio.token.hpp>
#include <otcbook/amax_math.hpp>
#include <otcbook/otcbook.hpp>
#include <otcconf/utils.hpp>
#include <otcsettle.hpp>
#include <otcswap.hpp>
#include "aplink.farm/aplink.farm.hpp"

static constexpr eosio::name active_permission{"active"_n};

#define STAKE_CHANGED(account, quantity, memo) \
    {	metabalance::otcbook::stakechanged_action act{ _self, { {_self, active_permission} } };\
			act.send( account, quantity , memo );}


#define NOTIFICATION(account, info, memo) \
    {	metabalance::otcbook::notification_action act{ _self, { {_self, active_permission} } };\
			act.send( account, info , memo );}


#define ALLOT(bank, land_id, customer, quantity, memo) \
    {	aplink::farm::allot_action act{ bank, { {_self, active_perm} } };\
			act.send( land_id, customer, quantity , memo );}

using namespace metabalance;
using namespace std;
using namespace eosio;
using namespace wasm::safemath;
using namespace otc;

inline int64_t get_precision(const symbol &s) {
    int64_t digit = s.precision();
    CHECK(digit >= 0 && digit <= 18, "precision digit " + std::to_string(digit) + " should be in range[0,18]");
    return calc_precision(digit);
}

inline int64_t get_precision(const asset &a) {
    return get_precision(a.symbol);
}

asset otcbook::_calc_order_stakes(const asset &quantity) {
    // calc order quantity value by price
    auto stake_symbol = _conf().coin_as_stake.at(quantity.symbol);
    auto value = multiply_decimal64( quantity.amount, get_precision(stake_symbol), get_precision(quantity) );
    int64_t amount = divide_decimal64(value, order_stake_pct, percent_boost);
    return asset(amount, stake_symbol);
}

asset otcbook::_calc_deal_amount(const asset &quantity) {
    auto stake_symbol = _conf().coin_as_stake.at(quantity.symbol);
    auto value = multiply_decimal64( quantity.amount, get_precision(stake_symbol), get_precision(quantity) );
    return asset(value, stake_symbol);
}

asset otcbook::_calc_deal_fee(const asset &quantity) {
    // calc order quantity value by price
    auto stake_symbol = _conf().coin_as_stake.at(quantity.symbol);
    auto value = multiply_decimal64( quantity.amount, get_precision(stake_symbol), get_precision(quantity) );
    const auto & fee_pct = _conf().fee_pct;
    int64_t amount = multiply_decimal64(value, fee_pct, percent_boost);
    amount = multiply_decimal64(amount, get_precision(stake_symbol), get_precision(quantity));
    return asset(amount, stake_symbol);
}

void otcbook::setconf(const name &conf_contract) {
    require_auth( get_self() );    
    CHECK( is_account(conf_contract), "Invalid account of conf_contract");
    _gstate.conf_contract = conf_contract;
    _conf(true);
}

void otcbook::setmerchant(const name& owner, const name& merchant, const string &merchant_name, const string &merchant_detail, const string& email, const string& memo) {
    name admin = _conf().managers.at(otc::manager_type::admin);
    require_auth(admin);
    auto isAdmin = (owner == admin);
    if (!isAdmin) {
        check(owner == merchant, "non-admin not allowed to set merchant" );
    }
    check(is_account(merchant), "account not activated");
    check(email.size() < 64, "email size too large: " + to_string(email.size()) );
    check(memo.size() < max_memo_size, "memo size too large: " + to_string(memo.size()) );

    merchant_t merchant_raw(merchant);
    if (!_dbc.get(merchant_raw)) { // first register, init
        merchant_raw.state = (isAdmin) ? (uint8_t)merchant_state_t::ENABLED : (uint8_t)merchant_state_t::REGISTERED;
    }
    merchant_raw.merchant_name = merchant_name;
    merchant_raw.merchant_detail = merchant_detail;
    merchant_raw.email = email;
    merchant_raw.memo = memo;
    merchant_raw.updated_at = time_point_sec(current_time_point());

    _dbc.set( merchant_raw, get_self() );
}

void otcbook::enbmerchant(const name& owner, bool is_enabled) {
    require_auth( _conf().managers.at(otc::manager_type::admin) );

    merchant_t merchant(owner);
    check( _dbc.get(merchant), "merchant not found: " + owner.to_string() );
    if (is_enabled) {
        check((merchant_state_t)merchant.state != merchant_state_t::ENABLED,
            "merchant has been enabled");
        merchant.state = (uint8_t)merchant_state_t::ENABLED;
    } else {
        check((merchant_state_t)merchant.state != merchant_state_t::DISABLED,
            "merchant has been disabled");
        merchant.state = (uint8_t)merchant_state_t::DISABLED;
    }
    _dbc.set( merchant , get_self());
}

/**
 * only merchant allowed to open orders
 */
void otcbook::openorder(const name& owner, const name& order_side, const set<name> &pay_methods, const asset& va_quantity, const asset& va_price,
    const asset& va_min_take_quantity,  const asset& va_max_take_quantity, const string &memo
){
    check(_conf().status == (uint8_t)status_type::RUNNING, "service is in maintenance");
    require_auth( owner );
    check( ORDER_SIDES.count(order_side) != 0, "Invalid order side" );
    check( va_quantity.is_valid(), "Invalid quantity");
    check( va_price.is_valid(), "Invalid va_price");
    const auto& conf = _conf();
    check( va_price.symbol == conf.fiat_type, "va price symbol not allow");
    check( conf.coin_as_stake.count(va_quantity.symbol), "va quantity symbol hasn't config stake asset");
    if (order_side == BUY_SIDE) {
        check( conf.buy_coins_conf.count(va_quantity.symbol) != 0, "va quantity symbol not allowed for buying" );
    } else {
        check( conf.sell_coins_conf.count(va_quantity.symbol) != 0, "va quantity symbol not allowed for selling" );
    }

    for (auto& method : pay_methods) {
        check( conf.pay_type.count(method) != 0, "pay method illegal: " + method.to_string() );
    }

    check( va_quantity.amount > 0, "quantity must be positive");
    // TODO: min order quantity
    check( va_price.amount > 0, "va price must be positive" );
    // TODO: price range
    check( va_min_take_quantity.symbol == va_quantity.symbol, "va_min_take_quantity Symbol mismatch with quantity" );
    check( va_max_take_quantity.symbol == va_quantity.symbol, "va_max_take_quantity Symbol mismatch with quantity" );
    check( va_min_take_quantity.amount > 0 && va_min_take_quantity.amount <= va_quantity.amount,
        "invalid va_min_take_quantity amount" );
    check( va_max_take_quantity.amount > 0 && va_max_take_quantity.amount <= va_quantity.amount,
        "invalid va_max_take_quantity amount" );

    merchant_t merchant(owner);
    check( _dbc.get(merchant), "merchant not found: " + owner.to_string() );
    check((merchant_state_t)merchant.state == merchant_state_t::ENABLED,
        "merchant not enabled");

    auto stake_frozen = _calc_order_stakes(va_quantity); // TODO: process 70% used-rate of stake
    _frozen(merchant, stake_frozen);

    // TODO: check pos_staking_contract
    // if (_gstate.min_pos_stake_frozen.amount > 0) {
    // 	auto staking_con = _gstate.pos_staking_contract;
    // 	balances bal(staking_con, staking_con.value);
    // 	auto itr = bal.find(owner.value);
    // 	check( itr != bal.end(), "POS staking not found for: " + owner.to_string() );
    // 	check( itr->remaining >= _gstate.min_pos_stake_frozen, "POS Staking requirement not met" );
    // }

    order_t order;
    order.owner 				    = owner;
    order.va_price				    = va_price;
    order.va_quantity			    = va_quantity;
    order.stake_frozen              = stake_frozen;
    order.va_min_take_quantity      = va_min_take_quantity;
    order.va_max_take_quantity      = va_max_take_quantity;
    order.memo                      = memo;
    order.stake_frozen              = stake_frozen;
    order.status				    = (uint8_t)order_status_t::RUNNING;
    order.created_at			    = time_point_sec(current_time_point());
    order.va_frozen_quantity       = asset(0, va_quantity.symbol);
    order.va_fulfilled_quantity    = asset(0, va_quantity.symbol);
    order.accepted_payments         = pay_methods;
    order.merchant_name             = merchant.merchant_name;
    order.updated_at                = time_point_sec(current_time_point());


    if (order_side == BUY_SIDE) {
        buy_order_table_t orders(_self, _self.value);
        order.id = orders.available_primary_key();
        orders.emplace( _self, [&]( auto& row ) {
            row = order;
        });
    } else {
        sell_order_table_t orders(_self, _self.value);
        order.id = orders.available_primary_key();
        orders.emplace( _self, [&]( auto& row ) {
            row = order;
        });
    }
}

void otcbook::pauseorder(const name& owner, const name& order_side, const uint64_t& order_id) {
    require_auth( owner );

    merchant_t merchant(owner);
    check( _dbc.get(merchant), "merchant not found: " + owner.to_string() );
    check( ORDER_SIDES.count(order_side) != 0, "Invalid order side" );

    auto order_wrapper_ptr = (order_side == BUY_SIDE) ?
        buy_order_wrapper_t::get_from_db(_self, _self.value, order_id)
        : sell_order_wrapper_t::get_from_db(_self, _self.value, order_id);
    check( order_wrapper_ptr != nullptr, "order not found");
    const auto &order = order_wrapper_ptr->get_order();
    check( owner == order.owner, "have no access to close others' order");
    check( (order_status_t)order.status == order_status_t::RUNNING, "order not running" );
    order_wrapper_ptr->modify(_self, [&]( auto& row ) {
        row.status = (uint8_t)order_status_t::PAUSED;
        row.updated_at = time_point_sec(current_time_point());
    });
}

void otcbook::resumeorder(const name& owner, const name& order_side, const uint64_t& order_id) {
    require_auth( owner );

    merchant_t merchant(owner);
    check( _dbc.get(merchant), "merchant not found: " + owner.to_string() );
    check( ORDER_SIDES.count(order_side) != 0, "Invalid order side" );

    auto order_wrapper_ptr = (order_side == BUY_SIDE) ?
        buy_order_wrapper_t::get_from_db(_self, _self.value, order_id)
        : sell_order_wrapper_t::get_from_db(_self, _self.value, order_id);
    check( order_wrapper_ptr != nullptr, "order not found");
    const auto &order = order_wrapper_ptr->get_order();
    check( owner == order.owner, "have no access to close others' order");
    check( (order_status_t)order.status == order_status_t::PAUSED, "order not paused" );
    order_wrapper_ptr->modify(_self, [&]( auto& row ) {
        row.status = (uint8_t)order_status_t::RUNNING;
        row.updated_at = time_point_sec(current_time_point());
    });
}

void otcbook::closeorder(const name& owner, const name& order_side, const uint64_t& order_id) {
    require_auth( owner );

    merchant_t merchant(owner);
    check( _dbc.get(merchant), "merchant not found: " + owner.to_string() );
    check( ORDER_SIDES.count(order_side) != 0, "Invalid order side" );

    auto order_wrapper_ptr = (order_side == BUY_SIDE) ?
        buy_order_wrapper_t::get_from_db(_self, _self.value, order_id)
        : sell_order_wrapper_t::get_from_db(_self, _self.value, order_id);
    check( order_wrapper_ptr != nullptr, "order not found");
    const auto &order = order_wrapper_ptr->get_order();
    check( owner == order.owner, "have no access to close others' order");
    check( (uint8_t)order.status != (uint8_t)order_status_t::CLOSED, "order already closed" );
    check( order.va_frozen_quantity.amount == 0, "order being processed" );
    check( order.va_quantity >= order.va_fulfilled_quantity, "order quantity insufficient" );

    _unfrozen(merchant, order.stake_frozen);

    _dbc.set( merchant , get_self());

    order_wrapper_ptr->modify(_self, [&]( auto& row ) {
        row.status = (uint8_t)order_status_t::CLOSED;
        row.closed_at = time_point_sec(current_time_point());
        row.updated_at  = time_point_sec(current_time_point());
    });

}

void otcbook::opendeal(const name& taker, const name& order_side, const uint64_t& order_id,
    const asset& deal_quantity, const uint64_t& order_sn,
    const string& session_msg
) {
    auto conf = _conf();
    check(conf.status == (uint8_t)status_type::RUNNING, "service is in maintenance");
    require_auth( taker );

    check( ORDER_SIDES.count(order_side) != 0, "Invalid order side" );

    auto order_wrapper_ptr = (order_side == BUY_SIDE) ?
        buy_order_wrapper_t::get_from_db(_self, _self.value, order_id)
        : sell_order_wrapper_t::get_from_db(_self, _self.value, order_id);
    check( order_wrapper_ptr != nullptr, "order not found");
    const auto &order = order_wrapper_ptr->get_order();
    check( order.owner != taker, "taker can not be equal to maker");
    check( deal_quantity.symbol == order.va_quantity.symbol, "Token Symbol mismatch" );
    check( order.status == (uint8_t)order_status_t::RUNNING, "order not running" );
    check( order.va_quantity >= order.va_frozen_quantity + order.va_fulfilled_quantity + deal_quantity,
        "Order's quantity insufficient" );
    check( deal_quantity >= order.va_min_take_quantity, "Order's min accept quantity not met!" );
    check( deal_quantity <= order.va_max_take_quantity, "Order's max accept quantity not met!" );

    auto now = current_time_point();

    blacklist_t::idx_t blacklist_tbl(_self, _self.value);
    auto blacklist_itr = blacklist_tbl.find(taker.value);
    CHECK(blacklist_itr == blacklist_tbl.end() || blacklist_itr->expired_at <= now,
        "taker is in blacklist")

    asset order_price = order.va_price;
    name order_maker = order.owner;
    string merchant_name = order.merchant_name;

    deal_t::idx_t deals(_self, _self.value);
    auto ordersn_index 			= deals.get_index<"ordersn"_n>();

    check( ordersn_index.find(order_sn) == ordersn_index.end() , "order_sn already existing!" );
    auto deal_fee = _calc_deal_fee(deal_quantity);

    auto deal_id = deals.available_primary_key();
    deals.emplace( taker, [&]( auto& row ) {
        row.id 					= deal_id;
        row.order_side 			= order_side;
        row.merchant_name       = merchant_name;
        row.order_id 			= order_id;
        row.order_price			= order_price;
        row.deal_quantity		= deal_quantity;
        row.order_maker			= order_maker;
        row.order_taker			= taker;
        row.status				=(uint8_t)deal_status_t::CREATED;
        row.arbit_status        =(uint8_t)arbit_status_t::UNARBITTED;
        row.created_at			= now;
        row.updated_at          = now;
        row.order_sn 			= order_sn;
        row.deal_fee            = deal_fee;
        // row.expired_at 			= time_point_sec(created_at.sec_since_epoch() + _gstate.withhold_expire_sec);
        row.session.push_back({(uint8_t)account_type_t::USER, taker, (uint8_t)deal_status_t::NONE,
            (uint8_t)deal_action_t::CREATE, session_msg, now});
    });

    // // 添加交易到期表数据
    // deal_expiry_tbl deal_expiries(_self, _self.value);
    // deal_expiries.emplace( _self, [&]( auto& row ){
    //     row.deal_id = deal_id;
    //     row.expired_at 			= time_point_sec(created_at.sec_since_epoch() + _gstate.withhold_expire_sec);
    // });

    order_wrapper_ptr->modify(_self, [&]( auto& row ) {
        row.va_frozen_quantity 	+= deal_quantity;
        row.updated_at          = time_point_sec(current_time_point());
    });

    NOTIFICATION(order_maker, conf.app_info, 
        "deal.new, meta.taker "+ taker.to_string() + ", meta.quantity " + deal_quantity.to_string());
}

/**
 * actively close the deal by order taker
 */
void otcbook::closedeal(const name& account, const uint8_t& account_type, const uint64_t& deal_id, const string& session_msg) {
    require_auth( account );
    auto conf = _conf();
    deal_t::idx_t deals(_self, _self.value);
    auto deal_itr = deals.find(deal_id);
    check( deal_itr != deals.end(), "deal not found: " + to_string(deal_id) );
    auto status = (deal_status_t)deal_itr->status;
    check( (uint8_t)status != (uint8_t)deal_status_t::CLOSED, "deal already closed: " + to_string(deal_id) );
    check( (uint8_t)status != (uint8_t)deal_status_t::CANCELLED, "deal already cancelled: " + to_string(deal_id) );
    auto arbit_status =  (arbit_status_t)deal_itr->arbit_status;
    auto merchant_paid_at = deal_itr->merchant_paid_at;

    switch ((account_type_t) account_type) {
    case account_type_t::USER:
        check( deal_itr->order_taker == account, "taker account mismatched");
        break;
    case account_type_t::ADMIN:
        check( _conf().managers.at(otc::manager_type::admin) == account, "admin account mismatched");
        break;
    case account_type_t::ARBITER:
        check( deal_itr->arbiter == account, "abiter account mismatched");
        break;
    case account_type_t::MERCHANT:
        check( deal_itr->order_maker == account, "merchant account mismatched");
        check( (uint8_t)status == (uint8_t)deal_status_t::MAKER_RECV_AND_SENT, "deal already cancelled: " + to_string(deal_id) );
        check( merchant_paid_at + seconds(_conf().payed_timeout) < current_time_point(), "deal is not expired.");
        break;
    default:
        check(false, "account type not supported: " + to_string(account_type));
        break;
    }

    auto order_id = deal_itr->order_id;
    auto order_wrapper_ptr = (deal_itr->order_side == BUY_SIDE) ?
        buy_order_wrapper_t::get_from_db(_self, _self.value, order_id)
        : sell_order_wrapper_t::get_from_db(_self, _self.value, order_id);
    check( order_wrapper_ptr != nullptr, "order not found");
    const auto &order = order_wrapper_ptr->get_order();

    check( (uint8_t)order.status != (uint8_t)order_status_t::CLOSED, "order already closed" );

    auto action = deal_action_t::CLOSE;
    const auto &order_maker  = deal_itr->order_maker;

    auto deal_quantity = deal_itr->deal_quantity;
    check( order.va_frozen_quantity >= deal_quantity, "Err: order frozen quantity smaller than deal quantity" );
    auto deal_fee= deal_itr->deal_fee;

    if ((account_type_t) account_type == account_type_t::MERCHANT || (account_type_t) account_type == account_type_t::USER) {
        check( deal_status_t::MAKER_RECV_AND_SENT == status,
            "can not process deal action:" + to_string((uint8_t)action)
                + " at status: " + to_string((uint8_t)status) );
    }

    auto stake_quantity = _calc_order_stakes(deal_quantity);
    order_wrapper_ptr->modify(_self, [&]( auto& row ) {
        row.stake_frozen -= stake_quantity;
        row.va_frozen_quantity -= deal_quantity;
        row.va_fulfilled_quantity += deal_quantity;
        row.updated_at = time_point_sec(current_time_point());
    });

    deals.modify( *deal_itr, account, [&]( auto& row ) {
        row.status = (uint8_t)deal_status_t::CLOSED;
        row.closed_at = time_point_sec(current_time_point());
        row.updated_at = time_point_sec(current_time_point());
        row.session.push_back({account_type, account, (uint8_t)status, (uint8_t)action, session_msg, row.closed_at});
    });

    merchant_t merchant(order_maker);
    check( _dbc.get(merchant), "merchant not found: " + order_maker.to_string() );
    _unfrozen(merchant, stake_quantity);
    _sub_balance(merchant, deal_fee, "fee:"+to_string(deal_id));

    const auto &fee_recv_addr  = conf.managers.at(otc::manager_type::feetaker);
    TRANSFER( conf.stake_assets_contract.at(deal_fee.symbol), fee_recv_addr, deal_fee, 
        "otcfee:"+to_string(order_id) + ":" +  to_string(deal_id));

    auto fee = deal_itr->deal_fee;
    asset deal_amount = _calc_deal_amount(deal_itr->deal_quantity);
    name settle_arc = conf.managers.at(otc::manager_type::settlement);
    if(is_account(settle_arc)){
        SETTLE_DEAL(settle_arc,
                    deal_id, 
                    deal_itr->order_maker,
                    deal_itr->order_taker, 
                    deal_amount,
                    fee,
                    0, 
                    deal_itr->created_at, 
                    deal_itr->closed_at);
    }    

    name swap_arc = conf.managers.at(otc::manager_type::swaper);
    if(is_account(swap_arc)){
        SWAP_SETTLE(swap_arc, 
                    deal_itr->order_taker, 
                    fee ,
                    deal_amount);
    }
    name farm_arc = conf.managers.at(otc::manager_type::aplinkfarm);
    if(is_account(farm_arc) 
        && conf.farm_id > 0 && conf.farm_scale > 0 ){
        auto value = multiply_decimal64( fee.amount, get_precision(APLINK_SYMBOL), get_precision(fee.symbol));
        value = value * conf.farm_scale / percent_boost;
        if (aplink::farm::get_avaliable_apples(farm_arc, conf.farm_id).amount>value)
            ALLOT(farm_arc, conf.farm_id, deal_itr->order_taker,
                asset(value, APLINK_SYMBOL), "metabalance farm allot: "+to_string(deal_id));
    }
}

void otcbook::canceldeal(const name& account, const uint8_t& account_type, const uint64_t& deal_id,
                         const string& session_msg, bool is_taker_black) {
    require_auth( account );

    deal_t::idx_t deals(_self, _self.value);
    auto deal_itr = deals.find(deal_id);
    check( deal_itr != deals.end(), "deal not found: " + to_string(deal_id) );
    auto status = (deal_status_t)deal_itr->status;
    auto arbit_status =  (arbit_status_t)deal_itr->arbit_status;
    auto now = current_time_point();

    switch ((account_type_t) account_type) {
    case account_type_t::USER:
        switch ((deal_status_t) status) {
            case deal_status_t::CREATED:
                break;
            case deal_status_t::MAKER_ACCEPTED: {
                auto merchant_accepted_at = deal_itr->merchant_accepted_at;
                check(merchant_accepted_at + seconds(_conf().accepted_timeout) < now, "deal is not expired.");
                break;
            }
            default:
                check( false,  "deal status need be CREATED or MAKER_ACCEPTED, deal_id:" + to_string(deal_id));
        }
        check( deal_itr->order_taker == account, "user account mismatched");
        break;
    case account_type_t::MERCHANT: {
        switch ((deal_status_t) status) {
            case deal_status_t::CREATED:
                break;
            case deal_status_t::MAKER_ACCEPTED: {
                auto merchant_accepted_at = deal_itr->merchant_accepted_at;
                check(merchant_accepted_at + seconds(_conf().accepted_timeout) < now, "deal is not expired.");
                if (is_taker_black)
                    _set_blacklist(deal_itr->order_taker, default_blacklist_duration_second, account);
                break;
            }
            default:
                check( false,  "deal status need be CREATED or MAKER_ACCEPTED, deal_id:" + to_string(deal_id));
        }
        check( deal_itr->order_maker == account, "merchant account mismatched");
        break;
    }
    case account_type_t::ADMIN:
        check( _conf().managers.at(otc::manager_type::admin) == account, "admin account mismatched");
        break;
    case account_type_t::ARBITER:
        check( deal_itr->arbiter == account, "abiter account mismatched");
        break;
    default:
        check(false, "account type not supported: " + to_string(account_type));
        break;
    }

    auto order_id = deal_itr->order_id;
    auto order_wrapper_ptr = (deal_itr->order_side == BUY_SIDE) ?
        buy_order_wrapper_t::get_from_db(_self, _self.value, order_id)
        : sell_order_wrapper_t::get_from_db(_self, _self.value, order_id);
    check( order_wrapper_ptr != nullptr, "order not found");
    const auto &order = order_wrapper_ptr->get_order();

    check( (uint8_t)order.status != (uint8_t)order_status_t::CLOSED, "order already closed" );

    deals.modify( *deal_itr, account, [&]( auto& row ) {
            row.arbit_status = (uint8_t)arbit_status_t::UNARBITTED;
            row.status = (uint8_t)deal_status_t::CANCELLED;
            row.closed_at = time_point_sec(current_time_point());
            row.updated_at = time_point_sec(current_time_point());
            row.session.push_back({account_type, account, (uint8_t)status, (uint8_t)deal_action_t::CANCEL, session_msg, row.closed_at});
        });

    auto deal_quantity = deal_itr->deal_quantity;
    // finished deal-canceled
    order_wrapper_ptr->modify(_self, [&]( auto& row ) {
        row.va_frozen_quantity -= deal_quantity;
        row.updated_at = time_point_sec(current_time_point());
    });
}


void otcbook::processdeal(const name& account, const uint8_t& account_type, const uint64_t& deal_id,
    uint8_t action_type, const string& session_msg
) {
    require_auth( account );

    deal_t::idx_t deals(_self, _self.value);
    auto deal_itr = deals.find(deal_id);
    check( deal_itr != deals.end(), "deal not found: " + to_string(deal_id) );

    auto order_wrapper_ptr = (deal_itr->order_side == BUY_SIDE) ?
        buy_order_wrapper_t::get_from_db(_self, _self.value, deal_itr->order_id)
        : sell_order_wrapper_t::get_from_db(_self, _self.value, deal_itr->order_id);
    check( order_wrapper_ptr != nullptr, "order not found");

    auto now = time_point_sec(current_time_point());

    switch ((account_type_t) account_type) {
    case account_type_t::MERCHANT:
        check( deal_itr->order_maker == account, "maker account mismatched");
        NOTIFICATION(deal_itr->order_taker, _conf().app_info, 
            "deal.process, meta.maker "+ account.to_string() + "meta.processed meta.deal " + to_string(deal_id) + " deal.status"+to_string(action_type));
        break;
    case account_type_t::USER:
        check( deal_itr->order_taker == account, "taker account mismatched");
        NOTIFICATION(deal_itr->order_maker, _conf().app_info, 
            "deal.process, meta.taker "+ account.to_string() + "meta.processed meta.deal " + to_string(deal_id) + " deal.status"+to_string(action_type));
        break;
    case account_type_t::ARBITER:
        check( deal_itr->arbiter == account, "arbiter account mismatched");
        break;
    default:
        check(false, "account type not supported: " + to_string(account_type));
        break;
    }

    auto status = (deal_status_t)deal_itr->status;
    auto arbit_status = (arbit_status_t)deal_itr->arbit_status;
    deal_status_t limited_status = deal_status_t::NONE;
    account_type_t limited_account_type = account_type_t::NONE;
    arbit_status_t limit_arbit_status = arbit_status_t::UNARBITTED;
    deal_status_t next_status = deal_status_t::NONE;
    check( status != deal_status_t::CLOSED, "deal already closed: " + to_string(deal_id) );
    check( status != deal_status_t::CANCELLED, "deal already cancelled: " + to_string(deal_id) );

#define DEAL_ACTION_CASE(_action, _limited_account_type, _limit_arbit_status,  _limited_status, _next_status) \
    case deal_action_t::_action:                                                        \
        limited_account_type = account_type_t::_limited_account_type;                   \
        limit_arbit_status = arbit_status_t::_limit_arbit_status;                        \
        limited_status = deal_status_t::_limited_status;                                \
        next_status = deal_status_t::_next_status;                                      \
        break;

    switch ((deal_action_t)action_type)
    {
    // /*               action              account_type  arbit_status, limited_status   next_status  */
    DEAL_ACTION_CASE(MAKER_ACCEPT,          MERCHANT,     UNARBITTED,   CREATED,         MAKER_ACCEPTED)
    DEAL_ACTION_CASE(TAKER_SEND,            USER,         UNARBITTED,   MAKER_ACCEPTED,  TAKER_SENT)
    DEAL_ACTION_CASE(MAKER_RECV_AND_SENT,   MERCHANT,     UNARBITTED,   TAKER_SENT,      MAKER_RECV_AND_SENT)
    DEAL_ACTION_CASE(ADD_SESSION_MSG,       NONE,         NONE,         NONE,            NONE)
    default:
        check(false, "unsupported process deal action:" + to_string((uint8_t)action_type));
        break;
    }

    if (limited_status != deal_status_t::NONE)
        check(limited_status == status, "can not process deal action:" + to_string((uint8_t)action_type)
             + " at status: " + to_string((uint8_t)status) );
    if (limited_account_type != account_type_t::NONE)
        check(limited_account_type == (account_type_t)account_type,
            "can not process deal action:" + to_string((uint8_t)action_type)
             + " by account_type: " + to_string((uint8_t)account_type) );

    if ( (uint8_t)limit_arbit_status != (uint8_t)arbit_status_t::NONE)
        check(arbit_status == limit_arbit_status,
            "can not process deal action:" + to_string((uint8_t)action_type)
             + " by arbit status: " + to_string((uint8_t)arbit_status) );

    deals.modify( *deal_itr, account, [&]( auto& row ) {
        if (next_status != deal_status_t::NONE) {
            row.status = (uint8_t)next_status;
            row.updated_at = time_point_sec(current_time_point());
        }
        if((uint8_t)deal_action_t::MAKER_ACCEPT == action_type) {
            row.merchant_accepted_at = time_point_sec(current_time_point());
        }
        if((uint8_t)deal_action_t::MAKER_RECV_AND_SENT == action_type ) {
            row.merchant_paid_at = time_point_sec(current_time_point());
        }

        row.session.push_back({account_type, account, (uint8_t)status, action_type, session_msg, now});
    });
}


void otcbook::startarbit(const name& account, const uint8_t& account_type, const uint64_t& deal_id,
    const name& arbiter, const string& session_msg) {
    require_auth( account );

    deal_t::idx_t deals(_self, _self.value);
    auto deal_itr = deals.find(deal_id);
    check( deal_itr != deals.end(), "deal not found: " + to_string(deal_id) );

    auto order_wrapper_ptr = (deal_itr->order_side == BUY_SIDE) ?
        buy_order_wrapper_t::get_from_db(_self, _self.value, deal_itr->order_id)
        : sell_order_wrapper_t::get_from_db(_self, _self.value, deal_itr->order_id);
    check( order_wrapper_ptr != nullptr, "order not found");

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

    check( _conf().managers.at(otc::manager_type::arbiter) == arbiter, "arbiter illegal: " + arbiter.to_string() );

    auto status = (deal_status_t)deal_itr->status;
    auto arbit_status = (arbit_status_t)deal_itr->arbit_status;
    check( arbit_status == arbit_status_t::UNARBITTED, "arbit already started: " + to_string(deal_id) );

    set<deal_status_t> can_arbit_status = {deal_status_t::MAKER_ACCEPTED, deal_status_t::TAKER_SENT, deal_status_t::MAKER_RECV_AND_SENT };
    check( can_arbit_status.count(status) != 0, "status illegal: " + to_string((uint8_t)status) );

    deals.modify( *deal_itr, account, [&]( auto& row ) {
        row.arbit_status = (uint8_t)arbit_status_t::ARBITING;
        row.arbiter = arbiter;
        row.updated_at = time_point_sec(current_time_point());
        row.session.push_back({account_type, account, (uint8_t)status, (uint8_t)deal_action_t::START_ARBIT, session_msg, now});
    });
}

void otcbook::closearbit(const name& account, const uint64_t& deal_id, const uint8_t& arbit_result, const string& session_msg) {
    require_auth( account );

    deal_t::idx_t deals(_self, _self.value);
    auto deal_itr = deals.find(deal_id);
    check( deal_itr != deals.end(), "deal not found: " + to_string(deal_id) );

    auto order_wrapper_ptr = (deal_itr->order_side == BUY_SIDE) ?
        buy_order_wrapper_t::get_from_db(_self, _self.value, deal_itr->order_id)
        : sell_order_wrapper_t::get_from_db(_self, _self.value, deal_itr->order_id);
    check( order_wrapper_ptr != nullptr, "order not found");

    auto now = time_point_sec(current_time_point());
    check( deal_itr->arbiter == account, "arbiter account mismatched");

    auto status = (deal_status_t)deal_itr->status;
    auto arbit_status = (arbit_status_t)deal_itr->arbit_status;
    const auto &order_taker  = deal_itr->order_taker;
    const auto &order_maker  = deal_itr->order_maker;
    check( arbit_status == arbit_status_t::ARBITING, "arbit isn't arbiting: " + to_string(deal_id) );

    auto deal_status = (uint8_t)deal_status_t::CLOSED;
    if (arbit_result == 0) {
        deal_status =  (uint8_t)deal_status_t::CANCELLED;
    }

    deals.modify( *deal_itr, account, [&]( auto& row ) {
            row.arbit_status = uint8_t(arbit_result == 0 ? arbit_status_t::CLOSENOFINE : arbit_status_t::CLOSEWITHFINE );
            row.status = (uint8_t)deal_status_t::CLOSED;
            row.closed_at = time_point_sec(current_time_point());
            row.updated_at = time_point_sec(current_time_point());
            row.session.push_back({(uint8_t)account_type_t::ARBITER, account, (uint8_t)status, (uint8_t)deal_action_t::FINISH_ARBIT, session_msg, now});
        });

    auto deal_quantity = deal_itr->deal_quantity;
    auto deal_fee = deal_itr->deal_fee;
    auto deal_price = deal_itr->order_price;
    auto deal_amount = _calc_deal_amount(deal_quantity);
    auto order_id = deal_itr->order_id;

    if (arbit_result == 0) {
        // finished deal-canceled
        order_wrapper_ptr->modify(_self, [&]( auto& row ) {
            row.va_frozen_quantity -= deal_quantity;
            row.updated_at = time_point_sec(current_time_point());
        });
    } else {
        // end deal - finished
        auto stake_quantity = _calc_order_stakes(deal_quantity);
        order_wrapper_ptr->modify(_self, [&]( auto& row ) {
            row.stake_frozen -= stake_quantity;
            row.va_frozen_quantity -= deal_quantity;
            row.va_fulfilled_quantity += deal_quantity;
            row.updated_at = time_point_sec(current_time_point());
        });

        //sub arbit fine
        merchant_t merchant(order_maker);
        check( _dbc.get(merchant), "merchant not found: " + order_maker.to_string() );
        _unfrozen(merchant, stake_quantity);
        _sub_balance(merchant, stake_quantity, "arbit fine:"+to_string(deal_id));

        TRANSFER(_conf().stake_assets_contract.at(stake_quantity.symbol), order_taker, 
            stake_quantity, "arbit fine: "+to_string(deal_id));
   }
}

void otcbook::cancelarbit( const uint8_t& account_type, const name& account, const uint64_t& deal_id, const string& session_msg )
{
    require_auth( account );

    deal_t::idx_t deals(_self, _self.value);
    auto deal_itr = deals.find(deal_id);
    CHECK( deal_itr != deals.end(), "deal not found: " + to_string(deal_id) );
    auto arbit_status = (arbit_status_t)deal_itr->arbit_status;

    CHECK( arbit_status == arbit_status_t::ARBITING, "deal is not arbiting" );
    auto status = deal_itr->status;

    switch ((account_type_t) account_type) {
    case account_type_t::MERCHANT:
        check( deal_itr->order_maker == account, "maker account mismatched");
        check( status == (uint8_t)deal_status_t::MAKER_ACCEPTED,
                    "arbiting only can be cancelled at TAKER_SENT or MAKER_ACCEPTED");
        break;
    // case account_type_t::USER:
    //     check( deal_itr->order_taker == account, "taker account mismatched");
    //     check( status == (uint8_t)deal_status_t::MAKER_RECV_AND_SENT, "arbiting only can be cancelled at MAKER_RECV_AND_SENT");
    //     break;
    default:
        check(false, "account type not supported: " + to_string(account_type));
        break;
    }

    auto now = time_point_sec(current_time_point());
    deals.modify( *deal_itr, account, [&]( auto& row ) {
        row.arbit_status = (uint8_t)arbit_status_t::UNARBITTED;
        row.updated_at = now;
        row.session.push_back({account_type, account, (uint8_t)status,
            (uint8_t)deal_action_t::CANCEL_ARBIT, session_msg, now});
    });
}

void otcbook::resetdeal(const name& account, const uint64_t& deal_id, const string& session_msg){

    require_auth( account );

    CHECK( _conf().managers.at(otc::manager_type::admin) == account, "Only admin allowed" );

    deal_t::idx_t deals(_self, _self.value);
    auto deal_itr = deals.find(deal_id);
    CHECK( deal_itr != deals.end(), "deal not found: " + to_string(deal_id) );

    auto status = (deal_status_t)deal_itr->status;
    CHECK( status != deal_status_t::CLOSED, "deal already closed: " + to_string(deal_id) );
    CHECK( status != deal_status_t::CREATED, "deal no need to reverse" );

    auto now = time_point_sec(current_time_point());
    deals.modify( *deal_itr, account, [&]( auto& row ) {
        row.status = (uint8_t)deal_status_t::CREATED;
        row.updated_at = time_point_sec(current_time_point());
        row.session.push_back({(uint8_t)account_type_t::ADMIN, account, (uint8_t)status,
            (uint8_t)deal_action_t::REVERSE, session_msg, now});
    });
}

void otcbook::withdraw(const name& owner, asset quantity){
    auto conf = _conf();
    check(conf.status == (uint8_t)status_type::RUNNING, "service is in maintenance");
    require_auth( owner );

    check( quantity.amount > 0, "quanity must be positive" );
    check( quantity.symbol.is_valid(), "Invalid quantity symbol name" );
    check( _conf().stake_assets_contract.count(quantity.symbol), "Token Symbol not allowed" );

    merchant_t merchant(owner);
    check( _dbc.get(merchant), "merchant not found: " + owner.to_string() );
    check((merchant_state_t)merchant.state == merchant_state_t::ENABLED,
    "merchant not enabled");

    check((time_point_sec(current_time_point())-merchant.updated_at) > seconds(default_withdraw_limit_second),
        "Can only withdraw after 3 days from fund changed");

    _sub_balance(merchant, quantity, "merchant withdraw");

    TRANSFER( _conf().stake_assets_contract.at(quantity.symbol), owner, quantity, "merchant withdraw" )
}

/*************** Begin of eosio.token transfer trigger function ******************/
/**
 * This happens when a merchant decides to open sell orders
 */
void otcbook::deposit(name from, name to, asset quantity, string memo) {
    if(_self == from || to != _self) return;

    check( _conf().stake_assets_contract.count(quantity.symbol), "Token Symbol not allowed" );
    check( _conf().stake_assets_contract.at(quantity.symbol) == get_first_receiver(), "Token Symbol not allowed" );
    
    merchant_t merchant(from);
    check(_dbc.get( merchant ),"merchant is not set, from:" + from.to_string()+ ",to:" + to.to_string());
    check((merchant_state_t)merchant.state == merchant_state_t::ENABLED,
        "merchant not enabled");
    _add_balance(merchant, quantity, "merchant deposit");
}


void otcbook::setblacklist(const name& account, uint64_t duration_second) {
    require_auth( _conf().managers.at(otc::manager_type::admin) );

    CHECK( is_account(account), "account does not exist: " + account.to_string() );
    CHECK( duration_second <= max_blacklist_duration_second,
           "duration_second too large than: " + to_string(max_blacklist_duration_second));

   _set_blacklist(account, duration_second, _conf().managers.at(otc::manager_type::admin));
}

const otcbook::conf_t& otcbook::_conf(bool refresh/* = false*/) {
    if (!_conf_ptr || refresh) {
        CHECK(_gstate.conf_contract.value != 0, "Invalid conf_table");
        _conf_tbl_ptr = make_unique<conf_table_t>(_gstate.conf_contract, _gstate.conf_contract.value);
        CHECK(_conf_tbl_ptr->exists(), "conf table not existed in contract: " + _gstate.conf_contract.to_string());
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}

void otcbook::stakechanged(const name& account, const asset &quantity, const string& memo){
    require_auth(get_self());
    require_recipient(account);
}

void otcbook::notification(const name& account, const AppInfo_t &info, const string& memo){
    require_auth(get_self());
    require_recipient(account);
}

void otcbook::_set_blacklist(const name& account, uint64_t duration_second, const name& payer) {
    blacklist_t::idx_t blacklist_tbl(_self, _self.value);
    auto blacklist_itr = blacklist_tbl.find(account.value);
    if (duration_second > 0) {
        blacklist_tbl.set( account.value, payer, [&]( auto& row ) {
            row.account     = account;
            row.expired_at  = current_time_point() + eosio::seconds(duration_second);
        });
    } else {
        blacklist_tbl.erase_by_pk(account.value);
    }
}

void otcbook::_add_balance(merchant_t& merchant, const asset& quantity, const string & memo){
    merchant.assets[quantity.symbol].balance += quantity.amount;
    merchant.updated_at = current_time_point();
    _dbc.set( merchant , get_self());
    if(memo.length() > 0) STAKE_CHANGED(merchant.owner, quantity, memo);
}

void otcbook::_sub_balance(merchant_t& merchant, const asset& quantity, const string & memo){
    check( merchant.assets[quantity.symbol].balance >= quantity.amount, "merchant stake balance quantity insufficient");
    merchant.assets[quantity.symbol].balance -= quantity.amount;
    merchant.updated_at = current_time_point();
    _dbc.set( merchant , get_self());
    if(memo.length() > 0) STAKE_CHANGED(merchant.owner, -quantity, memo);
}

void otcbook::_frozen(merchant_t& merchant, const asset& quantity){
    check( merchant.assets[quantity.symbol].balance >= quantity.amount, "merchant stake balance quantity insufficient");
    merchant.assets[quantity.symbol].balance -= quantity.amount;
    merchant.assets[quantity.symbol].frozen += quantity.amount;
    merchant.updated_at = current_time_point();
    _dbc.set( merchant , get_self());
}


void otcbook::_unfrozen(merchant_t& merchant, const asset& quantity){
    check( merchant.assets[quantity.symbol].frozen >= quantity.amount, "merchant stake frozen quantity insufficient");
    merchant.assets[quantity.symbol].frozen -= quantity.amount;
    merchant.assets[quantity.symbol].balance += quantity.amount;
    merchant.updated_at = current_time_point();
    _dbc.set( merchant , get_self());
}