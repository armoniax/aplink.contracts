/*
 * @Author: your name
 * @Date: 2022-04-13 15:58:25
 * @LastEditTime: 2022-04-14 15:40:31
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: /deotc.contracts/contracts/otcconf/include/otcconf/otcconf.hpp
 */
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

    [[eosio::action]]
    void setarbiters(const set<name>& arbiters);

    [[eosio::action]]
    void setotcname(const name& otc_name);

private:

};

}
