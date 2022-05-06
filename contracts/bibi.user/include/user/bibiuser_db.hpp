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

namespace wasm { namespace db {

using namespace std;
using namespace eosio;
using namespace wasm;

#define USER_TBL [[eosio::table, eosio::contract("bibiuser")]]

struct [[eosio::table("global"), eosio::contract("bibiuser")]] global_t {            
    asset               topup_val;  
    name                contract_name;
    bool                enable = false;
    uint16_t            effective_days;
    global_t() {}

    EOSLIB_SERIALIZE( global_t, (topup_val)(contract_name)(enable)(effective_days) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

struct USER_TBL chatuser {
    name        owner;
    uint16_t    status;
    string      portrait;
    string      pubkey;
    string      nickname;
    time_point  vip_ex_time;
    bool        is_topup;
    bool        enable;
    uint64_t    primary_key()const { return owner.value; }
    uint64_t    scope() const { return 0; }

    chatuser() {}
    chatuser(const name& c): owner(c) {}

    EOSLIB_SERIALIZE( chatuser, (owner)(status)(portrait)(pubkey)(nickname)(vip_ex_time)(is_topup)(enable) )

    typedef eosio::multi_index<"chatusers"_n, chatuser > tbl_t;
};

} } //wasm