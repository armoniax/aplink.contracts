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

void farm::init(const name& lord, const name& jamfactory) {
    require_auth( get_self() );

    CHECKC(is_account(lord), err::ACCOUNT_INVALID, "Invalid account of lord")
    CHECKC(is_account(jamfactory), err::ACCOUNT_INVALID, "Invalid account of jamfactory")

    _gstate.landlord        = lord;
    _gstate.jamfactory      = jamfactory;
    
}

void farm::lease(   const name& tenant, 
                    const string& land_title, 
                    const string& land_uri, 
                    const string& banner_uri) {
    require_auth( _gstate.landlord );

    CHECKC( is_account(tenant), err::ACCOUNT_INVALID, "Tenant account invalid")
    CHECKC( land_title.size() < max_text_size, err::CONTENT_LENGTH_INVALID, "title size too large, respect " + to_string(max_text_size))
    CHECKC( land_uri.size() < max_text_size, err::CONTENT_LENGTH_INVALID, "url size too large, respect " + to_string(max_text_size))
    CHECKC( banner_uri.size() < max_text_size, err::CONTENT_LENGTH_INVALID, "banner size too large, respect " + to_string(max_text_size))
    // CHECKC( opened_at > current_time_point(), err::TIME_INVALID, "start time cannot earlier than now")
    // CHECKC( closed_at > opened_at, err::TIME_INVALID, "end time cannot earlier than start time")

    auto now                    = current_time_point();
    auto lands                  = lease_t::idx_t(_self, _self.value);
    auto pid                    = lands.available_primary_key(); if(pid == 0) pid = 1;
    
    auto lease                  = lease_t(pid);
    lease.tenant                = tenant;
    lease.land_title            = land_title;
    lease.land_uri              = land_uri;
    lease.banner_uri            = banner_uri;
    lease.opened_at             = now;
    lease.created_at            = now;
    lease.updated_at            = now;

    _db.set(lease, _gstate.landlord);
}


void farm::allot(const uint64_t& lease_id, const name& farmer, const asset& quantity, const string& memo) {

    CHECKC( is_account(farmer), err::ACCOUNT_INVALID, "Invalid account of farmer" );
    CHECKC( quantity.amount > 0, err::PARAM_ERROR, "non-positive quantity not allowed" );
    CHECKC( quantity.symbol == APLINK_SYMBOL, err::SYMBOL_MISMATCH, "symbol not allowed" );
    CHECKC( memo.size() < max_text_size, err::CONTENT_LENGTH_INVALID, "title size too large, respect " + to_string(max_text_size));
    
    auto lease                  = lease_t(lease_id);
    auto now                    = time_point_sec(current_time_point());
    
    CHECKC( _db.get( lease ), err::RECORD_NOT_FOUND, "land not found: " + to_string(lease_id) )
    CHECKC( farmer != lease.tenant, err::ACCOUNT_INVALID, "cannot allot to land's tenant: " + lease.tenant.to_string() )
    // CHECKC( now >= lease.opened_at && now <= lease.closed_at, err::TIME_INVALID, "lease is not open")
    CHECKC( lease.status == lease_status::active, err::NOT_STARTED, "lease is not active")
    CHECKC( lease.available_apples >= quantity, err::OVERSIZED, "allot quantity is oversized, balance: " + lease.available_apples.to_string() )
    require_auth(lease.tenant);

    lease.available_apples       -= quantity;
    lease.alloted_apples         += quantity;
    _db.set( lease );

    auto allots                 = allot_t::idx_t(_self, _self.value);
    auto pid                    = allots.available_primary_key(); if (pid == 0) pid = 1;

    auto allot                  = allot_t(pid);
    allot.lease_id              = lease_id;
    allot.farmer                = farmer;
    allot.apples                = quantity;
    allot.alloted_at            = now;
    allot.expired_at            = now + MONTH_SECONDS;

    _db.set(allot, _self);
}

