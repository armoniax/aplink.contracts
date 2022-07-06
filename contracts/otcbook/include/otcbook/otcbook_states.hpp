 #pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include <deque>
#include <optional>
#include <string>
#include <map>
#include <set>
#include <type_traits>

namespace metabalance {

using namespace std;
using namespace eosio;

#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

static constexpr eosio::name active_perm{"active"_n};

// crypto assets
static constexpr symbol   CNYD_SYMBOL           = SYMBOL("CNYD", 4);
static constexpr symbol   CNY                   = SYMBOL("CNY", 4);
static constexpr symbol   APLINK_SYMBOL              = SYMBOL("APL", 4);

static constexpr uint64_t percent_boost     = 10000;
static constexpr uint64_t order_stake_pct   = 10000; // 100%
static constexpr uint64_t max_memo_size     = 1024;

static constexpr uint64_t seconds_per_day                   = 24 * 3600;
static constexpr uint64_t seconds_per_year                  = 365 * seconds_per_day;
static constexpr uint64_t max_blacklist_duration_second     = 100 * seconds_per_year; // 100 year

#ifndef DEFAULT_BLACKLIST_DURATION_SECOND_FOR_TEST
static constexpr uint64_t default_blacklist_duration_second = 3 * seconds_per_day;    // 3 days
#else
#warning "DEFAULT_BLACKLIST_DURATION_SECOND_FOR_TEST should only be used for test
static constexpr uint64_t default_blacklist_duration_second = DEFAULT_BLACKLIST_DURATION_SECOND_FOR_TEST;
#endif//DEFAULT_BLACKLIST_DURATION_SECOND_FOR_TEST

#ifndef DEFAULT_WITHDRAW_LIMIT_SECOND_FOR_TEST
static constexpr uint64_t default_withdraw_limit_second = 3 * seconds_per_day;    // 3 days
#else
#warning "DEFAULT_WITHDRAW_LIMIT_SECOND_FOR_TEST should only be used for test
static constexpr uint64_t default_withdraw_limit_second = DEFAULT_WITHDRAW_LIMIT_SECOND_FOR_TEST;
#endif//DEFAULT_WITHDRAW_LIMIT_SECOND_FOR_TEST

#define OTCBOOK_TBL [[eosio::table, eosio::contract("otcbook")]]

struct [[eosio::table("global"), eosio::contract("otcbook")]] global_t {
    name conf_contract      = "otcconf"_n;
    EOSLIB_SERIALIZE( global_t, (conf_contract))
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

enum class account_type_t: uint8_t {
    NONE           = 0,
    ADMIN          = 1,
    MERCHANT       = 2,    // merchant
    USER           = 3,    // user
    ARBITER        = 4
};

enum class deal_action_t: uint8_t {
    CREATE              = 1,
    MAKER_ACCEPT        = 2,
    TAKER_SEND          = 3,
    MAKER_RECV_AND_SENT = 4,
    CLOSE               = 5,
    CANCEL              = 9,

    REVERSE             = 10,
    ADD_SESSION_MSG     = 11,
    START_ARBIT         = 21,
    FINISH_ARBIT        = 22,
    CANCEL_ARBIT        = 23
};


enum class order_status_t: uint8_t {
    NONE                = 0,
    RUNNING             = 1,
    PAUSED              = 2,
    CLOSED              = 3
};


enum class deal_status_t: uint8_t {
    NONE                = 0,
    CREATED             = 1,
    MAKER_ACCEPTED      = 2,
    TAKER_SENT          = 3,
    MAKER_RECV_AND_SENT = 4,
    CLOSED              = 5,
    CANCELLED           = 9
};

// order sides
static constexpr name BUY_SIDE = "buy"_n;
static constexpr name SELL_SIDE = "sell"_n;
static const set<name> ORDER_SIDES = {
    BUY_SIDE, SELL_SIDE
};

enum class merchant_state_t: uint8_t {
    NONE        = 0,
    REGISTERED  = 1,
    DISABLED    = 2,
    ENABLED     = 3
};

enum class arbit_status_t: uint8_t {
    NONE        =0,
    UNARBITTED = 1,
    ARBITING   = 2,
    CLOSENOFINE = 3,
    CLOSEWITHFINE = 4
};

struct asset_stake{
    uint64_t balance;
    uint64_t frozen;
};

struct OTCBOOK_TBL merchant_t {
    name owner;                     // owner account of merchant
    string merchant_name;           // merchant's name
    string merchant_detail;         // merchant's detail
    string email;                   // email
    string memo;                    // memo
    uint8_t state;                 // status, merchant_state_t
    map<symbol, asset_stake> assets;
    time_point_sec updated_at;

