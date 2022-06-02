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

static constexpr uint32_t MAX_CONTENT_SIZE = 64; // 4*0 behind 1
static constexpr symbol CNYD_SYMBOL = SYMBOL("CNYD", 4);
static constexpr eosio::name CNYD_BANK{"cnyd.token"_n};
static constexpr uint64_t percent_boost = 10000;

namespace wasm
{
    namespace db
    {

#define ACCOUNT_TBL [[eosio::table, eosio::contract("otcswap")]]
#define SETTLE_TBL_NAME(name) [[eosio::table(name), eosio::contract("otcswap")]]

        struct SETTLE_TBL_NAME("global") global_t
        {
            name admin;
            name sellto_admin;
            vector<pair<uint64_t, double>> fee_rates = {
                {0, 0.15}, {130000000, 0.25}, {500000000, 0.35}, {1500000000, 0.5}};

            EOSLIB_SERIALIZE(global_t, (admin)(sellto_admin)(fee_rates))
        };
        typedef eosio::singleton<"global"_n, global_t> global_singleton;

        struct ACCOUNT_TBL account_t
        {
            name admin;
            uint32_t balance=0;
            uint32_t sum=0;

            uint64_t primary_key() const { return admin.value; }

            account_t() {}
            account_t(const name &paccount) : admin(paccount) {}


            typedef wasm::db::multi_index<"blacklist"_n, account_t> idx_t;

            EOSLIB_SERIALIZE(account_t, (admin)(balance)(sum))
        };
    }
}
