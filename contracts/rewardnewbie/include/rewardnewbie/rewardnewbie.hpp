#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include "rewardnewbie_db.hpp"

#include <string>
#include <vector>
#include <deque>
#include <optional>
#include <type_traits>

using std::string;
using std::vector;

using namespace eosio;
using namespace wasm::db;

class [[eosio::contract("rewardnewbie")]] rewardnewbie : public contract {
private:
    dbc                 _db;
    global_t            _gstate;
    global_singleton    _global;

public:
    using contract::contract;

    rewardnewbie(name receiver, name code, datastream<const char*> ds):_db(_self), contract(receiver, code, ds), _global(_self, _self.value) {
        if (_global.exists()) {
            _gstate = _global.get();

        } else {
            _gstate = global_t{};
        }
    }

    ~rewardnewbie() {
        _global.set( _gstate, get_self() );
    }

    [[eosio::action]]
    void claimreward(const name& newbie);

    [[eosio::action]]
    void setstate(const bool& enable, const asset& newbie_reward, const name& contract_name);

    using claimreward_action = eosio::action_wrapper<"claimreward"_n, &rewardnewbie::claimreward>;
    using setstate_action = eosio::action_wrapper<"setstate"_n, &rewardnewbie::setstate>;

};