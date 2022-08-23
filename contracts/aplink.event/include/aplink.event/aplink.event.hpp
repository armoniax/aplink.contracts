#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>

#include "aplink.event.db.hpp"
#include "utils.hpp"

#include "wasm_db.hpp"

using namespace eosio;
using namespace wasm::db;

static constexpr name APL_BANK   = "aplink.token"_n;
static constexpr symbol   APL    = symbol(symbol_code("APL"), 4);

static constexpr symbol   SYS_SYMBOL            = symbol(symbol_code("AMAX"), 8);

// static constexpr aplink_token_contract = "aplink.token"_n;


#define CHECKC(exp, code, msg) \
   { if (!(exp)) eosio::check(false, string("[[") + to_string((int)code) + string("]] ") + msg); }
   
enum class err: uint8_t {
   NONE                 = 0,
   TIME_INVALID         = 1,
   RECORD_EXISTING      = 2,
   RECORD_NOT_FOUND     = 3,
   SYMBOL_MISMATCH      = 4,
   PARAM_ERROR          = 5,
   PAUSED               = 6,
   NO_AUTH              = 7,
   NOT_POSITIVE         = 8,
   NOT_STARTED          = 9,
   OVERSIZED            = 10,
   TIME_EXPIRED         = 11,
   ZERO_NOTIFY_TIMES    = 12,
   ACTION_REDUNDANT     = 13,
   ACCOUNT_INVALID      = 14,
   CONTENT_LENGTH_INVALID = 15,
   NOT_DISABLED          = 16,

};

class [[eosio::contract("aplink.event")]] event_center : public contract {
private:
    global_t            _gstate;
    global_singleton    _global;
    dbc                 _db;

public:
    using contract::contract;

    event_center(name receiver, name code, datastream<const char*> ds):
                 _db(_self), contract(receiver, code, ds), _global(_self, _self.value) {

        if (_global.exists()) {
            _gstate = _global.get();

        } else {
            _gstate = global_t{};
        }
    }

    ~event_center() {
        _global.set( _gstate, get_self() );
    }

    ACTION init();
    ACTION seteventcpm(const asset& event_cpm);
    ACTION registerdapp(const name& dapp_contract);
    ACTION emitevent(const dapp_info_t& dapp_info, const name& recipient, const string& message);

    /**
     * @brief topup apples for a land
     * @param from 
     * @param to 
     * @param quantity 
     * @param memo  land_id
     */
    [[eosio::on_notify("amax.token::transfer")]]
    void ontransfer(const name& from, const name& to, const asset& quantity, const string& memo);

};
