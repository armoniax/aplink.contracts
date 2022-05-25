#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>

#include "aplink.newbie.db.hpp"
// #include "wasm_db.hpp"

using namespace eosio;
using namespace wasm::db;

class [[eosio::contract("aplink.newbie")]] newbie : public contract {
private:
    global_t            _gstate;
    global_singleton    _global;

public:
    using contract::contract;

    newbie(name receiver, name code, datastream<const char*> ds):contract(receiver, code, ds), _global(_self, _self.value) {
        if (_global.exists()) {
            _gstate = _global.get();

        } else {
            _gstate = global_t{};
        }
    }

    ~newbie() {
        _global.set( _gstate, get_self() );
    }

    [[eosio::action]]
    void claimreward(const name& newbie);

    [[eosio::action]]
    void setstate(const bool& enable, const asset& newbie_reward, const name& contract_name);

    /**
     * recycle db
     * require contract self auth
     * @param max_rows - max rows to recycle
     */
    // [[eosio::action]]
    // void recycledb(uint32_t max_rows);

    // using claimreward_action = eosio::action_wrapper<"claimreward"_n, &newbie::claimreward>;
    // using setstate_action = eosio::action_wrapper<"setstate"_n, &newbie::setstate>;

};