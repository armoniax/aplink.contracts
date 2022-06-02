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

struct account_res {
    uint8_t account_create_ram_bytes = (uint8_t) 4096;
    asset account_stake_cpu = asset(200000, SYS_SYMBOL);
    asset account_stake_net = asset(200000, SYS_SYMBOL);
};

struct waku_node_info {
    string wakunode_domain;
    uint16_t wakunode_rpc_port;
    uint16_t wakunode_ws_port;
    uint16_t wakunode_wss_port;

    EOSLIB_SERIALIZE(waku_node_info, (wakunode_domain)(wakunode_rpc_port)(wakunode_ws_port)(wakunode_wss_port) )
};


/**
 * The `aplink.conf` is configuration contract for APLink APP
 * 
 */
class [[eosio::contract("aplink.conf")]] settings : public contract {
public:
    using contract::contract;

    settings(eosio::name receiver, eosio::name code, datastream<const char*> ds): contract(receiver, code, ds), _global(_self, _self.value) 
    {
        if (_global.exists()) {
            _gstate = _global.get();

        } else { // first init
            _gstate = global_t{};
            _gstate.admin = _self;
        }
    }

    ~settings() { _global.set( _gstate, get_self() ); }
   
    ACTION init(const name& admin);
    using init_action = eosio::action_wrapper<"init"_n, &settings::init>;

    ACTION update(const account_res& account_create_res);
    using update_action = eosio::action_wrapper<"update"_n, &settings::update>;

private:
    struct [[eosio::table("global"), eosio::contract("aplink.conf")]] global_t {
        name admin;     //default: _self
        account_res account_create_res;
        vector<string> amc_nodes =  {
            "a1.nchain.me:8888",
            "a2.nchain.me:8888",
        };
      
        vector<waku_node_info> waku_nodes = {
            {
                "wakun1.acsiwang.com",
                8545,
                7000,
                7443
            }
        };
        
        bool show_price = false;
        
        EOSLIB_SERIALIZE( global_t, (admin)(account_create_res)(amc_nodes)(waku_nodes)(show_price) )
    };

    typedef eosio::singleton< "global"_n, global_t > global_singleton;

    global_singleton    _global;
    global_t            _gstate;
};
} //namespace apollo
