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

static constexpr uint64_t percent_boost = 10000;

namespace wasm
{
    namespace db
    {

    #define ACCOUNT_TBL [[eosio::table, eosio::contract("otcswap")]]
    #define SWAP_TBL_NAME(name) [[eosio::table(name), eosio::contract("otcswap")]]
    
    struct SWAP_TBL_NAME("global") gswap_t
    {
        name conf_contract      = "otcconf"_n;
        EOSLIB_SERIALIZE(gswap_t, (conf_contract))
    };

    typedef eosio::singleton<"global"_n, gswap_t> gswap_singleton;

    struct ACCOUNT_TBL account_t
    {
        name admin;
        uint32_t balance=0;
        uint32_t sum=0;

        uint64_t primary_key() const { return admin.value; }

        account_t() {}
        account_t(const name &paccount) : admin(paccount) {}


        typedef wasm::db::multi_index<"accounts"_n, account_t> idx_t;

        EOSLIB_SERIALIZE(account_t, (admin)(balance)(sum))
    };
}
}
