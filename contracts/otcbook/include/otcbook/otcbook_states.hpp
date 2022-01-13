 #pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include <deque>
#include <optional>
#include <string>
#include <type_traits>

namespace mgp {

using namespace std;
using namespace eosio;

static constexpr eosio::name active_perm{"active"_n};
static constexpr eosio::name SYS_BANK{"eosio.token"_n};

static constexpr symbol   SYS_SYMBOL            = symbol(symbol_code("MGP"), 4);
static constexpr symbol   CNY_SYMBOL            = symbol(symbol_code("CNY"), 2);
static constexpr symbol   USD_SYMBOL            = symbol(symbol_code("USD"), 4);
static constexpr uint32_t seconds_per_year      = 24 * 3600 * 7 * 52;
static constexpr uint32_t seconds_per_month     = 24 * 3600 * 30;
static constexpr uint32_t seconds_per_week      = 24 * 3600 * 7;
static constexpr uint32_t seconds_per_day       = 24 * 3600;
static constexpr uint32_t seconds_per_hour      = 3600;
static constexpr uint32_t max_memo_size         = 1024;


#define CONTRACT_TBL [[eosio::table, eosio::contract("otcbook")]]

struct [[eosio::table("global"), eosio::contract("otcbook")]] global_t {
    asset min_buy_order_quantity;
    asset min_sell_order_quantity;
    asset min_pos_stake_quantity;
    name pos_staking_contract;
    uint64_t withhold_expire_sec;   // the amount hold will be unfrozen upon expiry
    name transaction_fee_receiver;  // receiver account to transaction fees
    uint64_t transaction_fee_ratio; // fee ratio boosted by 10000
    set<name> otc_arbiters;
    string cs_contact_title;
    string cs_contact;

    global_t() {
        min_buy_order_quantity      = asset(10, SYS_SYMBOL);
        min_sell_order_quantity     = asset(10, SYS_SYMBOL);
        min_pos_stake_quantity      = asset(0, SYS_SYMBOL);
        withhold_expire_sec         = 600; //10 mins
        transaction_fee_ratio       = 0;
        
    }

