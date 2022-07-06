#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>
#include <otcconf/wasm_db.hpp>

using namespace eosio;
using namespace std;
using std::string;

using namespace wasm;
#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

static constexpr symbol CASH_SYMBOL              = SYMBOL("MUSDT", 6);

static constexpr uint32_t MAX_CONTENT_SIZE = 64;
static constexpr uint16_t RATE_BOOST        = 10000;

enum class arbit_status_t: uint8_t {
    NONE        =0,
    UNARBITTED = 1,
    ARBITING   = 2,
    FINISHED   = 3
};

namespace wasm { namespace db {

#define SETTLE_TBL [[eosio::table, eosio::contract("otcsettle")]]
#define SETTLE_TBL_NAME(name) [[eosio::table(name), eosio::contract("otcsettle")]]

struct SETTLE_TBL_NAME("global") gsettle_t {
    name conf_contract = "otcconf"_n;
    EOSLIB_SERIALIZE( gsettle_t, (conf_contract))
};
typedef eosio::singleton< "global"_n, gsettle_t > gsettle_singleton;

struct SETTLE_TBL settle_t {
    name        account;
    uint8_t     level = 0;
    uint64_t    sum_deal = 0;
    uint32_t    sum_fee = 0;
    uint16_t    sum_deal_count = 0;
    uint32_t    sum_arbit_count = 0;
    uint32_t    sum_deal_time = 0;
    uint64_t    sum_child_deal = 0;
    
    settle_t() {}
    settle_t(const name& paccount): account(paccount) {}

    uint64_t primary_key() const { return account.value; }

    uint64_t scope() const { return 0; }

    typedef wasm::db::multi_index <"settles"_n, settle_t> idx_t;

    EOSLIB_SERIALIZE( settle_t, (account)(level)(sum_deal)(sum_fee)(sum_deal_count)
    (sum_arbit_count)(sum_deal_time)(sum_child_deal))
};
struct SETTLE_TBL reward_t {
    uint64_t        id;
    uint64_t        deal_id;
    name            reciptian;                     //land farmer
    asset           cash;
    asset           score;
    time_point_sec  created_at;                 //expire time (UTC time)

    reward_t() {}
    reward_t(const uint64_t& pid): id(pid) {}
    uint64_t primary_key() const { return id; }

    uint128_t by_reciptian() const { return (uint128_t)reciptian.value << 64 | (uint128_t)id; }

    typedef eosio::multi_index<"rewards"_n, reward_t,
        indexed_by<"reciptianidx"_n,  const_mem_fun<reward_t, uint128_t, &reward_t::by_reciptian> >
    > idx_t;

    EOSLIB_SERIALIZE( reward_t, (id)(deal_id)(reciptian)(cash)(score)(created_at) )
};

}}