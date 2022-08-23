
#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <eosio/name.hpp>

#include "utils.hpp"

using namespace eosio;

#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

namespace wasm { namespace db {

using namespace std;
using namespace eosio;
using namespace wasm;

#define CUSTODY_TBL [[eosio::table, eosio::contract("aplink.event")]]
namespace status {
    static constexpr eosio::name ACTIVE         = "active"_n;
    static constexpr eosio::name INACTIVE       = "inactive"_n;
};

struct [[eosio::table("global"), eosio::contract("aplink.event")]] global_t {
    name            admin                       = "aplink.admin"_n;
    name            fee_collector               = "amax.daodev"_n;
    asset           event_cpm                   = asset_from_string("0.10000000 AMAX");    //cost per mille/thousand
    name            status                      = status::ACTIVE;           // active | inactive

    global_t() {}

    EOSLIB_SERIALIZE( global_t, (admin)(fee_collector)(event_cpm)(status) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

struct dapp_info_t {
    name            dapp_contract;
    string          dapp_title;
    string          logo_url;
    string          invoke_url;
};

struct CUSTODY_TBL dapp_t {
    name            dapp_contract;
    uint64_t        available_notify_times      = 0;
    uint64_t        used_notify_times           = 0;
    name            status                      = status::ACTIVE;
    time_point_sec  registered_at;

    uint64_t        primary_key()const { return dapp_contract.value; }
    uint64_t        scope() const { return 0; }

    dapp_t() {}
    dapp_t(const name& c): dapp_contract(c) {}

    EOSLIB_SERIALIZE( dapp_t, (dapp_contract)(available_notify_times)(used_notify_times)(status)(registered_at) )


    typedef eosio::multi_index<"dapps"_n, dapp_t > idx_t;
};

} } //wasm