#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include "user_db.hpp"

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
    // enum status_list {anonymous=0, online=1, no_disturb=2, stealth=3};

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
    void create(const string& pubkey, const name& owner);

    [[eosio::action]]
    void update(const string& pubkey, const string& nickname, const uint16_t& status, const string& portrait, const name& owner);

    [[eosio::on_notify("amax.token::transfer")]]
    void top_up(name from, name to, asset quantity, string memo);

    [[eosio::action]]
    void destory(const name& owner);

    [[eosio::action]]
    void settopupconf(const bool& enable, const asset& topup_val, const name& contract_name, uint16_t days);


    using create_action = eosio::action_wrapper<"create"_n, &bibiuser::create>;
    using update_action = eosio::action_wrapper<"update"_n, &bibiuser::update>;
    using transfer_action = eosio::action_wrapper<"transfer"_n, &bibiuser::transfer>;
    using destory_action = eosio::action_wrapper<"destory"_n, &bibiuser::destory>;
    using settopupconf_action = eosio::action_wrapper<"settopupconf"_n, &bibiuser::settopupconf>;

};