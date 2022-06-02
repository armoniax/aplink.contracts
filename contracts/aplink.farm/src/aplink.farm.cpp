#include "aplink.farm.hpp"
#include <string>
#include "utils.hpp"
#include "aplink.token/aplink.token.hpp"

static constexpr uint32_t max_text_size     = 64;
using namespace wasm;
using namespace aplink;

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
    CHECKC(is_account(lord), err::ACCOUNT_INVALID, "Invalid account of lord");
    _gstate.lord = lord;
    _gstate.jamfactory = jamfactory;
    _global.set(_gstate, get_self());
}

void farm::lease(const name& farmer, 
                    const string& title, 
                    const string& uri, 
                    const time_point& open_at, 
                    const time_point& close_at){
    require_auth( _gstate.lord );
    CHECKC(is_account(farmer), err::ACCOUNT_INVALID, "Invalid account of farmer");
    CHECKC(title.size() < max_text_size, err::CONTENT_LENGTH_INVALID, "title size too large, respect " + to_string(max_text_size));
    CHECKC(uri.size() < max_text_size, err::CONTENT_LENGTH_INVALID, "url size too large, respect " + to_string(max_text_size));
    CHECKC(open_at > current_time_point(), err::TIME_INVALID, "start time cannot earlier than now");
    CHECKC(close_at > open_at, err::TIME_INVALID, "end time cannot earlier than start time");

    auto current_time = time_point_sec(current_time_point());
    auto lands = land_t::idx_t(_self, _self.value);
    auto pid = lands.available_primary_key();
    auto land = land_t(pid);
    land.farmer = farmer;
    land.title = title;
    land.uri = uri;
    land.seeds = asset(0, APLINK_SYMBOL);
    land.open_at = time_point_sec(open_at);
    land.close_at = time_point_sec(close_at);
    land.created_at = current_time;
    land.updated_at = current_time;

    _db.set(land, _gstate.lord);
}


void farm::grow(const uint64_t& land_id, const name& customer, const asset& quantity, const string& memo){
    CHECKC( is_account(customer), err::ACCOUNT_INVALID, "Invalid account of farmer" );
    CHECKC( quantity.amount > 0, err::PARAM_ERROR, "non-positive quantity not allowed" );
    CHECKC( quantity.symbol == APLINK_SYMBOL, err::SYMBOL_MISMATCH, "symbol not allowed" );
    CHECKC( memo.size() < max_text_size, err::CONTENT_LENGTH_INVALID, "title size too large, respect " + to_string(max_text_size));
    auto land = land_t(land_id);
    CHECKC( _db.get( land ), err::RECORD_NOT_FOUND, "land not found: " + to_string(land_id) );
    CHECKC( customer != land.farmer, err::ACCOUNT_INVALID, "can not crop to farmer" );

    require_auth(land.farmer);
    auto current_time = time_point_sec(current_time_point());
    if(land.seeds.amount < quantity.amount) return;
    if(current_time < land.open_at)  return;
    if(current_time > land.close_at)  return;
    if(land.status != land_status_t::LAND_ENABLED) return;

    land.seeds -= quantity;
    _db.set( land );

    auto apples = apple_t::idx_t(_self, _self.value);
    auto pid = apples.available_primary_key();
    auto apple = apple_t(pid);
    apple.croper = customer;
    apple.weight = quantity;
    apple.memo = memo;
    apple.expire_at = current_time + MONTH_SECONDS;

    _db.set(apple, land.farmer);
}

void farm::pick(const name& croper, vector<uint64_t> appleids){
    CHECKC( aplink::token::account_exist(APLINK_BANK, croper, APLINK_SYMBOL.code()), 
        err::ACCOUNT_INVALID, "croper should get newbie reward first");
    require_auth(croper);

    auto croper_quantity = asset(0, APLINK_SYMBOL);
    auto factory_quantity = asset(0, APLINK_SYMBOL);
    auto current = time_point_sec(current_time_point());

    CHECKC( appleids.size() <= 20, err::CONTENT_LENGTH_INVALID, "appleids too long, expect length 20" );

    string memo = "";
    for (int i = 0; i<appleids.size(); ++i)
	{
        auto apple_id = appleids[i];
        auto apple = apple_t(apple_id);
        CHECKC( _db.get( apple ), err::RECORD_NOT_FOUND, "apple not found: " + to_string(apple_id));
        CHECKC( apple.croper == croper || _gstate.jamfactory == croper, err::ACCOUNT_INVALID, "account invalid");
        
        if( current > apple.expire_at) {
            factory_quantity += apple.weight;
        }
        else {
            if(appleids.size() == 1 && memo.size() == 0) memo = apple.memo;
            croper_quantity += apple.weight;
        }
        _db.del(apple);
	}

    if(croper_quantity.amount > 0) TRANSFER( APLINK_BANK, croper, croper_quantity, memo.size()==0?"group crop":memo);
    if(factory_quantity.amount > 0) TRANSFER( APLINK_BANK, _gstate.jamfactory, factory_quantity, "jam");
}

void farm::ontransfer(const name& from, 
                       const name& to, 
                       const asset& quantity, 
                       const string& memo){
    if (to == _self){
        CHECKC( quantity.amount > 0, err::PARAM_ERROR, "non-positive quantity not allowed" )
        CHECKC( memo != "", err::PARAM_ERROR, "empty memo!" )  
        CHECKC( quantity.symbol == APLINK_SYMBOL, err::PARAM_ERROR, "non-sys-symbol" )
        CHECKC( from == _gstate.lord, err::ACCOUNT_INVALID, "can only trans by lord : " + _gstate.lord.to_string());

        // memo: land_id
        uint64_t land_id = stoi(string(memo));
        auto land = land_t(land_id);
        CHECKC( _db.get( land ), err::RECORD_NOT_FOUND, "land not found: " + to_string(land_id) );
        CHECKC( land.status == land_status_t::LAND_ENABLED, err::NOT_DISABLED, "land not open");

        land.seeds += quantity;
        _db.set( land ); 
    }
}

void farm::reclaim(const uint64_t& land_id, const name& recipient, const string& memo){
    CHECKC( is_account(recipient), err::ACCOUNT_INVALID, "Invalid account of recipient" );
    require_auth(_gstate.lord);

    auto land = land_t(land_id);
    CHECKC( _db.get( land ), err::RECORD_NOT_FOUND, "land not found: " + to_string(land_id) );
    CHECKC( land.status == land_status_t::LAND_DISABLED, err::NOT_DISABLED, "land not found: " + to_string(land_id) );
    CHECKC( land.seeds.amount > 0, err::NOT_POSITIVE, "non-positive quantity not allowed");
    
    TRANSFER( APLINK_BANK, recipient, land.seeds, memo);

    land.seeds -= land.seeds;
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