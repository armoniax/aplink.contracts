#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <eosio/action.hpp>
#include <string>

#include "wasm_db.hpp"
#include "otcbook_states.hpp"

using namespace wasm::db;

namespace mgp {

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

#define MGP_LOG( debug, exception, ... ) {  \
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

class [[eosio::contract("otcbook")]] otcbook: public eosio::contract {
private:
    dbc                 _dbc;
    global_singleton    _global;
    global_t            _gstate;
    global2_singleton   _global2;
    global2_t           _gstate2;
    
public:
    using contract::contract;
    otcbook(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        _dbc(_self), contract(receiver, code, ds), 
        _global(_self, _self.value), _global2(_self, _self.value)
    {
        _gstate = _global.exists() ? _global.get() : global_t{};
        _gstate2 = _global2.exists() ? _global2.get() : global2_t{};
    }

    ~otcbook() {
        _global.set( _gstate, get_self() );
        _global2.set( _gstate2, get_self() );
    }

    [[eosio::action]] //only code maintainer can init
    void init();

    [[eosio::action]]
    void setmerchant(const name& owner, const set<uint8_t>pay_methods, const string& email, const string& memo_to_buyer);
    
    [[eosio::action]]
    void setarbiter(const name& arbiter, const bool to_add); //true: to add; false: to remove
    
    /**
     * merchant to open sell order
     */
    [[eosio::action]]
    void openorder(const name& owner, uint8_t side, const asset& quantity, const asset& price, 
        const asset& min_accept_quantity, const string &memo);

    [[eosio::action]]
    void closeorder(const name& owner, const uint64_t& order_id);

    [[eosio::action]]
    void opendeal(const name& taker, const uint64_t& order_id, const asset& deal_quantity,const uint64_t& order_sn, 
        const string& memo);

    [[eosio::action]]
    void closedeal(const name& account, const uint8_t& account_type, const uint64_t& deal_id, const string& memo);
    
    /**
     *  @param: user_type -> 1: merchant, 2: user, 3: otc-arbiter
     *  @param: pass: 0: NO pass, 1: pass; two agreed passes means a decision! 
     */
    [[eosio::action]]
    void processdeal(const name& account, const uint8_t& account_type, const uint64_t& deal_id, 
        deal_action_t action, const string& memo);

    [[eosio::on_notify("eosio.token::transfer")]]
    void deposit(name from, name to, asset quantity, string memo);

    [[eosio::action]]
    void withdraw(const name& owner, asset quantity);

    [[eosio::action]]
    void timeoutdeal();

    [[eosio::action]]
    void deltable();

    [[eosio::action]]
    void reversedeal(const name& owner,const uint64_t& deal_id);

    [[eosio::action]]
    void restart(const name& owner,const uint64_t& deal_id,const uint8_t& user_type);

    [[eosio::action]]
    void setrate(const name& owner,const asset& mgp_price,const asset& usd_exchange_rate);

private:
    void _init();
    void _data_migrate();
};

inline vector <string> string_split(string str, char delimiter) {
      vector <string> r;
      string tmpstr;
      while (!str.empty()) {
          int ind = str.find_first_of(delimiter);
          if (ind == -1) {
              r.push_back(str);
              str.clear();
          } else {
              r.push_back(str.substr(0, ind));
              str = str.substr(ind + 1, str.size() - ind - 1);
          }
      }
      return r;

  }

}
