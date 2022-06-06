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
static constexpr uint64_t percent_boost = 10000;

static constexpr symbol SCORE_SYMBOL = SYMBOL("METAS", 4);

static constexpr symbol CNYD_SYMBOL = SYMBOL("CNYD", 4);
static constexpr eosio::name CNYD_BANK{"cnyd.token"_n};

namespace wasm
{
    namespace db
    {

#define ACCOUNT_TBL [[eosio::table, eosio::contract("otcswap")]]
#define SETTLE_TBL_NAME(name) [[eosio::table(name), eosio::contract("otcswap")]]
    
    struct balance_config {
        uint64_t quantity_limit;
        uint16_t balance_precent;
    };
    struct SETTLE_TBL_NAME("global") global_t
    {
        name admin;
        name settle_contract;
        vector<balance_config> fee_rates = {
            {0, 1500}, {130000000, 2500}, {500000000, 3500}, {1500000000, 5000}};

        EOSLIB_SERIALIZE(global_t, (admin)(settle_contract)(fee_rates))
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


        typedef wasm::db::multi_index<"accounts"_n, account_t> idx_t;

        EOSLIB_SERIALIZE(account_t, (admin)(balance)(sum))
    };
}
}
