
#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <eosio/name.hpp>

#include "utils.hpp"

using namespace eosio;

#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

// static constexpr name active_perm               {"active"_n};
// static constexpr symbol APL_SYMBOL              = SYMBOL("APL", 4);
// static constexpr name   APL_BANK                { "aplink.token"_n };   //NTT token

namespace wasm { namespace db {

using namespace std;
using namespace eosio;
using namespace wasm;

#define CUSTODY_TBL [[eosio::table, eosio::contract("aplink.newbie")]]

struct aplink_farm {
    name contract = "aplink.farm"_n;
    uint64_t land_id;
    asset parent_inviter_reward = asset_from_string("100 APL");
    asset grandparent_inviter_reward = asset_from_string("100 APL");
};

struct [[eosio::table("global"), eosio::contract("aplink.newbie")]] global_t {
    asset               newbie_reward;  //"100.0000 APL"
    name                aplink_token_contract;
    aplink_farm         apl_farm;
    bool                enable = false;

    global_t() {}

    EOSLIB_SERIALIZE( global_t, (newbie_reward)(aplink_token_contract)(apl_farm)(enable) )
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