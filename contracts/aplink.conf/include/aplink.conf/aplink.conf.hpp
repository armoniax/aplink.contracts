#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <string>
#include <vector>

namespace aplink {

using std::string;
using std::vector;
using std::pair;
using namespace eosio;

static constexpr symbol   SYS_SYMBOL            = symbol(symbol_code("AMAX"), 8);
static constexpr symbol   APL_SYMBOL            = symbol(symbol_code("APL"), 4);
static constexpr symbol   CNYD_SYMBOL           = symbol(symbol_code("CNYD"), 4);

struct account_res {
    uint8_t account_create_ram_bytes            = (uint8_t) 4096;
    asset account_stake_cpu                     = asset(200000, SYS_SYMBOL);
    asset account_stake_net                     = asset(200000, SYS_SYMBOL);
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
    ACTION setacctres(const account_res& account_create_res);
    ACTION setprices(const vector<pair<symbol_code, asset>> prices);

private:
    struct [[eosio::table("global"), eosio::contract("aplink.conf")]] global_t {
        name admin;     //default: _self
        account_res account_create_res;
        vector<string> amc_nodes =  {
            "amc1.nchain.me:8888",
            "amc2.nchain.me:8888",
        };
        bool show_price = false;
        
        EOSLIB_SERIALIZE( global_t, (admin)(account_create_res)(amc_nodes)(show_price) )
    };
    typedef eosio::singleton< "global"_n, global_t > global_singleton;

    struct [[eosio::table("prices"), eosio::contract("aplink.conf")]] price_t {
        symbol_code         symb;
        asset               price;
        time_point_sec      updated_at;

        uint64_t    primary_key()const { return symb.raw(); }

        price_t() {}
        price_t(const symbol_code& sc, const asset& p): symb(sc), price(p) { updated_at = time_point_sec(current_time_point()); }

        EOSLIB_SERIALIZE( price_t, (symb)(price)(updated_at) )

        typedef eosio::multi_index<"prices"_n, price_t > tbl_t;
    };

    global_singleton    _global;
    global_t            _gstate;
};
} //namespace apollo
