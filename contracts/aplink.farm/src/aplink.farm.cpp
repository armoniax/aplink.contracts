#include "aplink.farm.hpp"
#include <string>
#include "utils.hpp"
#include "aplink.token/aplink.token.hpp"

static constexpr uint64_t max_text_size     = 128;
using namespace wasm;
using namespace aplink;


static constexpr eosio::name active_permission{"active"_n};
#define TRANSFER(bank, to, quantity, memo) \
    {	token::transfer_action act{ bank, { {_self, active_permission} } };\
			act.send( _self, to, quantity , memo );}

void farm::setlord(const name& lord) {
    require_auth( get_self() );
    CHECKC(is_account(lord), err::ACCOUNT_INVALID, "Invalid account of lord");
    _gstate.lord = lord;
    _global.set(_gstate, get_self());
}

void farm::lend(const name& farmer, 
                    const string& title, 
                    const string& uri, 
                    const time_point& crop_start_at, 
                    const time_point& crop_end_at){
    require_auth( _gstate.lord );
    CHECKC(is_account(farmer), err::ACCOUNT_INVALID, "Invalid account of farmer");
    CHECKC(title.size() < max_text_size, err::CONTENT_LENGTH_INVALID, "title size too large, respect " + to_string(max_text_size));
    CHECKC(uri.size() < max_text_size, err::CONTENT_LENGTH_INVALID, "url size too large, respect " + to_string(max_text_size));
    CHECKC(crop_start_at > current_time_point(), err::TIME_INVALID, "start time cannot earlier than now");
    CHECKC(crop_end_at > crop_start_at, err::TIME_INVALID, "end time cannot earlier than start time");

    auto lands = land_t::idx_t(_self, _self.value);
    auto pid = lands.available_primary_key();
    auto land = land_t(pid);
    land.farmer = farmer;
    land.title = title;
    land.uri = uri;
    land.seeds = asset(0, APLINK_SYMBOL);
    land.crop_start_at = crop_start_at;
    land.crop_end_at = crop_end_at;
    land.crop_end_at = crop_end_at;
    land.created_at = current_time_point();
    land.updated_at = current_time_point();

    _db.set(land, _gstate.lord);
}


void farm::crop(const uint64_t& land_id, const name& customer, const asset& quantity, const string& memo){
    CHECKC( is_account(customer), err::ACCOUNT_INVALID, "Invalid account of farmer" );
    CHECKC( quantity.amount > 0, err::PARAM_ERROR, "non-positive quantity not allowed" );
    CHECKC( quantity.symbol == APLINK_SYMBOL, err::SYMBOL_MISMATCH, "symbol not allowed" );

    auto land = land_t(land_id);
    CHECKC( _db.get( land ), err::RECORD_NOT_FOUND, "land not found: " + to_string(land_id) );

    require_auth(land.farmer);
    CHECKC( customer != land.farmer, err::ACCOUNT_INVALID, "can not crop to farmer" );
    CHECKC( land.seeds.amount >= quantity.amount, err::OVERSIZED, "Overdrawn not allowed: " + land.seeds.to_string() + " > " + quantity.to_string() );
    CHECKC( current_time_point() < land.crop_end_at, err::TIME_EXPIRED, "land has been expired" );
    CHECKC( current_time_point() > land.crop_start_at, err::NOT_STARTED, "land is not start" );
    CHECKC( land.status == land_status_t::LAND_ENABLED, err::NOT_DISABLED, "land is not open" );

    land.seeds -= quantity;
    _db.set( land );

    TRANSFER( APLINK_BANK, customer, quantity, memo);
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

void farm::retrieve(const uint64_t& land_id, const name& recipient, const string& memo){
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