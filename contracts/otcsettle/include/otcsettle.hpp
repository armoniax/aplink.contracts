#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <otcconf/otcconf_states.hpp>

#include <string>
#include "otcsettledb.hpp"

using namespace eosio;
using namespace wasm::db;
using std::string;

#define SETTLE_DEAL(bank, deal_id, merchant, user, quantity, fee, arbit_status, start_at, end_at) \
    {	otcsettle::settle::deal_action act{ bank, { {_self, active_permission} } };\
			act.send( deal_id, merchant, user, quantity, fee, arbit_status, start_at, end_at );}

namespace otcsettle {
class [[eosio::contract("otcsettle")]] settle : public contract { 
       
using conf_t = otc::global_t;
using conf_table_t = otc::global_singleton;

private:
    gsettle_singleton    _global;
    gsettle_t            _gstate;
    dbc           _db;
    
    std::unique_ptr<conf_table_t> _conf_tbl_ptr;
    std::unique_ptr<conf_t> _conf_ptr;

    const conf_t& _conf(bool refresh = false);

public:
    using contract::contract;
    settle(name receiver, name code, datastream<const char*> ds):
        _db(_self), contract(receiver, code, ds),
        _global(get_self(), get_self().value)
    {
        _gstate = _global.exists() ? _global.get() : gsettle_t{};
    }

    ~settle() { _global.set(_gstate, get_self()); }

    [[eosio::action]]
    void setconf(const name &conf_contract);

    [[eosio::action]]
    void setlevel(const name& user, uint8_t level);

    [[eosio::action]]
    void deal(const uint64_t& deal_id,
                const name& merchant, 
                const name& user, 
                const asset& quantity, 
                const asset& fee,
                const uint8_t& arbit_status, 
                const time_point_sec& start_at, 
                const time_point_sec& end_at);

    /**
     * @brief pick rewards
     * 
     * @param reciptian 
     * @param rewards reward_id array, support lessthan 20 rewards
     */
    [[eosio::action]]
    void pick(const name& reciptian, vector<uint64_t> rewards);

    using deal_action = eosio::action_wrapper<"deal"_n, &settle::deal>;
};
}