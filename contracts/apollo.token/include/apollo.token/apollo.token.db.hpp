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
static constexpr eosio::name SYS_BANK{"apollo.token"_n};

static constexpr uint64_t percent_boost     = 10000;
static constexpr uint64_t max_memo_size     = 1024;

// static constexpr uint64_t seconds_per_year      = 24 * 3600 * 7 * 52;
// static constexpr uint64_t seconds_per_month     = 24 * 3600 * 30;
// static constexpr uint64_t seconds_per_week      = 24 * 3600 * 7;
// static constexpr uint64_t seconds_per_day       = 24 * 3600;
// static constexpr uint64_t seconds_per_hour      = 3600;


#define APLTOKEN_TBL struct [[eosio::table, eosio::contract("apollo.token")]]

struct [[eosio::table("global"), eosio::contract("apollo.token")]] global_t {
    name admin;             // default is contract self
    bool initialized        = false; 

    EOSLIB_SERIALIZE( global_t, (admin)(initialized) )
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
    CLOSED              = 5
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

    uint64_t by_state()     const { return state; }

    uint64_t primary_key()const { return owner.value; }
    uint64_t scope()const { return 0; }
    uint64_t by_update_time() const {
        return (uint64_t) updated_at.utc_seconds ;
    }

    typedef eosio::multi_index<"merchants"_n, merchant_t,
        indexed_by<"status"_n, const_mem_fun<merchant_t, uint64_t, &merchant_t::by_state> >,
        indexed_by<"updatedat"_n, const_mem_fun<merchant_t, uint64_t, &merchant_t::by_update_time> >
    > idx_t;

    EOSLIB_SERIALIZE(merchant_t,  (owner)(merchant_name)(merchant_detail)
                                  (email)(memo)(state)(stake_free)(stake_frozen)(updated_at)
    )
};

/**
 * mining equipment product NFT token
 * 
 */
