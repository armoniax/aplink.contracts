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
    string manufacturer_cn;
    string manufacturer_en;
    string mine_coin_symbol;                        //BTC, ETH
    float hashrate;                                 //E.g. 21.457 MH/s
    char hashrate_unit;                             //M, T
    float power_in_watt;                            //E.g. 2100 Watt
    asset daily_electricity_charge;                 //E.g. "0.85 CNYD"
    asset price;                                    //E.g. "10000 CNYD"
    asset daily_earing_base;                        //E.g. "0.00397002 AMETH"
    asset last_recd_earning;
    asset total_recd_earing;
    uint16_t shell_life_days;                       //running time (E.g. 3*365)
    uint8_t onshelf_days;                           //0: T+0, 1:T+1
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

    typedef eosio::multi_index
    < "mineproducts"_n,  order_t,
        indexed_by<"price"_n, const_mem_fun<order_t, uint64_t, &order_t::by_invprice> >,
        indexed_by<"maker"_n, const_mem_fun<order_t, uint128_t, &order_t::by_maker_status> >,
        indexed_by<"coin"_n, const_mem_fun<order_t, uint128_t, &order_t::by_coin> >,
        indexed_by<"updatedat"_n, const_mem_fun<order_t, uint64_t, &order_t::by_update_time> >
    > tbl_t;

};


} // apollo
