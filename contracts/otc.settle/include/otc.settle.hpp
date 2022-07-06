#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>
#include "otc.settledb.hpp"

using namespace eosio;
using namespace wasm::db;



namespace otc {

static constexpr name SCORE_BANK                  { "oxo.token"_n };
static constexpr name CASH_BANK                  { "cnyd.token"_n };
static constexpr name APLINK_FARM                  { "aplinkfarm13"_n };
static constexpr symbol APLINK_SYMBOL              = SYMBOL("APL", 4);
static constexpr uint64_t APLINK_FARM_LAND              = 1;

#define CHECKC(exp, code, msg) \
   { if (!(exp)) eosio::check(false, string("$$$") + to_string((int)code) + string("$$$ ") + msg); }
   
enum class err: uint8_t {
   NONE                 = 0,
   RECORD_NOT_FOUND     = 1,
   RECORD_EXISTING      = 2,
   SYMBOL_MISMATCH      = 4,
   PARAM_ERROR          = 5,
   PAUSED               = 6,
   NO_AUTH              = 7,
   NOT_POSITIVE         = 8,
   NOT_STARTED          = 9,
   OVERSIZED            = 10,
   TIME_EXPIRED         = 11,
   NOTIFY_UNRELATED     = 12,
   ACTION_REDUNDANT     = 13,
   ACCOUNT_INVALID      = 14,
   UN_INITIALIZE        = 16,
};

using std::string;

class [[eosio::contract("otc.settle")]] settle : public contract {    

public:
    using contract::contract;
    settle(name receiver, name code, datastream<const char*> ds):
        _db(_self), contract(receiver, code, ds),
        _global(get_self(), get_self().value)
    {
        _gstate = _global.exists() ? _global.get() : global_t{};
    }

    [[eosio::action]]
    void setadmin(const name& admin, const name& market, const name& swap);

    [[eosio::action]]
    void setlevel(const name& user, uint8_t level);

    [[eosio::action]]
    void setconfig(const vector<level_config>& configs);

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

private:
    global_singleton    _global;
    global_t            _gstate;
    dbc                 _db;

    void create_settle(const name& user);
};
}