#include <eosio/permission.hpp>
#include "aplink.event.hpp"
#include "utils.hpp"
#include <aplink.token/aplink.token.hpp>

#include <amax.token/amax.token.hpp>


void event_center::init() {
    require_auth( _self );
  
}

void event_center::seteventcpm(const asset& event_cpm) {
    require_auth( _gstate.admin );

    CHECKC( event_cpm.symbol == SYS_SYMBOL,     err::SYMBOL_MISMATCH, "not AMAX symbol" )
    CHECKC( event_cpm.amount > 0,               err::NOT_POSITIVE, "not positive amount" )

    _gstate.event_cpm               = event_cpm;
}

void event_center::emitevent(const dapp_info_t& dapp_info, const name& recipient, const string& message)
{
    require_auth( dapp_info.dapp_contract );
    CHECKC( is_account( dapp_info.dapp_contract ), err::PARAM_ERROR, "invalid dapp contract" )
    CHECKC( is_account( recipient ), err::ACCOUNT_INVALID, "invalid recipient account" )
    
    auto dapp = dapp_t( dapp_info.dapp_contract );
    CHECKC( _db.get( dapp ),                    err::RECORD_NOT_FOUND, "dapp not registered" )
    CHECKC( dapp.available_notify_times > 0,    err::ZERO_NOTIFY_TIMES, "running out of notify times" )
    CHECKC( _gstate.status == status::ACTIVE,   err::INACTIVE, "event center not active" )
    CHECKC( dapp.status == status::ACTIVE,      err::INACTIVE, "dapp not active" )

    dapp.available_notify_times     -= 1;
    dapp.used_notify_times          += 1;
    _db.set( dapp );
}

void event_center::registerdapp(const name& dapp_contract) {
    require_auth( _gstate.admin );

    auto dapp                       = dapp_t( dapp_contract );
    CHECKC( !_db.get( dapp ), err::RECORD_EXISTING, "dapp already registered" )
    
    dapp.registered_at              = current_time_point();
    _db.set( dapp );
}

/**
 * @brief 
 * 
 * @param from 
 * @param to 
 * @param quantity 
 * @param memo： dapp:${dapp_name} 
 */
void event_center::ontransfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    if( to == _self ) {
        CHECKC( quantity.amount > 0, err::PARAM_ERROR, "non-positive quantity not allowed" )
        CHECKC( quantity.symbol == SYS_SYMBOL, err::PARAM_ERROR, "non-sys-symbol" )
        CHECKC( memo.size() > 4 && memo.size() <= 16, err::PARAM_ERROR, "memo format error!" )  

        vector<string_view> memo_params = split(memo, ":");
        ASSERT(memo_params.size() == 2);
        CHECKC( memo_params[0] == "dapp", err::PARAM_ERROR, "memo must start with `dapp`" )
        auto dapp_contract = name(memo_params[1]);
        CHECKC( is_account( dapp_contract ), err::PARAM_ERROR, "dapp contract in memo is not an account" )

        auto dapp = dapp_t( dapp_contract );
        CHECKC( _db.get( dapp ), err::RECORD_NOT_FOUND, "dapp not registered" )
        dapp.available_notify_times += quantity.amount / _gstate.event_cpm.amount;
        _db.set( dapp );
    }
}