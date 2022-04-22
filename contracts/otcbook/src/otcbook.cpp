#include <eosio.token/eosio.token.hpp>
#include <otcbook/amax_math.hpp>
#include <otcbook/otcbook.hpp>
#include <otcbook/utils.hpp>


using namespace eosio;
using namespace std;
using std::string;

namespace amax {

using namespace std;
using namespace eosio;
using namespace wasm::safemath;

inline int64_t get_precision(const symbol &s) {
    int64_t digit = s.precision();
    CHECK(digit >= 0 && digit <= 18, "precision digit " + std::to_string(digit) + " should be in range[0,18]");
    return calc_precision(digit);
}

inline int64_t get_precision(const asset &a) {
    return get_precision(a.symbol);
}

asset otcbook::_calc_order_stakes(const asset &quantity, const asset &price) {
    // calc order quantity value by price
    auto value = multiply_decimal64( quantity.amount, price.amount, get_precision(price) );

    const auto & prices_quote_cny = _conf().prices_quote_cny;
    if (price.symbol != CNY)
    {
        auto price_itr = prices_quote_cny.find(price.symbol);
        CHECK(price_itr != prices_quote_cny.end() && price_itr->second.amount > 0, "the fiat symbol not have price/cny");
        const auto &price_cny = price_itr->second;
        value = multiply_decimal64( value, price_cny.amount, get_precision(price) );
    }
    // adjust precision
    if (quantity.symbol.precision() != STAKE_SYMBOL.precision()) {
       value = multiply_decimal64( value, get_precision(STAKE_SYMBOL), get_precision(quantity) );
    }

    int64_t amount = divide_decimal64(value, order_stake_pct, percent_boost);

    return asset(amount, STAKE_SYMBOL);
}

asset otcbook::_calc_deal_amount(const asset &quantity, const asset &price) {
    auto value = multiply_decimal64( quantity.amount, price.amount, get_precision(price));
    int64_t amount = multiply_decimal64(value, get_precision(STAKE_SYMBOL), get_precision(quantity));
    return asset(amount, STAKE_SYMBOL);
}

asset otcbook::_calc_deal_fee(const asset &quantity, const asset &price) {
    // calc order quantity value by price
    auto value = multiply_decimal64(quantity.amount, price.amount, get_precision(price));

    const auto & fee_pct = _conf().fee_pct;
    int64_t amount = multiply_decimal64(value, fee_pct, percent_boost);
    amount = multiply_decimal64(amount, get_precision(STAKE_SYMBOL), get_precision(quantity));

    return asset(amount, STAKE_SYMBOL);
}


void otcbook::init(const name &conf_contract) {
    require_auth( _gstate.admin );
    _set_conf(conf_contract);
    _gstate.initialized = true;
}

void otcbook::setconf(const name &conf_contract) {
    require_auth( _gstate.admin );
    _set_conf(conf_contract);
}

void otcbook::_set_conf(const name &conf_contract) {
    require_auth( _gstate.admin );
    CHECK( is_account(conf_contract), "Invalid account of conf_contract");
    _gstate.conf_contract = conf_contract;
    _conf(true);
}

void otcbook::setadmin(const name& admin) {
    require_auth( _self );
    _gstate.admin = admin;
}

void otcbook::setmerchant(const name& owner, const name& merchant, const string &merchant_name, const string &merchant_detail, const string& email, const string& memo) {

    require_auth( owner );
    auto isAdmin = (owner == _gstate.admin);
    if(!isAdmin) {
        check(owner == merchant, "non-admin not allowed to set merchant" );
    }
    check(is_account(merchant), "account have not to be activated");
    check(email.size() < 64, "email size too large: " + to_string(email.size()) );
    check(memo.size() < max_memo_size, "memo size too large: " + to_string(memo.size()) );

    merchant_t merchant_raw(merchant);
    if (!_dbc.get(merchant_raw)) {
        // first register, init
        if(isAdmin)
            merchant_raw.state = (uint8_t)merchant_state_t::ENABLED;
        else
            merchant_raw.state = (uint8_t)merchant_state_t::REGISTERED;
    }
    merchant_raw.merchant_name = merchant_name;
    merchant_raw.merchant_detail = merchant_detail;
    merchant_raw.email = email;
    merchant_raw.memo = memo;

    _dbc.set( merchant_raw );
}

void otcbook::enbmerchant(const name& owner, bool is_enabled) {
    require_auth( _gstate.admin );

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
    _dbc.set( merchant );
}

/**
 * only merchant allowed to open orders
 */
void otcbook::openorder(const name& owner, const name& order_side, const set<name> &pay_methods, const asset& va_quantity, const asset& va_price,
    const asset& va_min_take_quantity,  const asset& va_max_take_quantity, const string &memo
){
    require_auth( owner );
    check( ORDER_SIDES.count(order_side) != 0, "Invalid order side" );
    check( va_quantity.is_valid(), "Invalid quantity");
    check( va_price.is_valid(), "Invalid va_price");
    const auto& conf = _conf();
    check( va_price.symbol == conf.fiat_type, "va price symbol not allow");
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

    auto stake_frozen = _calc_order_stakes(va_quantity, va_price); // TODO: process 70% used-rate of stake
    check( merchant.stake_free >= stake_frozen, "merchant stake quantity insufficient, expected: " + stake_frozen.to_string() );
    merchant.stake_free -= stake_frozen;
    merchant.stake_frozen += stake_frozen;
    _dbc.set( merchant );
    _add_fund_log(owner, "openorder"_n, -stake_frozen);

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

    check( merchant.stake_frozen >= order.stake_frozen, "merchant stake quantity insufficient" );
    // 撤单后币未交易完成的币退回
    merchant.stake_frozen -= order.stake_frozen;
    merchant.stake_free += order.stake_frozen;
    merchant.stake_free -= order.total_fee;
    merchant.stake_free -= order.fine_amount;
    _dbc.set( merchant );
    _add_fund_log(owner, "closeorder"_n, order.stake_frozen);

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
    require_auth( taker );

    // check( deal_quantity >= _gstate.min_buy_order_quantity, "min buy order quantity not met: " +  _gstate.min_buy_order_quantity.to_string() );

    check( ORDER_SIDES.count(order_side) != 0, "Invalid order side" );

    auto order_wrapper_ptr = (order_side == BUY_SIDE) ?
        buy_order_wrapper_t::get_from_db(_self, _self.value, order_id)
        : sell_order_wrapper_t::get_from_db(_self, _self.value, order_id);
    check( order_wrapper_ptr != nullptr, "order not found");
    const auto &order = order_wrapper_ptr->get_order();
    check( order.owner != taker, "taker can not be equal to maker");
    check( deal_quantity.symbol == order.va_quantity.symbol, "Token Symbol mismatch" );
    check( order.status == (uint8_t)order_status_t::RUNNING, "Order not runing" );
    check( order.va_quantity >= order.va_frozen_quantity + order.va_fulfilled_quantity + deal_quantity,
        "Order's quantity insufficient" );
    // check( itr->price.amount * deal_quantity.amount >= itr->va_min_take_quantity.amount * 10000, "Order's min accept quantity not met!" );
    check( deal_quantity >= order.va_min_take_quantity, "Order's min accept quantity not met!" );
    check( deal_quantity <= order.va_max_take_quantity, "Order's max accept quantity not met!" );

    asset order_price = order.va_price;
    // asset order_price_usd = itr->price_usd;
    name order_maker = order.owner;
    string merchant_name = order.merchant_name;

    deal_t::idx_t deals(_self, _self.value);
    auto ordersn_index 			= deals.get_index<"ordersn"_n>();
    auto lower_itr 				= ordersn_index.lower_bound(order_sn);
    auto upper_itr 				= ordersn_index.upper_bound(order_sn);

    check( ordersn_index.find(order_sn) == ordersn_index.end() , "order_sn already existing!" );
    auto deal_fee = _calc_deal_fee(deal_quantity, order_price);

    auto created_at = time_point_sec(current_time_point());
    auto updated_at = time_point_sec(current_time_point());
    auto deal_id = deals.available_primary_key();
    deals.emplace( _self, [&]( auto& row ) {
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
        row.created_at			= created_at;
        row.updated_at          = updated_at;
        row.order_sn 			= order_sn;
        row.deal_fee            = deal_fee;
        // row.expired_at 			= time_point_sec(created_at.sec_since_epoch() + _gstate.withhold_expire_sec);
        row.session.push_back({(uint8_t)account_type_t::USER, taker, (uint8_t)deal_status_t::NONE,
            (uint8_t)deal_action_t::CREATE, session_msg, created_at});
    });

    // // 添加交易到期表数据
    // deal_expiry_tbl deal_expiries(_self, _self.value);
    // deal_expiries.emplace( _self, [&]( auto& row ){
    //     row.deal_id = deal_id;
    //     row.expired_at 			= time_point_sec(created_at.sec_since_epoch() + _gstate.withhold_expire_sec);
    // });

    order_wrapper_ptr->modify(_self, [&]( auto& row ) {
        row.va_frozen_quantity 	+= deal_quantity;
    });
}

/**
 * actively close the deal by order taker
 */
void otcbook::closedeal(const name& account, const uint8_t& account_type, const uint64_t& deal_id, const string& session_msg) {
    require_auth( account );

    deal_t::idx_t deals(_self, _self.value);
    auto deal_itr = deals.find(deal_id);
    check( deal_itr != deals.end(), "deal not found: " + to_string(deal_id) );
    auto status = (deal_status_t)deal_itr->status;
    check( (uint8_t)status != (uint8_t)deal_status_t::CLOSED, "deal already closed: " + to_string(deal_id) );
    auto arbit_status =  (arbit_status_t)deal_itr->arbit_status;

    switch ((account_type_t) account_type) {
    case account_type_t::USER:
        check( deal_itr->order_taker == account, "taker account mismatched");
        break;
    case account_type_t::ADMIN:
        check( _gstate.admin == account, "admin account mismatched");
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

    order_wrapper_ptr->modify(_self, [&]( auto& row ) {
        row.va_frozen_quantity -= deal_quantity;
        row.va_fulfilled_quantity += deal_quantity;
        row.total_fee += deal_fee;
        row.updated_at = time_point_sec(current_time_point());
    });

    deals.modify( *deal_itr, _self, [&]( auto& row ) {
        row.status = (uint8_t)deal_status_t::CLOSED;
        row.closed_at = time_point_sec(current_time_point());
        row.updated_at = time_point_sec(current_time_point());
        row.session.push_back({account_type, account, (uint8_t)status, (uint8_t)action, session_msg, row.closed_at});
    });

    const auto &fee_recv_addr  = _conf().fee_recv_addr;
    TRANSFER( SYS_BANK, fee_recv_addr, deal_fee, to_string(order_id) + ":" +  to_string(deal_id));
    _add_fund_log(order_maker, "dealfee"_n, -deal_fee);
}


void otcbook::canceldeal(const name& account, const uint8_t& account_type, const uint64_t& deal_id, const string& session_msg){
    require_auth( account );

    deal_t::idx_t deals(_self, _self.value);
    auto deal_itr = deals.find(deal_id);
    check( deal_itr != deals.end(), "deal not found: " + to_string(deal_id) );
    auto status = (deal_status_t)deal_itr->status;
    auto arbit_status =  (arbit_status_t)deal_itr->arbit_status;

    switch ((account_type_t) account_type) {
    case account_type_t::USER:
        check( (uint8_t)status == (uint8_t)deal_status_t::CREATED ||  (uint8_t)status == (uint8_t)deal_status_t::MAKER_ACCEPTED,
                     "deal already status need CREATED or MAKER_ACCEPTED " + to_string(deal_id));
        check( deal_itr->order_taker == account, "taker account mismatched");
        break;
    case account_type_t::MERCHANT:
        check( (uint8_t)status == (uint8_t)deal_status_t::CREATED,
                     "deal already status need CREATED or MAKER_ACCEPTED " + to_string(deal_id));
        check( deal_itr->order_maker == account, "merchant account mismatched");
        break;
    case account_type_t::ADMIN:
        check( _gstate.admin == account, "admin account mismatched");
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

    auto action = deal_action_t::CLOSE;

    deals.modify( *deal_itr, _self, [&]( auto& row ) {
            row.arbit_status = (uint8_t)arbit_status_t::UNARBITTED;
            row.status = (uint8_t)deal_status_t::CLOSED;
            row.closed_at = time_point_sec(current_time_point());
            row.updated_at = time_point_sec(current_time_point());
            row.session.push_back({account_type, account, (uint8_t)status, (uint8_t)deal_action_t::FINISH_ARBIT, session_msg, row.closed_at});
        });

    auto deal_quantity = deal_itr->deal_quantity;
    // finished deal-canceled
    order_wrapper_ptr->modify(_self, [&]( auto& row ) {
        row.va_frozen_quantity -= deal_quantity;
        row.updated_at = time_point_sec(current_time_point());
    });
}


void otcbook::processdeal(const name& account, const uint8_t& account_type, const uint64_t& deal_id,
    uint8_t action, const string& session_msg
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
        break;
    case account_type_t::USER:
        check( deal_itr->order_taker == account, "taker account mismatched");
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

#define DEAL_ACTION_CASE(_action, _limited_account_type, _limit_arbit_status,  _limited_status, _next_status) \
    case deal_action_t::_action:                                                        \
        limited_account_type = account_type_t::_limited_account_type;                   \
        limit_arbit_status = arbit_status_t::_limit_arbit_status;                        \
        limited_status = deal_status_t::_limited_status;                                \
        next_status = deal_status_t::_next_status;                                      \
        break;

    switch ((deal_action_t)action)
    {
    // /*               action              account_type  arbit_status, limited_status   next_status  */
    DEAL_ACTION_CASE(MAKER_ACCEPT,          MERCHANT,     UNARBITTED,   CREATED,         MAKER_ACCEPTED)
    DEAL_ACTION_CASE(TAKER_SEND,            USER,         UNARBITTED,   MAKER_ACCEPTED,  TAKER_SENT)
    DEAL_ACTION_CASE(MAKER_RECV_AND_SENT,   MERCHANT,     UNARBITTED,   TAKER_SENT,      MAKER_RECV_AND_SENT)
    DEAL_ACTION_CASE(ADD_SESSION_MSG,       NONE,         NONE,         NONE,            NONE)
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

    if ( (uint8_t)limit_arbit_status != (uint8_t)arbit_status_t::NONE)
        check(arbit_status == limit_arbit_status,
            "can not process deal action:" + to_string((uint8_t)action)
             + " by arbit status: " + to_string((uint8_t)arbit_status) );

    deals.modify( *deal_itr, _self, [&]( auto& row ) {
        if (next_status != deal_status_t::NONE) {
            row.status = (uint8_t)next_status;
            row.updated_at = time_point_sec(current_time_point());
        }
        row.session.push_back({account_type, account, (uint8_t)status, action, session_msg, now});
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
    const auto& conf = _conf();
    // check arbiter is vaild
    check( conf.arbiters.count(arbiter) != 0, "arbiter illegal: " + arbiter.to_string() );

    auto status = (deal_status_t)deal_itr->status;
    auto arbit_status = (arbit_status_t)deal_itr->arbit_status;
    check( arbit_status == arbit_status_t::UNARBITTED, "arbit already started: " + to_string(deal_id) );

    set<deal_status_t> can_arbit_status = {deal_status_t::MAKER_ACCEPTED, deal_status_t::TAKER_SENT, deal_status_t::MAKER_RECV_AND_SENT };
    check( can_arbit_status.count(status) != 0, "status illegal: " + to_string((uint8_t)status) );

    deals.modify( *deal_itr, _self, [&]( auto& row ) {
        row.arbit_status = (uint8_t)arbit_status_t::ARBITING;
        row.arbiter = arbiter;
        row.session.push_back({account_type, account, (uint8_t)status, (uint8_t)deal_action_t::START_ARBIT, session_msg, now});
    });
}

void otcbook::closearbit(const name& account, const uint64_t& deal_id, const uint8_t& arbit_result, const string& session_msg) {
    require_auth( account );
    eosio::print("closearbit: ", deal_id);

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

     deals.modify( *deal_itr, _self, [&]( auto& row ) {
            row.arbit_status = (uint8_t)arbit_status_t::FINISHED;
            row.status = (uint8_t)deal_status_t::CLOSED;
            row.closed_at = time_point_sec(current_time_point());
            row.session.push_back({(uint8_t)account_type_t::ARBITER, account, (uint8_t)status, (uint8_t)deal_action_t::FINISH_ARBIT, session_msg, now});
        });

    auto deal_quantity = deal_itr->deal_quantity;
    auto deal_fee = deal_itr->deal_fee;
    auto deal_price = deal_itr->order_price;
    auto deal_amount = _calc_deal_amount(deal_quantity, deal_price);
    auto order_id = deal_itr->order_id;

    if (arbit_result == 0) {
        // finished deal-canceled
        order_wrapper_ptr->modify(_self, [&]( auto& row ) {
            row.va_frozen_quantity -= deal_quantity;
        });
    } else {
        // end deal - finished
        order_wrapper_ptr->modify(_self, [&]( auto& row ) {
            row.va_frozen_quantity -= deal_quantity;
            row.va_fulfilled_quantity += deal_quantity;
            row.total_fee += deal_fee;
            row.fine_amount = deal_amount;
        });

        //< send CNYD to user
        TRANSFER(SYS_BANK, order_taker, deal_amount, "");

        const auto &fee_recv_addr  = _conf().fee_recv_addr;
        TRANSFER( SYS_BANK, fee_recv_addr, deal_fee, to_string(order_id) + ":" +  to_string(deal_id));
        _add_fund_log(order_maker, "dealfee"_n, -deal_fee);
    }
}

void otcbook::resetdeal(const name& account, const uint64_t& deal_id, const string& session_msg){

    require_auth( account );

    CHECK( _gstate.admin == account, "Only admin allowed" );

    deal_t::idx_t deals(_self, _self.value);
    auto deal_itr = deals.find(deal_id);
    CHECK( deal_itr != deals.end(), "deal not found: " + to_string(deal_id) );

    auto status = (deal_status_t)deal_itr->status;
    CHECK( status != deal_status_t::CLOSED, "deal already closed: " + to_string(deal_id) );
    CHECK( status != deal_status_t::CREATED, "deal no need to reverse" );

    auto now = time_point_sec(current_time_point());
    deals.modify( *deal_itr, _self, [&]( auto& row ) {
        row.status = (uint8_t)deal_status_t::CREATED;
        row.updated_at = time_point_sec(current_time_point());
        row.session.push_back({(uint8_t)account_type_t::ADMIN, account, (uint8_t)status,
            (uint8_t)deal_action_t::REVERSE, session_msg, now});
    });
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
    check((merchant_state_t)merchant.state == merchant_state_t::ENABLED,
    "merchant not enabled");
    check( merchant.stake_free >= quantity, "The withdrawl amount must be less than the balance" );
    merchant.stake_free -= quantity;
    _dbc.set(merchant);

    TRANSFER( SYS_BANK, owner, quantity, "withdraw" )

    _add_fund_log(owner, "withdraw"_n, -quantity);
}

/**
 * 超时检测
 *
 */
// void otcbook::timeoutdeal() {

//     auto check_time = current_time_point().sec_since_epoch() - seconds_per_day;

//     deal_expiry_tbl exp_time(_self,_self.value);
//     auto exp_index = exp_time.get_index<"expiry"_n>();
//     auto lower_itr = exp_index.find(check_time);
//     // auto itr = exp_time.begin()
//     bool processed = false;

//     for (auto itr = exp_index.begin(); itr != lower_itr; ) {
//         if (itr->expired_at <= time_point_sec(check_time)) {
//             deal_t::idx_t deals(_self, _self.value);
//             auto deal_itr = deals.find(itr -> deal_id);

//              // 订单处于买家未操作状态进行关闭
//             if (deal_itr != deals.end() && 
//                 ((deal_status_t)deal_itr->status == deal_status_t::CREATED 
//                     ||  (deal_status_t)deal_itr->status == deal_status_t::MAKER_ACCEPTED) ) {

//                 auto order_id = deal_itr->order_id;
//                 order_table_t orders(_self, _self.value);
//                 auto order_itr = orders.find(order_id);
//                 check( order_itr != orders.end(), "sell order not found: " + to_string(order_id) );
//                 check( !order_itr->closed, "order already closed" );

//                 auto deal_quantity = deal_itr->deal_quantity;
//                 check( order_itr->va_frozen_quantity >= deal_quantity, "Err: order frozen quantity smaller than deal quantity" );

//                 orders.modify( *order_itr, _self, [&]( auto& row ) {
//                     row.va_frozen_quantity -= deal_quantity;
//                 });

//                 deals.modify( *deal_itr, _self, [&]( auto& row ) {
//                     row.closed = true;
//                     row.closed_at = time_point_sec(current_time_point());
//                 });

//                 processed = true;
//             }

//             itr = exp_index.erase(itr);
//         } else {
//             itr ++;
//         }
//     }

// }

/*************** Begin of eosio.token transfer trigger function ******************/
/**
 * This happens when a merchant decides to open sell orders
 */
void otcbook::deposit(name from, name to, asset quantity, string memo) {
    eosio::print("from: ", from, ", to:", to, ", quantity:" , quantity, ", memo:" , memo);
    if(_self == from ){
        return;
    }
    if (to != _self)
        return;
    // check( false, "deposit check test , from:" + from.to_string()+ ",to:" + to.to_string());

    if (get_first_receiver() == SYS_BANK && quantity.symbol == CNYD_SYMBOL){
        merchant_t merchant(from);
        check(_dbc.get( merchant ),"merchant is not set, from:" + from.to_string()+ ",to:" + to.to_string());
        check((merchant_state_t)merchant.state == merchant_state_t::ENABLED,
            "merchant not enabled");
        merchant.stake_free += quantity;
        _dbc.set( merchant );
        _add_fund_log(from, "deposit"_n, quantity);
    }
}
/**
 * 进行数据清除，测试用，发布正式环境去除
 */
void otcbook::deltable(){
    require_auth( _self );

    // order_table_t sellorders(_self,_self.value);
    // auto itr = sellorders.begin();
    // while(itr != sellorders.end()){
    //     itr = sellorders.erase(itr);
    // }

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

    // deal_expiry_tbl exp(_self,_self.value);
    // auto itr3 = exp.begin();
    // while(itr3 != exp.end()){
    //     itr3 = exp.erase(itr3);
    // }

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

void otcbook::_add_fund_log(const name& owner, const name & action, const asset &quantity) {
    auto now = time_point_sec(current_time_point());
    fund_log_t::table_t stake_log_tbl(_self, _self.value);
    auto id = stake_log_tbl.available_primary_key();
    stake_log_tbl.emplace( _self, [&]( auto& row ) {
        row.id 					= id;
        row.owner 			    = owner;
        row.action			    = action;
        row.quantity		    = quantity;
        row.log_at			    = now;
        row.updated_at          = time_point_sec(current_time_point());
    });
}

}  //end of namespace:: amaxbpvoting
