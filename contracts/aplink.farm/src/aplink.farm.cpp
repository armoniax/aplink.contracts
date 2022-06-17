#include <aplink.farm/aplink.farm.hpp>
#include <string>
#include "utils.hpp"
#include "aplink.token/aplink.token.hpp"

static constexpr uint32_t max_text_size     = 64;
using namespace aplink;
using namespace wasm;

#ifndef MONTH_SECONDS_FOR_TEST
static constexpr uint32_t MONTH_SECONDS        = 30 * 24 * 3600;
#else
#warning "MONTH_SECONDS_FOR_TEST should be used only for test!!!"
static constexpr uint64_t MONTH_SECONDS        =  MONTH_SECONDS_FOR_TEST;
#endif//DAY_SECONDS_FOR_TEST

static constexpr eosio::name active_permission{"active"_n};
#define TRANSFER(bank, to, quantity, memo) \
    {	token::transfer_action act{ bank, { {_self, active_permission} } };\
			act.send( _self, to, quantity , memo );}

void farm::setlord(const name& lord, const name& jamfactory) {
    require_auth( get_self() );

    CHECKC(is_account(lord), err::ACCOUNT_INVALID, "Invalid account of lord")
    CHECKC(is_account(jamfactory), err::ACCOUNT_INVALID, "Invalid account of jamfactory")
    _gstate.lord = lord;
    _gstate.jamfactory = jamfactory;
    _global.set(_gstate, get_self());
}

void farm::lease(const name& farmer, 
                    const string& title, 
                    const string& uri, 
                    const string& banner, 
                    const time_point& opened_at, 
                    const time_point& closed_at){
    require_auth( _gstate.lord );

    CHECKC(is_account(farmer), err::ACCOUNT_INVALID, "Invalid account of farmer")
    CHECKC(title.size() < max_text_size, err::CONTENT_LENGTH_INVALID, "title size too large, respect " + to_string(max_text_size))
    CHECKC(uri.size() < max_text_size, err::CONTENT_LENGTH_INVALID, "url size too large, respect " + to_string(max_text_size))
    CHECKC(banner.size() < max_text_size, err::CONTENT_LENGTH_INVALID, "banner size too large, respect " + to_string(max_text_size))
    CHECKC(opened_at > current_time_point(), err::TIME_INVALID, "start time cannot earlier than now")
    CHECKC(closed_at > opened_at, err::TIME_INVALID, "end time cannot earlier than start time")

    auto current_time = current_time_point();
    auto lands = land_t::idx_t(_self, _self.value);
    auto pid = lands.available_primary_key();
    auto land = land_t(pid);
    land.farmer = farmer;
    land.title = title;
    land.uri = uri;
    land.banner = banner;
    land.avaliable_apples = asset(0, APLINK_SYMBOL);
    land.alloted_apples = asset(0, APLINK_SYMBOL);
    land.opened_at = opened_at;
    land.closed_at = closed_at;
    land.created_at = current_time;
    land.updated_at = current_time;

    _db.set(land, _gstate.lord);
}


void farm::allot(const uint64_t& land_id, const name& customer, const asset& quantity, const string& memo){
    CHECKC( is_account(customer), err::ACCOUNT_INVALID, "Invalid account of farmer" );
    CHECKC( quantity.amount > 0, err::PARAM_ERROR, "non-positive quantity not allowed" );
    CHECKC( quantity.symbol == APLINK_SYMBOL, err::SYMBOL_MISMATCH, "symbol not allowed" );
    CHECKC( memo.size() < max_text_size, err::CONTENT_LENGTH_INVALID, "title size too large, respect " + to_string(max_text_size));
    auto land = land_t(land_id);
    CHECKC( _db.get( land ), err::RECORD_NOT_FOUND, "land not found: " + to_string(land_id) );
    CHECKC( customer != land.farmer, err::ACCOUNT_INVALID, "can not crop to farmer" );

    require_auth(land.farmer);
    auto current_time = time_point_sec(current_time_point());
    //TODO CHECK
    if(land.avaliable_apples < quantity) return;
    if(current_time < land.opened_at)  return;
    if(current_time > land.closed_at)  return;
    if(land.status != land_status_t::LAND_ENABLED) return;

    land.avaliable_apples -= quantity;
    land.alloted_apples += quantity;
    _db.set( land );

    auto apples = apple_t::idx_t(_self, _self.value);
    auto pid = apples.available_primary_key();
    auto apple = apple_t(pid);
    //TODO picker->picker, weight->quantity
    apple.picker = customer;
    apple.quantity = quantity;
    apple.memo = memo;
    apple.expired_at = current_time + MONTH_SECONDS;

    _db.set(apple, land.farmer);
}

