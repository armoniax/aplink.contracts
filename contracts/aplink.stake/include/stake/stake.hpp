#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <eosio/action.hpp>
#include <string>

#include "wasm_db.hpp"
#include "stake_states.hpp"

using namespace wasm::db;

namespace amax {

using eosio::asset;
using eosio::check;
using eosio::datastream;
using eosio::name;
using eosio::symbol;
using eosio::symbol_code;
using eosio::unsigned_int;

using std::string;

static constexpr bool DEBUG = true;

#define WASM_FUNCTION_PRINT_LENGTH 50

#define AMAX_LOG( debug, exception, ... ) {  \
if ( debug ) {                               \
   std::string str = std::string(__FILE__); \
   str += std::string(":");                 \
   str += std::to_string(__LINE__);         \
   str += std::string(":[");                \
   str += std::string(__FUNCTION__);        \
   str += std::string("]");                 \
   while(str.size() <= WASM_FUNCTION_PRINT_LENGTH) str += std::string(" ");\
   eosio::print(str);                                                             \
   eosio::print( __VA_ARGS__ ); }}

class [[eosio::contract("stake")]] stake: public eosio::contract {
    using conf_t = otc::global_t;
    using conf_table_t = otc::global_singleton;
private:
    dbc                 _dbc;
    global_singleton    _global;
    global_t            _gstate;
    
public:
    using contract::contract;
    stake(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        _dbc(_self), contract(receiver, code, ds), 
        _global(_self, _self.value)/*, _global2(_self, _self.value) */
    {
        if (_global.exists()) {
            _gstate = _global.get();
        } else { // first init
            _gstate = global_t{};
            _gstate.admin = _self;
        }
    }

    ~stake() {
        _global.set( _gstate, get_self() );
    }

    /**
     * action trigger by transfer()
     * transfer token to this contract will trigger this action
     * only support merchant to deposit
     * @param from from account name
     * @param to must be this contract name
     * @param quantity transfer quantity
     * @param memo memo
     * @note require from auth
     */
    [[eosio::on_notify("eosio.token::transfer")]]
    void deposit(name from, name to, asset quantity, string memo);
  
    /**
     * withdraw
     * @param owner owner account, only support merchant to withdraw
     * @param quantity withdraw quantity
     * @note require owner auth
     */
    [[eosio::action]]
    void redeem(const name& owner);

    __attribute__((eosio_wasm_import))
    name get_account_creator(name accout) {
        return name(internal_use_do_not_use::get_account_creator(account.value))
    }
    
private:
    void _token_transfer(const name& to, const uint8_t& type, const asset &quantity);

    void _add_reward_log(const name& owner, const uint8_t& type, const asset &quantity);
};

}
