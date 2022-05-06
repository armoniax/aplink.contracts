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

namespace amax {

using namespace std;
using namespace eosio;

#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

static constexpr eosio::name active_perm{"active"_n};
static constexpr eosio::name SYS_BANK{"cnyd.token"_n};

// crypto assets
static constexpr symbol   CNYD_SYMBOL           = SYMBOL("CNYD", 4);
static constexpr symbol   CNY                   = SYMBOL("CNY", 4);
static constexpr symbol   STAKE_SYMBOL          = CNYD_SYMBOL;

static constexpr uint64_t percent_boost     = 10000;
static constexpr uint64_t order_stake_pct   = 10000; // 100%
static constexpr uint64_t max_memo_size     = 1024;

// static constexpr uint64_t seconds_per_year      = 24 * 3600 * 7 * 52;
// static constexpr uint64_t seconds_per_month     = 24 * 3600 * 30;
// static constexpr uint64_t seconds_per_week      = 24 * 3600 * 7;
// static constexpr uint64_t seconds_per_day       = 24 * 3600;
// static constexpr uint64_t seconds_per_hour      = 3600;




#define OTCBOOK_TBL [[eosio::table, eosio::contract("otcbook")]]

struct [[eosio::table("global"), eosio::contract("otcbook")]] global_t {
    // asset min_buy_order_quantity;
    // asset min_sell_order_quantity;
    // asset min_pos_stake_frozen;
    // uint64_t withhold_expire_sec = 600;   // the amount hold will be unfrozen upon expiry
    // name transaction_fee_receiver;  // receiver account to transaction fees
    // uint64_t transaction_fee_ratio = 0; // fee ratio boosted by 10000
    name admin;             // default is contract self
    name conf_contract      = "otcconf"_n;
    bool initialized        = false; 

    EOSLIB_SERIALIZE( global_t, /*(min_buy_order_quantity)(min_sell_order_quantity)*/
                                /*(withhold_expire_sec)(transaction_fee_receiver)
                                (transaction_fee_ratio)*/(admin)(conf_contract)(initialized)
    )
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
    
    REVERSE             = 10,
    ADD_SESSION_MSG     = 11,
    START_ARBIT         = 21,
    FINISH_ARBIT        = 22
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
    FINISHED   = 3
};

struct OTCBOOK_TBL merchant_t {
    name owner;                     // owner account of merchant
    string merchant_name;           // merchant's name
    string merchant_detail;         // merchant's detail
    string email;                   // email
    string memo;                    // memo
    uint8_t state;                 // status, merchant_state_t
    asset stake_free = asset(0, STAKE_SYMBOL);      // staked and free to make orders
    asset stake_frozen = asset(0, STAKE_SYMBOL);     // staked and frozen in orders
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
                                  (email)(memo)(state)(stake_free)(stake_frozen)(updated_at)
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
    asset stake_frozen = asset(0, STAKE_SYMBOL);    // stake frozen asset
    asset total_fee = asset(0, STAKE_SYMBOL);       
    asset fine_amount= asset(0, STAKE_SYMBOL);      // aarbit fine amount, not contain fee
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
                                (stake_frozen)(total_fee)(fine_amount)
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
                                (session))
};

/**
 * deposit log
 */
struct OTCBOOK_TBL fund_log_t {
    uint64_t id = 0;        // PK: available_primary_key, auto increase
    name owner;             // merchant
    uint64_t order_id;
    name order_side;
    name action;            // operation action, [deposit, withdraw, openorder, closeorder]
    asset quantity;         // maybe positive(plus) or negative(minus)
    time_point_sec log_at;  // log time at 
    time_point_sec updated_at;

    fund_log_t() {}
    fund_log_t(uint64_t i): id(i) {}
    uint64_t primary_key() const { return id; }
    uint64_t scope() const { return /*order_price.symbol.code().raw()*/ 0; }

    uint64_t by_update_time() const {
        return (uint64_t) updated_at.utc_seconds ;
    }

    typedef eosio::multi_index
    <"fundlog"_n, fund_log_t,
        indexed_by<"updatedat"_n, const_mem_fun<fund_log_t, uint64_t, &fund_log_t::by_update_time> >
    > table_t;

    EOSLIB_SERIALIZE(fund_log_t,    (id)(owner)(order_id)(order_side)(action)(quantity)(log_at)(updated_at) )
};

// /**
//  * 交易订单过期时间
//  *
//  */
// struct OTCBOOK_TBL deal_expiry_t{
//     uint64_t deal_id;
//     time_point_sec expired_at;

//     deal_expiry_t() {}
//     deal_expiry_t(uint64_t i): deal_id(i) {}

//     uint64_t primary_key()const { return deal_id; }
//     uint64_t scope()const { return 0; }

//     uint64_t by_expired_at() const    { return uint64_t(expired_at.sec_since_epoch()); }

//     EOSLIB_SERIALIZE(deal_expiry_t,  (deal_id)(expired_at) )
// };

// typedef eosio::multi_index
//     <"dealexpiries"_n, deal_expiry_t ,
//         indexed_by<"expiry"_n,    const_mem_fun<deal_expiry_t, uint64_t, &deal_expiry_t::by_expired_at>   >
//     > deal_expiry_tbl;

} // AMA