void farm::pick(const name& picker, vector<uint64_t> appleids){
    CHECKC( aplink::token::account_exist(APLINK_BANK, picker, APLINK_SYMBOL.code()), 
        err::ACCOUNT_INVALID, "picker should get newbie reward first");
    require_auth(picker);

    auto picker_quantity = asset(0, APLINK_SYMBOL);
    auto factory_quantity = asset(0, APLINK_SYMBOL);
    auto now = time_point_sec(current_time_point());

    CHECKC( appleids.size() <= 20, err::CONTENT_LENGTH_INVALID, "appleids too long, expect length 20" );

    string memo = "";
    for (int i = 0; i<appleids.size(); ++i)
	{
        auto apple_id = appleids[i];
        auto apple = apple_t(apple_id);
        CHECKC( _db.get( apple ), err::RECORD_NOT_FOUND, "apple not found: " + to_string(apple_id));
        CHECKC( picker == apple.picker || picker == _gstate.jamfactory, err::ACCOUNT_INVALID, "account invalid");
        
        if( now > apple.expired_at) {
            factory_quantity += apple.quantity;
            _db.del(apple);
        }
        else if( picker != _gstate.jamfactory ){
            if(appleids.size() == 1 && memo.size() == 0) memo = apple.memo;
            picker_quantity += apple.quantity;
            _db.del(apple);
        }
	}

    if(picker_quantity.amount > 0) TRANSFER( APLINK_BANK, picker, picker_quantity, memo.size()==0?"group crop":memo);
    if(factory_quantity.amount > 0) TRANSFER( APLINK_BANK, _gstate.jamfactory, factory_quantity, "jam");
}

void farm::ontransfer(const name& from, 
                       const name& to, 
                       const asset& quantity, 
                       const string& memo){
    if (to != _self) return;

    CHECKC( quantity.amount > 0, err::PARAM_ERROR, "non-positive quantity not allowed" )
    CHECKC( memo != "", err::PARAM_ERROR, "empty memo!" )  
    CHECKC( quantity.symbol == APLINK_SYMBOL, err::PARAM_ERROR, "non-sys-symbol" )
    CHECKC( from == _gstate.lord, err::ACCOUNT_INVALID, "can only trans by lord : " + _gstate.lord.to_string());

    // memo: land_id
    uint64_t land_id = stoi(string(memo));
    auto land = land_t(land_id);
    CHECKC( _db.get( land ), err::RECORD_NOT_FOUND, "land not found: " + to_string(land_id) );
    CHECKC( land.status == land_status_t::LAND_ENABLED, err::NOT_DISABLED, "land not open");

    land.avaliable_apples += quantity;
    _db.set( land ); 
}

void farm::reclaim(const uint64_t& land_id, const string& memo){
    require_auth(_gstate.lord);

    auto land = land_t(land_id);
    CHECKC( _db.get( land ), err::RECORD_NOT_FOUND, "land not found: " + to_string(land_id) )
    CHECKC( land.status == land_status_t::LAND_DISABLED, err::NOT_DISABLED, "land not found: " + to_string(land_id) )
    CHECKC( land.avaliable_apples.amount > 0, err::NOT_POSITIVE, "non-positive quantity not allowed")
    
    TRANSFER( APLINK_BANK, _gstate.jamfactory, land.avaliable_apples, memo);

    land.avaliable_apples -= land.avaliable_apples;
    _db.set( land );
}


void farm::setstatus(const uint64_t& land_id, const uint8_t& status){
    require_auth(_gstate.lord);

    auto land = land_t(land_id);
    CHECKC( _db.get( land ), err::RECORD_NOT_FOUND, "land not found: " + to_string(land_id) );
    CHECKC( land.status != status, err::ACTION_REDUNDANT, "land already in status" );

    switch(status) {
        case land_status_t::LAND_NONE:
        case land_status_t::LAND_ENABLED:
        case land_status_t::LAND_DISABLED:
            break;
        default:
            CHECKC( false, err::PARAM_ERROR, "status not defined : " + to_string(status));
            break;
    };

    land.status = status;
    _db.set( land );
}