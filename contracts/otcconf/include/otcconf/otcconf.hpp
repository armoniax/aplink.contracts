#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <eosio/action.hpp>
#include <string>

#include "wasm_db.hpp"
#include "otcconf_states.hpp"

using namespace wasm::db;

namespace otc {

using eosio::asset;
using eosio::check;
using eosio::datastream;
using eosio::name;
using eosio::symbol;
using eosio::symbol_code;
using eosio::unsigned_int;

using std::string;

class [[eosio::contract("otcconf")]] otcconf: public eosio::contract {
private:
    global_singleton    _global;
    global_t            _gstate;
    
public:
    using contract::contract;
    otcconf(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        contract(receiver, code, ds), _global(_self, _self.value) {
    }

    ~otcconf() {
        _global.set( _gstate, get_self() );
    }

    /**
     * reset the global with default values
     * only code maintainer can init
     */
    [[eosio::action]] 
    void init();

    [[eosio::action]]
    void setrate(const map<symbol, asset>& prices_quote_cny);
private:
};

}
