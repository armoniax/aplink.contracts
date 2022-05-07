#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include "bibiuser_db.hpp"

#include <string>
#include <vector>
#include <deque>
#include <optional>
#include <type_traits>

using std::string;
using std::vector;

using namespace eosio;
using namespace wasm::db;

class [[eosio::contract("bibiuser")]] bibiuser : public contract {
private:
    dbc                 _db;
    global_t            _gstate;
    global_singleton    _global;

public:
    using contract::contract;

    bibiuser(name receiver, name code, datastream<const char*> ds):_db(_self), contract(receiver, code, ds), _global(_self, _self.value) {
        if (_global.exists()) {
            _gstate = _global.get();

        } else {
            _gstate = global_t{};
        }
    }

    ~bibiuser() {
        _global.set( _gstate, get_self() );
    }

    [[eosio::action]]
    void create(const name& owner,const string& pubkey);

    [[eosio::action]]
    void update(const name& owner,const string& pubkey, const string& nickname, const uint16_t& status, const string& portrait);

    // [[eosio::on_notify("amax.token::transfer")]]
    [[eosio::on_notify("aplink::transfer")]]
    void fee(name from, name to, asset quantity, string memo);

    [[eosio::action]]
    void destory(const name& owner);

    [[eosio::action]]
    void setfeeconf(const bool& enable, const asset& fee, uint16_t days);

    [[eosio::action]]
    void gmuserstat(const name& owner, const bool& enable);

    [[eosio::action]]
    void gmdestory(const name& owner);

    using create_action = eosio::action_wrapper<"create"_n, &bibiuser::create>;
    using update_action = eosio::action_wrapper<"update"_n, &bibiuser::update>;
    using fee_action = eosio::action_wrapper<"fee"_n, &bibiuser::fee>;
    using destory_action = eosio::action_wrapper<"destory"_n, &bibiuser::destory>;
    using setfeeconf_action = eosio::action_wrapper<"setfeeconf"_n, &bibiuser::setfeeconf>;
    using gmuserstat_action = eosio::action_wrapper<"gmuserstat"_n, &bibiuser::gmuserstat>;
    using gmdestory_action = eosio::action_wrapper<"gmdestory"_n, &bibiuser::gmdestory>;

};