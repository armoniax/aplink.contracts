#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <string>
#include <vector>

namespace aplink {

using std::string;
using std::vector;
using namespace eosio;

static constexpr symbol   SYS_SYMBOL            = symbol(symbol_code("AMAX"), 8);

/**
 * The `aplink.conf` is configuration contract for APLink APP
 * 
 */
class [[eosio::contract("aplink.conf")]] settings : public contract {
public:
    using contract::contract;

    settings(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        contract(receiver, code, ds), _global(_self, _self.value) 
    {
        if (_global.exists()) {
            _gstate = _global.get();

        } else { // first init
            _gstate = global_t{};
            _gstate.admin = _self;
        }
    }

    ~settings() { _global.set( _gstate, get_self() ); }
   
    ACTION init(const aplink_settings& settings);
    using init_action = eosio::action_wrapper<"init"_n, &settings::init>;

private:
    struct aplink_settings {
        asset account_stake_cpu = asset(200000, SYS_SYMBOL);
        asset account_stake_net = asset(200000, SYS_SYMBOL);
        vector<string> mainnet_nodes =  {
            "a1.nchain.me:8888",
            "a2.nchain.me:8888",
        };
        vector<string> testnet_nodes = {
            "t1.nchain.me:18888",
            "t2.nchain.me:18888",
        };

    };

    struct [[eosio::table("global"), eosio::contract("aplink.conf")]] global_t {
        name admin = _self;
        aplink_settings settings;

        EOSLIB_SERIALIZE( global_t, (admin)(settings) )
    };

    typedef eosio::singleton< "global"_n, global_t > global_singleton;

    global_singleton    _global;
    global_t            _gstate;
};
} //namespace apollo