    merchant_t() {}
    merchant_t(const name& o): owner(o) {}


    uint64_t primary_key()const { return owner.value; }
    uint64_t scope()const { return 0; }
    uint64_t by_update_time() const {
        return (uint64_t) updated_at.utc_seconds ;
    }

    typedef eosio::multi_index<"merchants"_n, merchant_t,
        indexed_by<"updatedat"_n, const_mem_fun<merchant_t, uint64_t, &merchant_t::by_update_time> >
    > idx_t;

    EOSLIB_SERIALIZE(merchant_t,  (owner)(merchant_name)(merchant_detail)
                                  (email)(memo)(state)(assets)(updated_at)
    )
};

/**
 * Generic order struct for maker(merchant)
 * when the owner decides to close it before complete fulfillment, it just get erased
 * if it is truly fulfilled, it also get deleted.
 */
struct OTCBOOK_TBL order_t {
    uint64_t id = 0;                                // PK: available_primary_key, auto increase

    name owner;                                     // order maker's account, merchant
    string merchant_name;
    set<name> accepted_payments;                    // accepted payments
    asset va_price;                                 // va(virtual asset) quantity price, quote in fiat, see fiat_type
    asset va_quantity;                              // va(virtual asset) quantity, see conf.coin_type
    asset va_min_take_quantity;                     // va(virtual asset) min take quantity quantity for taker, symbol must equal to quantity's
    asset va_max_take_quantity;                     // va(virtual asset) max take quantity quantity for taker, symbol must equal to quantity's
    asset va_frozen_quantity;                       // va(virtual asset) frozen quantity of sell/buy coin
    asset va_fulfilled_quantity;                    // va(virtual asset) fulfilled quantity of sell/buy coin, support partial fulfillment
    asset stake_frozen;                             // stake frozen asset
    string memo;                                    // memo

    uint8_t status;                                 //
    time_point_sec created_at;                      // created time at
    time_point_sec closed_at;                       // closed time at
    time_point_sec updated_at;

    order_t() {}
    order_t(const uint64_t& i): id(i) {}

    uint64_t primary_key() const { return id; }
    // uint64_t scope() const { return price.symbol.code().raw(); } //not in use actually

    // check this order can be took by user
    inline bool can_be_taken() const {
        return (order_status_t)status == order_status_t::RUNNING && va_quantity >= va_frozen_quantity + va_fulfilled_quantity + va_min_take_quantity;
    }

    // sort by order maker account + status(is closed) + id
    // owner: lower first
    // status: closed=true in first(=0), not can_be_book in second(=1), others in third(=2)
    // id: lower first
    // should use --revert option when get table by this index
    uint128_t by_maker_status() const {
        uint128_t orderStatus = (status == (uint8_t)order_status_t::CLOSED) ? 0 : !can_be_taken() ? 1 : 2;
        return (uint128_t)owner.value << 64 | orderStatus;
    }

    uint64_t by_update_time() const {
        return (uint64_t) updated_at.utc_seconds ;
    }

    EOSLIB_SERIALIZE(order_t,   (id)(owner)(merchant_name)(accepted_payments)(va_price)(va_quantity)
                                (va_min_take_quantity)(va_max_take_quantity)(va_frozen_quantity)(va_fulfilled_quantity)
                                (stake_frozen)
                                (memo)(status)(created_at)(closed_at)(updated_at))
};

/**
 * buyorders table
 * index 2: by_invprice
 * index 3: by_maker_status
 */
typedef eosio::multi_index
< "buyorders"_n,  order_t,
    indexed_by<"updatedat"_n, const_mem_fun<order_t, uint64_t, &order_t::by_update_time> >,
    indexed_by<"maker"_n, const_mem_fun<order_t, uint128_t, &order_t::by_maker_status> >
> buy_order_table_t;

/**
 * buyorders table
 * index 2: by_price
 * index 3: by_maker_status
 */
typedef eosio::multi_index
< "sellorders"_n, order_t,
    indexed_by<"updatedat"_n, const_mem_fun<order_t, uint64_t, &order_t::by_update_time> >,
    indexed_by<"maker"_n, const_mem_fun<order_t, uint128_t, &order_t::by_maker_status> >
> sell_order_table_t;


struct order_wrapper_t {
    typedef std::function<void(order_t&)> updater_t;
    virtual ~order_wrapper_t() {}
    virtual const order_t& get_order() const = 0;
    virtual void modify(eosio::name payer, const updater_t& updater) = 0;
};

buy_order_table_t::const_iterator *tttt;

template<typename table_t>
struct order_wrapper_impl_t: public order_wrapper_t {