APLTOKEN_TBL mine_product_t {
    uint64_t id = 0;                                // PK: available_primary_key, auto increase
    string uri;                                     // token_uri
    name owner;
    uint16_t quantity;
    
    string product_sn;                              //product serial no
    string manufacture_cn;
    string manufacture_en;
    string mine_coin_symbol;                        //BTC, ETH
    float hashrate;                                 //E.g. 21.457 MH/s
    char hashrate_unit;                             //M, T
    float power_in_watt;                            //E.g. 2100 Watt
    asset daily_electricity_charge;                 //E.g. "0.85 CNYD"
    asset price;                                    //E.g. "10000 CNYD"
    asset daily_earing_base;                        //E.g. "0.00397002 AMETH"
    uint8_t onshelf_days;                           //0: T+0, 1:T+1
    uint16_t shell_life_days;                       //running time (E.g. 3*365)
    uint16_t available_electricity;
    bool onshelf;                                   //if false, it can no longer be transferred
    time_point_sec created_at;
    time_point_sec mine_started_at;     
    time_point_sec last_settled_at;

    mine_product_t() {}
    mine_product_t(const uint64_t& i): id(i) {}

    uint64_t primary_key() const { return id; }
    // uint64_t scope() const { return price.symbol.code().raw(); } //not in use actually
  
    EOSLIB_SERIALIZE(order_t,   (id)(product_sn)(merchant_name)(accepted_payments)(va_price)(va_quantity)
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
    indexed_by<"price"_n, const_mem_fun<order_t, uint64_t, &order_t::by_invprice> >,
    indexed_by<"maker"_n, const_mem_fun<order_t, uint128_t, &order_t::by_maker_status> >,
    indexed_by<"coin"_n, const_mem_fun<order_t, uint128_t, &order_t::by_coin> >,
    indexed_by<"updatedat"_n, const_mem_fun<order_t, uint64_t, &order_t::by_update_time> >
> buy_order_table_t;

/**
 * buyorders table
 * index 2: by_price
 * index 3: by_maker_status
 */
typedef eosio::multi_index
< "sellorders"_n, order_t,
    indexed_by<"price"_n, const_mem_fun<order_t, uint64_t, &order_t::by_price> >,
    indexed_by<"maker"_n, const_mem_fun<order_t, uint128_t, &order_t::by_maker_status> >,
    indexed_by<"coin"_n, const_mem_fun<order_t, uint128_t, &order_t::by_coin> >,
    indexed_by<"updatedat"_n, const_mem_fun<order_t, uint64_t, &order_t::by_update_time> >
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
    // string ss_hash;                 //plaint shared secret's hash 
    // string user_ss;                 // user's shared secret
    // string merchant_ss;             // merchant's shared secret
    // string arbiter_ss;              // arbiter's shared secret
    time_point_sec created_at;      // create time at
    time_point_sec closed_at;       // closed time at
    time_point_sec updated_at;

    uint64_t order_sn = 0;          // order sn, created by external app
    // time_point_sec expired_at; // 订单到期时间
    // time_point_sec maker_expired_at; // 卖家操作到期时间
    vector<deal_session_msg_t> session; // session

    deal_t() {}
    deal_t(uint64_t i): id(i) {}

    uint64_t primary_key() const { return id; }
    uint64_t scope() const { return /*order_price.symbol.code().raw()*/ 0; }

    uint128_t by_order()     const { return (uint128_t)order_id << 64 | status; }
    uint128_t by_maker()     const { return (uint128_t)order_maker.value << 64 | status ; }
    uint128_t by_taker()     const { return (uint128_t)order_taker.value << 64 | status; }
    uint128_t by_arbiter()   const { return (uint128_t)arbiter.value << 64 | arbit_status; }
    uint64_t by_ordersn()    const { return order_sn;}


    uint128_t by_order_id() const {
        return (uint128_t)order_side.value << 64 | order_id;
    }
    uint128_t by_coin() const {
        return (uint128_t)deal_quantity.symbol.code().raw() << 64 | order_price.amount;
    }
    uint64_t by_update_time() const {
        return (uint64_t) updated_at.utc_seconds ;
    }
    // uint64_t by_expired_at() const    { return uint64_t(expired_at.sec_since_epoch()); }
    // uint64_t by_maker_expired_at() const    { return uint64_t(maker_expired_at.sec_since_epoch()); }

    typedef eosio::multi_index
    <"deals"_n, deal_t,
        indexed_by<"order"_n,   const_mem_fun<deal_t, uint128_t, &deal_t::by_order> >,
        indexed_by<"maker"_n,   const_mem_fun<deal_t, uint128_t, &deal_t::by_maker> >,
        indexed_by<"taker"_n,   const_mem_fun<deal_t, uint128_t, &deal_t::by_taker> >,
        indexed_by<"arbiter"_n, const_mem_fun<deal_t, uint128_t, &deal_t::by_arbiter> >,
        indexed_by<"ordersn"_n, const_mem_fun<deal_t, uint64_t, &deal_t::by_ordersn> >,
        indexed_by<"orderid"_n, const_mem_fun<deal_t, uint128_t, &deal_t::by_order_id> >,
        indexed_by<"coin"_n,    const_mem_fun<deal_t, uint128_t, &deal_t::by_coin> >,
        indexed_by<"updatedat"_n, const_mem_fun<deal_t, uint64_t, &deal_t::by_update_time> >
        // indexed_by<"expiry"_n,  const_mem_fun<deal_t, uint64_t, &deal_t::by_expired_at> >
    > idx_t;

    EOSLIB_SERIALIZE(deal_t,    (id)(order_side)(order_id)(order_price)(deal_quantity)
                                (order_maker)(merchant_name)
                                (order_taker)(deal_fee)(fine_amount)
                                (status)(arbit_status)(arbiter)
                                /*(ss_hash)(user_ss)(merchant_ss)(arbiter_ss)*/
                                (created_at)(closed_at)(updated_at)(order_sn)
                                /*(expired_at)(maker_expired_at)*/
                                (session))
};

/**
 * deposit log
 */
struct OTCBOOK_TBL fund_log_t {
    uint64_t id = 0;        // PK: available_primary_key, auto increase
    uint64_t order_id;
    name order_side;
    name owner;             // merchant
    name action;            // operation action, [deposit, withdraw, openorder, closeorder]
    asset quantity;         // maybe positive(plus) or negative(minus)
    time_point_sec log_at;  // log time at 
    time_point_sec updated_at;

    fund_log_t() {}
    fund_log_t(uint64_t i): id(i) {}
    uint64_t primary_key() const { return id; }
    uint64_t scope() const { return /*order_price.symbol.code().raw()*/ 0; }

    uint64_t by_owner()     const { return owner.value; }

    uint128_t by_action()     const {
        return (uint128_t)action.value << 64 | owner.value;
    }
    uint64_t by_update_time() const {
        return (uint64_t) updated_at.utc_seconds ;
    }

    typedef eosio::multi_index
    <"fundlog"_n, fund_log_t,
        indexed_by<"owner"_n,   const_mem_fun<fund_log_t, uint64_t, &fund_log_t::by_owner> > ,
        indexed_by<"action"_n,   const_mem_fun<fund_log_t, uint128_t, &fund_log_t::by_action> >,
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
