#pragma once

#include "wasm_db.hpp"

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

using namespace eosio;
using namespace std;
using std::string;

using namespace wasm;
#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

static constexpr uint32_t MAX_CONTENT_SIZE = 64;            // 4*0 behind 1
static constexpr uint16_t RATE_BOOST        = 10000;

enum class arbit_status_t: uint8_t {
    NONE        =0,
    UNARBITTED = 1,
    ARBITING   = 2,
    FINISHED   = 3
};

namespace wasm { namespace db {

#define SETTLE_TBL [[eosio::table, eosio::contract("otc.settle")]]
#define REWARD_TBL [[eosio::table, eosio::contract("otc.settle")]]
#define SETTLE_TBL_NAME(name) [[eosio::table(name), eosio::contract("otc.settle")]]

struct SETTLE_TBL_NAME("global") global_t {
    name admin;
    name market;
    vector<tuple<uint32_t, uint16_t, uint16_t>> level_config;
    EOSLIB_SERIALIZE( global_t, (admin)(level_config))
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

struct SETTLE_TBL settle_t {
    name        account;
    uint8_t     level = 0;
    uint32_t    sum_deal = 0;
    uint32_t    sum_fee = 0;
    uint16_t    sum_deal_count = 0;
    uint32_t    sum_arbit_count = 0;
    microseconds    sum_deal_time = microseconds(0);
    uint32_t    sum_child_deal = 0;
    
    settle_t() {}
    settle_t(const name& paccount): account(paccount) {}

    uint64_t primary_key() const { return account.value; }

    uint64_t scope() const { return 0; }

    typedef wasm::db::multi_index <"blacklist"_n, settle_t> idx_t;

    EOSLIB_SERIALIZE( settle_t, (account)(level)(sum_deal)(sum_deal_count)(sum_arbit_count)(sum_fee)
    (sum_child_deal)(sum_deal_time))
};
struct REWARD_TBL reward_t {
    uint64_t        id;
    name            reciptian;                     //land farmer
    asset           cash;
    asset           score;
    time_point_sec  created_at;                 //expire time (UTC time)

    reward_t() {}
    reward_t(const uint64_t& pid): id(pid) {}
    uint64_t primary_key() const { return id; }


    uint128_t by_reciptian() const { return (uint128_t)reciptian.value << 64 | (uint128_t)id; }

    typedef eosio::multi_index<"apples"_n, reward_t,
        indexed_by<"reciptianidx"_n,  const_mem_fun<reward_t, uint128_t, &reward_t::by_reciptian> >
    > idx_t;

    EOSLIB_SERIALIZE( reward_t, (id)(reciptian)(cash)(score)(created_at) )
};

}}