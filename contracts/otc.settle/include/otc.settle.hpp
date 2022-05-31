#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>
#include "otc.settledb.hpp"

using namespace eosio;
using namespace wasm::db;



namespace otc {


static constexpr symbol SCORE_SYMBOL              = SYMBOL("METAS", 4);
static constexpr name SCORE_BANK                  { "oxo.token"_n };

static constexpr symbol CASH_SYMBOL              = SYMBOL("CNYD", 4);
static constexpr name CASH_BANK                  { "cnyd.token"_n };

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
   FEE_INSUFFICIENT     = 15,
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
        void setadmin(const name& admin, const name& market);

        [[eosio::action]]
        void setconfig(const vector<tuple<uint32_t, uint16_t, uint16_t>>& configs);

        [[eosio::action]]
        void deal(const name& merchant, 
                  const name& user, 
                  const asset& quantity, 
                  const asset& fee, 
                  const uint8_t& arbit_staus, 
                  const time_point_sec& start_at, 
                  const time_point_sec& end_at);

    /**
     * @brief pick rewards
     * 
     * @param croper 
     * @param rewards reward_id array, support lessthan 20 rewards
     */
    [[eosio::action]]
    void pick(const name& croper, vector<uint64_t> rewards);


    private:
        global_singleton    _global;
        global_t            _gstate;
        dbc                 _db;

        void create_settle(const name& user);
};
}