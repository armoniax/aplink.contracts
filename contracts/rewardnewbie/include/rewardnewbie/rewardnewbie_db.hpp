   
#pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>
#include <eosio/name.hpp>

#include "wasm_db.hpp"
#include "utils.hpp"

using namespace eosio;

#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

static constexpr name active_perm               {"active"_n};
static constexpr symbol APL_SYMBOL              = SYMBOL("APL", 4);
static constexpr name   APL_BANK                { "aplink.token"_n };   //NTT token

namespace wasm { namespace db {

using namespace std;
using namespace eosio;
using namespace wasm;

#define CUSTODY_TBL [[eosio::table, eosio::contract("rewardnewbie")]]

struct [[eosio::table("global"), eosio::contract("rewardnewbie")]] global_t {            
    asset               newbie_reward;  //"100.0000 APL"
    bool                enable = false;

    global_t() {}

    EOSLIB_SERIALIZE( global_t, (newbie_reward)(enable) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

struct CUSTODY_TBL claim_t {
    name        claimer;
    time_point  claimed_at;
    
    uint64_t    primary_key()const { return claimer.value; }
    uint64_t    scope() const { return 0; }

    claim_t() {}
    claim_t(const name& c): claimer(c) {}

    EOSLIB_SERIALIZE( claim_t, (claimer)(claimed_at) )

    typedef eosio::multi_index<"claims"_n, claim_t > tbl_t;
};

} } //wasm