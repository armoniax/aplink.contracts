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
    dbc           _db;
    
public:
    using contract::contract;
    otcconf(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        _db(_self),
        contract(receiver, code, ds), _global(_self, _self.value) {
        _gstate = _global.exists() ? _global.get() : global_t{};
    }

    ~otcconf() {
        _global.set( _gstate, get_self() );
    }

    /**
     * reset the global with default values
     * only code maintainer can init
     */
    [[eosio::action]] 
    void init(const name& admin);

    /**
     * set running status by admin
     * @param status, 1 initialized, 2 running, 9 maintianing 
     * @note require contract admin auth
     */
    [[eosio::action]]
    void setstatus(const uint8_t& status);

    [[eosio::action]]
    void setmanager(const name& type, const name& account);

    [[eosio::action]]
    void setfarm(const name& farmname, const uint64_t& farm_id, const uint32_t& farm_scale);

    [[eosio::action]]
    void setappname(const name& otc_name);

    [[eosio::action]]
    void setsettlelv(const vector<settle_level_config>& configs);

    [[eosio::action]]
    void setswapstep(const vector<swap_step_config> rates);

    [[eosio::action]]
    void settimeout(const uint64_t& accepted_timeout, const uint64_t& payed_timeout);

private:

};

}