void farm::pick(const name& farmer, const vector<uint64_t>& allot_ids) {
    require_auth(farmer);
    
    auto has_apl                = aplink::token::account_exist(APLINK_BANK, farmer, APLINK_SYMBOL.code());
    CHECKC( has_apl, err::RECORD_NOT_FOUND, "farmer should get newbie reward first" )

    auto farmer_quantity        = asset(0, APLINK_SYMBOL);
    auto factory_quantity       = asset(0, APLINK_SYMBOL);
    auto now                    = time_point_sec(current_time_point());

    CHECKC( allot_ids.size() <= 20, err::CONTENT_LENGTH_INVALID, "allot_ids too long, expect length 20" );
    
    string memo = "";
    for (auto& allot_id : allot_ids) {
        auto allot = allot_t(allot_id);
        CHECKC( _db.get( allot ), err::RECORD_NOT_FOUND, "allot not found: " + to_string(allot_id) )
        CHECKC( farmer == allot.farmer || farmer == _gstate.jamfactory, err::ACCOUNT_INVALID, "farmer account not authorized" )
        
        if (now > allot.expired_at) { //already expired
            factory_quantity        += allot.apples;
            _db.del( allot );

        } else {
            CHECKC( farmer !=  _gstate.jamfactory, err::NO_AUTH, "jamfactory pick not allowed" )

            farmer_quantity         += allot.apples;
            _db.del( allot );
        }
	}

    if (farmer_quantity.amount > 0) 
        TRANSFER( APLINK_BANK, farmer, farmer_quantity, "pick" )

    if (factory_quantity.amount > 0) 
        TRANSFER( APLINK_BANK, _gstate.jamfactory, factory_quantity, "jam");
}

void farm::ontransfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    if (to == _self){
        CHECKC( quantity.amount > 0, err::PARAM_ERROR, "non-positive quantity not allowed" )
        CHECKC( quantity.symbol == APLINK_SYMBOL, err::PARAM_ERROR, "non-sys-symbol" )
        CHECKC( from == _gstate.landlord, err::ACCOUNT_INVALID, "can only trans by lord : " + _gstate.landlord.to_string());
        CHECKC( memo != "", err::PARAM_ERROR, "empty memo!" )  

        // memo: lease_id
        uint64_t lease_id           = stoi(string(memo));
        auto lease                  = lease_t(lease_id);
        CHECKC( _db.get( lease ), err::RECORD_NOT_FOUND, "lease not found: " + to_string(lease_id) );
        CHECKC( lease.status == lease_status::active, err::NOT_DISABLED, "lease not enabled");

        lease.available_apples      += quantity;
        _db.set( lease ); 
    }
}

void farm::reclaimlease(const name& issuer, const uint64_t& lease_id, const string& memo) {
    require_auth( issuer );
    CHECKC( _gstate.landlord == issuer, err::NO_AUTH, "issuer not landlord" )

    auto lease = lease_t(lease_id);
    CHECKC( _db.get( lease ), err::RECORD_NOT_FOUND, "lease not found: " + to_string(lease_id) )
    CHECKC( lease.status == lease_status::active, err::NOT_DISABLED, "lease not active: " + to_string(lease_id) )
    CHECKC( lease.available_apples.amount > 0, err::NOT_POSITIVE, "reclaim non-positive quantity not allowed")
    
    TRANSFER( APLINK_BANK, _gstate.jamfactory, lease.available_apples, memo )

    lease.available_apples.amount = 0;
    lease.updated_at = current_time_point();
    _db.set( lease );

}

void farm::reclaimallot(const name& issuer, const uint64_t& allot_id, const string& memo) {
    require_auth( issuer );

    auto allot = allot_t(allot_id);
    CHECKC( _db.get( allot ), err::RECORD_NOT_FOUND, "allot not found: " + to_string(allot_id) )

    auto lease = lease_t(allot.lease_id);
    CHECKC( _db.get( lease ), err::RECORD_NOT_FOUND, "lease not found: " + to_string(allot.lease_id) )

    if (_gstate.landlord == issuer) {
        TRANSFER( APLINK_BANK, _gstate.jamfactory, allot.apples, memo )
        _db.del( allot );

    } else if (lease.tenant == issuer) {
        CHECKC( lease.alloted_apples >= allot.apples, err::OVERSIZED, "lease has insufficient total alloted apples" )
        lease.available_apples  += allot.apples;
        lease.alloted_apples    -= allot.apples;

        _db.set( lease );
        _db.del( allot );

    } else {
        CHECKC( false, err::NO_AUTH, "neither landlord nor lease tenant" )
    } 
}

void farm::setstatus(const uint64_t& lease_id, const name& status){
    require_auth( _gstate.landlord );

    auto lease = lease_t(lease_id);
    CHECKC( _db.get( lease ), err::RECORD_NOT_FOUND, "lease not found: " + to_string(lease_id) );
    CHECKC( lease.status != status, err::ACTION_REDUNDANT, "land status unchanged" );
    CHECKC( status == lease_status::none || status == lease_status::active || status == lease_status::inactive, err::PARAM_ERROR, "status undefined: " + status.to_string() )

    lease.status = status;
    _db.set( lease );
}