    EOSLIB_SERIALIZE( global_t, (min_buy_order_quantity)(min_sell_order_quantity)
                                (min_pos_stake_quantity)(pos_staking_contract)
                                (withhold_expire_sec)(transaction_fee_receiver)
                                (transaction_fee_ratio)(otc_arbiters)
                                (cs_contact_title)(cs_contact) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

struct [[eosio::table("global2"), eosio::contract("otcbook")]] global2_t {
    asset mgp_price; // mgp 价格
    asset usd_exchange_rate; // usd汇率
    name admin; // action 调用员

    global2_t() {
        mgp_price  = asset(0, USD_SYMBOL);
        usd_exchange_rate = asset(0, CNY_SYMBOL);
    }

    EOSLIB_SERIALIZE( global2_t, (mgp_price)(usd_exchange_rate)(admin) )
};
typedef eosio::singleton< "global2"_n, global2_t > global2_singleton;

enum PayType: uint8_t {
    PAYMIN      = 0,
    BANK        = 1,
    WECAHAT     = 2,
    ALIPAY      = 3,
    USDT_ERC20  = 4,
    USDT_TRC20  = 5,
    MASTER      = 6,
    VISA        = 7,
    PAYPAL      = 8,
    PAYMAX      = 9
};

enum UserType: uint8_t {
    MERCHANT       = 0,    // merchant
    USER           = 1,    // user
    ARBITER        = 2
};

/**
 * Generic order struct for buyers/sellers
 * when the owner decides to close it before complete fulfillment, it just get erased
 * if it is truly fufilled, it also get deleted.
 */
struct CONTRACT_TBL order_t {
    uint64_t id;                //PK: available_primary_key

    name owner;                 //order maker's account
    set<uint8_t> accepted_payments;
    asset price;                // MGP price the buyer willing to buy, symbol CNY
    asset price_usd;            // MGP price the buyer willing to buy, symbol USD
    asset quantity;
    asset min_accept_quantity;
    asset frozen_quantity;
    asset fulfilled_quantity;    //support partial fulfillment
    bool closed;
    time_point_sec created_at;
    time_point_sec closed_at;

    order_t() {}
    order_t(const uint64_t& i): id(i) {}

    uint64_t primary_key() const { return id; }
    // uint64_t scope() const { return price.symbol.code().raw(); } //not in use actually

    //to sort sellers orders: smaller-price order first
    uint64_t by_price() const { 
        
        return closed || (frozen_quantity + fulfilled_quantity >= quantity) ? -1 : price.amount; 
    } 
    
    //to sort buyers orders: bigger-price order first
    uint64_t by_invprice() const { return closed ? 0 : std::numeric_limits<uint64_t>::max() - price.amount; } 

    //to sort by order makers account
    uint64_t by_maker() const { return owner.value; }
  
    EOSLIB_SERIALIZE(order_t,   (id)(owner)(accepted_payments)(price)(price_usd)(quantity)(min_accept_quantity)
                                    (frozen_quantity)(fulfilled_quantity)
                                    (closed)(created_at)(closed_at))
};

typedef eosio::multi_index
< "buyorders"_n,  order_t,
    indexed_by<"price"_n, const_mem_fun<order_t, uint64_t, &order_t::by_invprice> >,
    indexed_by<"maker"_n, const_mem_fun<order_t, uint64_t, &order_t::by_maker> >
> buy_order_t;

typedef eosio::multi_index
< "selorders"_n, order_t,
    indexed_by<"price"_n, const_mem_fun<order_t, uint64_t, &order_t::by_price> >,
    indexed_by<"maker"_n, const_mem_fun<order_t, uint64_t, &order_t::by_maker> >
> sell_order_t;

/**
 * buy/sell deal
 *
 */
struct CONTRACT_TBL deal_t {
    uint64_t id;                //PK: available_primary_key

    uint64_t order_id;
    asset order_price;
    asset order_price_usd;
    asset deal_quantity;

    name order_maker;
    bool maker_passed;
    time_point_sec maker_passed_at;

    name order_taker;
    bool taker_passed;
    time_point_sec taker_passed_at;

    name arbiter;
    bool arbiter_passed;
    time_point_sec arbiter_passed_at;

    bool closed;
    time_point_sec created_at;
    time_point_sec closed_at;

    uint64_t order_sn; // 订单号（前端生成）
    uint8_t pay_type; // 选择的支付类型
    time_point_sec expired_at; // 订单到期时间

    time_point_sec maker_expired_at; // 卖家操作到期时间
    uint8_t restart_taker_num; // 重启买家超时次数
    uint8_t restart_maker_num; // 重启卖家超时次数

    deal_t() {}
    deal_t(uint64_t i): id(i) {}

    uint64_t primary_key() const { return id; }
    uint64_t scope() const { return /*order_price.symbol.code().raw()*/ 0; }

    uint64_t by_order()     const { return order_id; }
    uint64_t by_maker()     const { return order_maker.value; }
    uint64_t by_taker()     const { return order_taker.value; }
    uint64_t by_arbiter()   const { return arbiter.value; }
    uint64_t by_ordersn()   const { return order_sn;}
    uint64_t by_expired_at() const    { return uint64_t(expired_at.sec_since_epoch()); }
    uint64_t by_maker_expired_at() const    { return uint64_t(maker_expired_at.sec_since_epoch()); }

    typedef eosio::multi_index
    <"deals"_n, deal_t,
        indexed_by<"order"_n,   const_mem_fun<deal_t, uint64_t, &deal_t::by_order> >,
        indexed_by<"maker"_n,   const_mem_fun<deal_t, uint64_t, &deal_t::by_maker> >,
        indexed_by<"taker"_n,   const_mem_fun<deal_t, uint64_t, &deal_t::by_taker> >,
        indexed_by<"arbiter"_n, const_mem_fun<deal_t, uint64_t, &deal_t::by_arbiter> >,
        indexed_by<"ordersn"_n, const_mem_fun<deal_t, uint64_t, &deal_t::by_ordersn> >,
        indexed_by<"expiry"_n,  const_mem_fun<deal_t, uint64_t, &deal_t::by_expired_at> >
    > idx_t;

    EOSLIB_SERIALIZE(deal_t,    (id)(order_id)(order_price)(order_price_usd)(deal_quantity)
                                (order_maker)(maker_passed)(maker_passed_at)
                                (order_taker)(taker_passed)(taker_passed_at)
                                (arbiter)(arbiter_passed)(arbiter_passed_at)
                                (closed)(created_at)(closed_at)(order_sn)(pay_type)
                                (expired_at)(maker_expired_at)
                                (restart_taker_num)(restart_maker_num))
};

struct CONTRACT_TBL merchant_t {
    name owner;
    asset available_quantity = asset(0, SYS_SYMBOL);
    set<uint8_t> accepted_payments; //accepted payments
    uint32_t processed_deals = 0;
    string email;
    string memo;

    merchant_t() {}
    merchant_t(const name& o): owner(o) {}

    uint64_t primary_key()const { return owner.value; }
    uint64_t scope()const { return 0; }

    typedef eosio::multi_index<"merchants"_n, merchant_t> idx_t;

    EOSLIB_SERIALIZE(merchant_t,  (owner)(available_quantity)(accepted_payments)
                                (processed_deals)(email)(memo) )
};

/**
 * 交易订单过期时间
 *
 */
struct CONTRACT_TBL deal_expiry_t{
    uint64_t deal_id;
    time_point_sec expired_at;

    deal_expiry_t() {}
    deal_expiry_t(uint64_t i): deal_id(i) {}

    uint64_t primary_key()const { return deal_id; }
    uint64_t scope()const { return 0; }

    uint64_t by_expired_at() const    { return uint64_t(expired_at.sec_since_epoch()); }

    EOSLIB_SERIALIZE(deal_expiry_t,  (deal_id)(expired_at) )
};

typedef eosio::multi_index
    <"dealexpiries"_n, deal_expiry_t ,
        indexed_by<"expiry"_n,    const_mem_fun<deal_expiry_t, uint64_t, &deal_expiry_t::by_expired_at>   >
    > deal_expiry_tbl;

} // MGP