    virtual const order_t& get_order() const override {
        return *_row_ptr;
    }

    void modify(eosio::name payer, const updater_t& updater) override {
        _table->modify(*_row_ptr, payer, updater);
    }

    static std::shared_ptr<order_wrapper_t> get_from_db(name code, uint64_t scope, uint64_t pk) {
        auto ret = std::make_shared<order_wrapper_impl_t<table_t>>();
        ret->_table = make_unique<table_t>(code, scope);
        auto itr = ret->_table->find(pk);
        if (itr != ret->_table->end()) {
            ret->_row_ptr = &(*itr);
            return ret;
        } else {
            return nullptr;
        }
    }

private:
    typedef typename table_t::const_iterator const_iterator;
    std::unique_ptr<table_t> _table;
    const order_t *_row_ptr;
};

typedef order_wrapper_impl_t<buy_order_table_t> buy_order_wrapper_t;
typedef order_wrapper_impl_t<sell_order_table_t> sell_order_wrapper_t;

/**
 * deal session msg(message)
 */
struct deal_session_msg_t {
    uint8_t account_type = 0;   // account type
    name account;               // action account
    uint8_t status = 0;         // status before action, deal_status_t
    uint8_t action = 0;         // action type, deal_action_t
    string msg;                // msg(message)
    time_point_sec created_at;  // created time at

    EOSLIB_SERIALIZE(deal_session_msg_t,   (account_type)(account)(status)(action)(msg)(created_at) )
};

/**
 * buy/sell deal
 *
 */
struct OTCBOOK_TBL deal_t {
    uint64_t id = 0;                // PK: available_primary_key, auto increase
    name order_side;                // order side, buy(1) or sell(2)
    uint64_t order_id = 0;          // order id, created by maker by openorder()
    asset order_price;              // order price, deal price
    asset deal_quantity;            // deal quantity
    name order_maker;               // maker, merchant
    string merchant_name;           // merchant's name
    name order_taker;               // taker, user
    asset deal_fee;                 // deal fee
    asset fine_amount;              // aarbit fine amount, not contain fee
    uint8_t status = 0;             // status
    uint8_t arbit_status = 0;       // arbit status
    name arbiter;

    time_point_sec created_at;      // create time at
    time_point_sec closed_at;       // closed time at
    time_point_sec updated_at;
    uint64_t order_sn = 0;          // order sn, created by external app

    vector<deal_session_msg_t> session; // session

    time_point_sec merchant_accepted_at;  // merchant accepted time
    time_point_sec merchant_paid_at;      // merchant paid time

    deal_t() {}
    deal_t(uint64_t i): id(i) {}

    uint64_t primary_key() const { return id; }
    uint64_t scope() const { return /*order_price.symbol.code().raw()*/ 0; }

    uint128_t by_order()     const { return (uint128_t)order_id << 64 | status; }
    uint64_t by_ordersn()    const { return order_sn;}
    uint64_t by_update_time() const {
        return (uint64_t) updated_at.utc_seconds ;
    }
    typedef eosio::multi_index
    <"deals"_n, deal_t,
        indexed_by<"updatedat"_n, const_mem_fun<deal_t, uint64_t, &deal_t::by_update_time> >,
        indexed_by<"order"_n,   const_mem_fun<deal_t, uint128_t, &deal_t::by_order> >,
        indexed_by<"ordersn"_n, const_mem_fun<deal_t, uint64_t, &deal_t::by_ordersn> >
    > idx_t;

    EOSLIB_SERIALIZE(deal_t,    (id)(order_side)(order_id)(order_price)(deal_quantity)
                                (order_maker)(merchant_name)
                                (order_taker)(deal_fee)(fine_amount)
                                (status)(arbit_status)(arbiter)
                                (created_at)(closed_at)(updated_at)(order_sn)
                                (session)
                                (merchant_accepted_at)(merchant_paid_at))
};

struct OTCBOOK_TBL blacklist_t {
    name account;               // account, PK
    time_point_sec expired_at;  // expired at time

    uint64_t primary_key() const { return account.value; }
    uint64_t scope() const { return /*order_price.symbol.code().raw()*/ 0; }

    typedef wasm::db::multi_index_ex <"blacklist"_n, blacklist_t> idx_t;
};

} // AMA
