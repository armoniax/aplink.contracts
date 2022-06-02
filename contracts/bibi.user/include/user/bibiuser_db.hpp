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

namespace wasm { namespace db {

using namespace std;
using namespace eosio;
using namespace wasm;

#define USER_TBL [[eosio::table, eosio::contract("bibi.user")]]

struct [[eosio::table("global"), eosio::contract("bibi.user")]] global_t {            
    asset               fee;  
    bool                enable = false;
    uint16_t            effective_days = 0;
    global_t() {}

    EOSLIB_SERIALIZE( global_t, (fee)(enable)(effective_days) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

struct USER_TBL chatuser {
    name        owner;
    uint16_t    status;
    string      portrait;
    string      pubkey;
    string      nickname;
    time_point  vip_ex_time;
    bool        enable;

    uint64_t    primary_key()const { return owner.value; }
    uint64_t    scope() const { return 0; }

    chatuser() {}
    chatuser(const name& c): owner(c) {}

    EOSLIB_SERIALIZE( chatuser, (owner)(status)(portrait)(pubkey)(nickname)(vip_ex_time)(enable) )

    typedef eosio::multi_index<"chatusers"_n, chatuser > tbl_t;
};

} } //wasm