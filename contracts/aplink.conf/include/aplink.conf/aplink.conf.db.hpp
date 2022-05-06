 #pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include <deque>
#include <optional>
#include <string>
#include <map>
#include <set>
#include <type_traits>

namespace aplink {

using namespace std;
using namespace eosio;

#define TBL struct [[eosio::table, eosio::contract("aplink.conf")]]

struct aplink_settings {
    auto account_stake_cpu = ASSET("0.002 AMAX");
    auto account_stake_net = ASSET("0.002 AMAX");
    auto mainnet_nodes = vector<string> {
        "a1.nchain.me:8888",
        "a2.nchain.me:8888",
    };
    auto testnet_nodes = vector<string> {
        "t1.nchain.me:18888",
        "t2.nchain.me:18888",
    };

};

struct [[eosio::table("global"), eosio::contract("aplink.conf")]] global_t {
    name admin;                 // default is contract self
    aplink_settings settings;         // mgmt fees to collector

    EOSLIB_SERIALIZE( global_t, (admin)(settings) )
};

typedef eosio::singleton< "global"_n, global_t > global_singleton;


} // aplink
