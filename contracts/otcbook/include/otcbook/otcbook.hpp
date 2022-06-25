#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <eosio/action.hpp>
#include <string>

#include <otcconf/otcconf_states.hpp>
#include <otcconf/wasm_db.hpp>
#include "otcbook_states.hpp"

using namespace wasm::db;
using namespace otc;

namespace metabalance {

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

#define AMA_LOG( debug, exception, ... ) {  \
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
    using conf_t = otc::global_t;
    using conf_table_t = otc::global_singleton;

private:
    dbc                 _dbc;
    global_singleton    _global;
    global_t            _gstate;
    std::unique_ptr<conf_table_t> _conf_tbl_ptr;
    std::unique_ptr<conf_t> _conf_ptr;

public:
    using contract::contract;
    otcbook(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        _dbc(_self), contract(receiver, code, ds),
        _global(_self, _self.value)/*, _global2(_self, _self.value) */
    {
        if (_global.exists()) {
            _gstate = _global.get();
        } else { // first init
            _gstate = global_t{};
        }
        // _gstate2 = _global2.exists() ? _global2.get() : global2_t{};
    }

    ~otcbook() {
        _global.set( _gstate, get_self() );
        // _global2.set( _gstate2, get_self() );
    }

    /**
     * set conf contract by admin
     * @param conf_contract conf contract
     * @note require admin auth
     */
    [[eosio::action]]
    void setconf(const name &conf_contract);

    /**
     * set merchant
     * @param owner account name
     * @param merchant account name
     * @param merchant_name merchant's name
     * @param merchant_detail merchant's detail
     * @param pay_methods pay methods
     * @param email email of merchant
     * @param memo memo of merchant
     * @note require owner auth
     */
    [[eosio::action]]
    void setmerchant(const name& owner, const name& merchant, const string &merchant_name, const string &merchant_detail, const string& email, const string& memo);

    /**
     * enable merchant by admin
     * @param owner merchant account name
     * @param is_enabled enable(true) or disable(false) merchant
     * @note require admin auth
     */
    [[eosio::action]]
    void enbmerchant(const name& owner, bool is_enabled);

    /**
     * open order by merchant
     * @param owner merchant account name
     * @param order_side order side, buy | sell
     * @param va_quantity  va quantity for buy|sell, (ex. "1.0000 CNYD")
     * @param va_price va price base on fiat, (ex. "1.0000 CNY")
     * @param va_min_take_quantity min take quantity for taker
     * @param memo memo of order
     * @note require owner auth
     */
    [[eosio::action]]
    void openorder(const name& owner, const name& order_side,const set<name> &pay_methods, const asset& va_quantity, const asset& va_price,
        const asset& va_min_take_quantity, const asset& va_max_take_quantity, const string &memo);


    /**
     * pause order by merchant
     * all of the related deals must be closed
     * @param owner merchant account name
     * @param order_id order id, created in openorder()
     * @note require owner auth
     */
    [[eosio::action]]
    void pauseorder(const name& owner, const name& order_side, const uint64_t& order_id);

    /**
     * resume order by merchant
     * all of the related deals must be closed
     * @param owner merchant account name
     * @param order_id order id, created in openorder()
     * @note require owner auth
     */
    [[eosio::action]]
    void resumeorder(const name& owner, const name& order_side, const uint64_t& order_id);


    /**
     * close order by merchant
     * all of the related deals must be closed
     * @param owner merchant account name
     * @param order_id order id, created in openorder()
     * @note require owner auth
     */
    [[eosio::action]]
    void closeorder(const name& owner, const name& order_side, const uint64_t& order_id);

    /**
     * open deal by user
     * @param taker user account name
     * @param order_id order id, created in openorder()
     * @param deal_quantity deal quantity of va
     * @param order_sn order_sn should be unique to locate current deal
     * @param session_msg session msg(message)
     * @note require taker auth
     */
    [[eosio::action]]
    void opendeal(const name& taker, const name& order_side, const uint64_t& order_id,
        const asset& deal_quantity, const uint64_t& order_sn,
        const string& session_msg);

    /**
     * close deal
     * merchat/user can close deal when status in [CREATED | MAKER_RECV_AND_SENT]
     * admin can close deal in any status except [CLOSE]
     * @param account account name
     * @param account_type account type, admin(1) | merchant(2) | user(3)
     * @param deal_id deal_id, created by opendeal()
     * @param session_msg session msg(message)
     * @note require account auth
     */
    [[eosio::action]]
    void closedeal(const name& account, const uint8_t& account_type, const uint64_t& deal_id, const string& session_msg);

    /**
     * close deal
     * merchat/user can close deal when status in [CREATED | MAKER_RECV_AND_SENT]
     * admin can close deal in any status except [CLOSE]
     * @param account account name
     * @param account_type account type, admin(1) | merchant(2) | user(3)
     * @param deal_id deal_id, created by opendeal()
     * @param session_msg session msg(message)
     * @param is_taker_black is taker black,  if true, and status is MAKER_ACCEPTED, and account is maker: add taker to blacklist
     * @note require account auth
     */
    [[eosio::action]]
    void canceldeal(const name& account, const uint8_t& account_type, const uint64_t& deal_id, const string& session_msg, bool is_taker_black);


    /**
     * process deal
     * @param account account name
     * @param account_type account type, merchant(2) | user(3)
     * @param deal_id deal_id, created by opendeal()
     * @param action deal action
     * @param session_msg session msg(message)
     * @note require account auth
     */
    [[eosio::action]]
    void processdeal(const name& account, const uint8_t& account_type, const uint64_t& deal_id,
        uint8_t action, const string& session_msg);


    /**
     * user or merchant start arbit request
     * @param account account name
     * @param account_type account type, merchant(2) | user(3)
     * @param deal_id deal_id, created by opendeal()
     * @param arbiter arbiter's name
     * @param session_msg session msg(message)
     * @note require account auth
     */
     [[eosio::action]]
    void startarbit(const name& account, const uint8_t& account_type, const uint64_t& deal_id,
        const name& arbiter, const string& session_msg);


    /**
     * arbiter close arbit request
     * @param account account name
     * @param account_type account type, merchant(2) | user(3)
     * @param deal_id deal_id, created by opendeal()
     * @param arbit_result 0:session
     * @param session_msg session msg(message)
     * @note require account auth
     */
     [[eosio::action]]
    void closearbit(const name& account, const uint64_t& deal_id, const uint8_t& arbit_result, const string& session_msg);

    /**
     * arbiter close arbit request
     * @param account account name
     * @param account_type account type, merchant(2) | user(3)
     * @param deal_id deal_id, created by opendeal()
     * @param arbit_result 0:session
     * @param session_msg session msg(message)
     * @note require account auth
     */
    [[eosio::action]]
    void cancelarbit( const uint8_t& account_type, const name& account, const uint64_t& deal_id, const string& session_msg );
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
    [[eosio::on_notify("*::transfer")]]
    void deposit(name from, name to, asset quantity, string memo);

    /**
     * withdraw
     * @param owner owner account, only support merchant to withdraw
     * @param quantity withdraw quantity
     * @note require owner auth
     */
    [[eosio::action]]
    void withdraw(const name& owner, asset quantity);

    /**
     * reversedeal
     * @param account account, must be admin
     * @param deal_id deal_id, created by opendeal()
     * @param session_msg session msg(message)
     * @note require account auth
     */
    [[eosio::action]]
    void resetdeal(const name& account, const uint64_t& deal_id, const string& session_msg);

    /**
     * set blacklist for opendeal()
     *
     * @param account account, must be admin
     * @param duration_second duration second
     * @note require admin auth
     */
    [[eosio::action]]
    void setblacklist(const name& account, uint64_t duration_second);

    // [[eosio::action]]
    // void timeoutdeal();

    [[eosio::action]]
    void stakechanged(const name& account, const asset &quantity, const string& memo);

    [[eosio::action]]
    void notification(const name& account, const otc::AppInfo_t &info, const string& memo);
    
    using stakechanged_action = eosio::action_wrapper<"stakechanged"_n, &otcbook::stakechanged>;
    using notification_action = eosio::action_wrapper<"notification"_n, &otcbook::notification>;

private:
    asset _calc_order_stakes(const asset &quantity);

    asset _calc_deal_fee(const asset &quantity);

    asset _calc_deal_amount(const asset &quantity);

    const conf_t& _conf(bool refresh = false);

    void _set_blacklist(const name& account, uint64_t duration_second, const name& payer);

    void _add_balance(merchant_t& merchant, const asset& quantity, const string & memo);
    void _sub_balance(merchant_t& merchant, const asset& quantity, const string & memo);
    void _frozen(merchant_t& merchant, const asset& quantity);
    void _unfrozen(merchant_t& merchant, const asset& quantityl);
};

}
