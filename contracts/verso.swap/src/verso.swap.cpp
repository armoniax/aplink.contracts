#include <amax.ntoken/amax.ntoken.hpp>
#include "verso.swap.hpp"
#include "commons/utils.hpp"

#include <cstdlib>
#include <ctime>

#include <chrono>

using std::chrono::system_clock;
using namespace wasm;
using namespace amax;
using namespace std;

#define TRANSFER(bank, to, quantity, memo) \
{ action(permission_level{get_self(), "active"_n }, bank, "transfer"_n, std::make_tuple( _self, to, quantity, memo )).send(); }


void blindbox::init(){

    CHECKC( false, err::MISC ,"error");
    pool_t::tbl_t pool( get_self(), get_self().value);
    auto p_itr = pool.begin();
    while( p_itr != pool.end() ){
        p_itr = pool.erase( p_itr );
    } 
}

void blindbox::createtotal( const uint64_t& amount ){
    
    require_auth( _self );

    CHECKC( amount > 0 , err::PARAM_ERROR , "amount must be > 0");
    totality_t::tbl_t total_t( _self, _self.value);
    total_t.emplace( _self, [&]( auto& row ) {
        row.id                 = ++_gstate.last_totality_id;
        row.total_amount       = amount;
        row.balance_amount     = amount;              
        row.created_at         = current_time_point();
        row.updated_at          = current_time_point();
    }); 
}

void blindbox::bindtotal( const vector<uint64_t>& pool_ids, const uint64_t& totality_id ){
    require_auth( _self );
    
    totality_t::tbl_t total_t( _self, _self.value);

    auto total_itr = total_t.find(totality_id);
    CHECKC( total_itr != total_t.end(),err::RECORD_NOT_FOUND,"totality not found");

    for( uint64_t id : pool_ids){
       pool_t::tbl_t pool( get_self() ,get_self().value );
       auto pool_itr = pool.find( id ); 
       CHECKC( pool_itr != pool.end(),err::RECORD_NOT_FOUND,"pool not found,id:" + to_string(id) );

       pool.modify( pool_itr, same_payer, [&]( auto& row ){
            row.totality_id  = totality_id;
            row.updated_at   = current_time_point();
       });
        
    }
}

void blindbox::addpooltoken( const name& owner,const string& title,const name& asset_contract, const name& blindbox_contract,
                           const asset& price, const name& fee_receiver,
                           const name& exchange_type, const uint64_t& totality_id,
                           const time_point_sec& opended_at, const uint64_t& opened_days){
 
    require_auth(owner);
    
    //CHECKC( is_account(owner),err::ACCOUNT_INVALID,"owner does not exist");
    CHECKC( is_account(asset_contract),err::ACCOUNT_INVALID,"asset contract does not exist");
    CHECKC( is_account(fee_receiver),err::ACCOUNT_INVALID,"fee receiver does not exist");
    CHECKC( is_account(blindbox_contract),err::ACCOUNT_INVALID,"blinbox contract does not exist");
    
    CHECKC( exchange_type == exchange_type_t::ergodic || exchange_type == exchange_type_t::random, err::STATUS_ERROR,"exchange type must be ergodic | random");
    CHECKC( price.amount > 0 , err::PARAM_ERROR ,"price must > 0");

    if ( totality_id != 0) {
        totality_t::tbl_t total_t( _self, _self.value);
        auto total_itr = total_t.find(totality_id);
        CHECKC( total_itr != total_t.end(),err::RECORD_NOT_FOUND,"totality not found");
    }
    price_token price_t;

    price_t.price               = price;
    price_t.received            = asset( 0 ,price.symbol);
    price_t.fee_receiver        = fee_receiver;

    pool_t::tbl_t pool( get_self() ,get_self().value );
    pool.emplace( owner, [&](auto& row){
        row.id                  = ++_gstate.last_pool_id;
        row.owner               = owner;
        row.title               = title;
        row.token_price         = price_t;
        row.asset_contract      = asset_contract;
        row.blindbox_contract   = blindbox_contract;
        row.asset_type          = asset_type_t::token;
        row.exchange_type       = exchange_type;
        row.created_at          = current_time_point();
        row.updated_at          = current_time_point();
        row.opended_at          = opended_at;
        row.closed_at           = time_point_sec(opended_at.sec_since_epoch() + opened_days * DAY_SECONDS);
        row.status              = pool_status::enabled;
        row.totality_id         = totality_id;
    });

    account::tbl_t account_tbl(get_self(), get_self().value);
    account_tbl.set(owner.value, owner, [&]( auto& acct ) {
        acct.owner                  = owner;
        acct.last_pool_id           = _gstate.last_pool_id;
    });
    _global.set( _gstate, get_self() );
}


void blindbox::addpoolnft( const name& owner,const string& title,const name& asset_contract, const name& blindbox_contract,
                           const nasset& price, const name& fee_receiver,
                           const name& exchange_type, const uint64_t& totality_id,
                           const time_point_sec& opended_at, const uint64_t& opened_days){
 
    require_auth(owner);
    //CHECKC( is_account(owner),err::ACCOUNT_INVALID,"owner does not exist");
    CHECKC( is_account(asset_contract),err::ACCOUNT_INVALID,"asset contract does not exist");
    CHECKC( is_account(fee_receiver),err::ACCOUNT_INVALID,"fee receiver does not exist");
    CHECKC( is_account(blindbox_contract),err::ACCOUNT_INVALID,"blinbox contract does not exist");

    CHECKC( exchange_type == exchange_type_t::ergodic || exchange_type == exchange_type_t::random, err::STATUS_ERROR,"exchange type must be ergodic | random");
    CHECKC( price.amount > 0 , err::PARAM_ERROR ,"price must > 0");
    totality_t::tbl_t total_t( _self, _self.value);

    if ( totality_id != 0) {
        auto total_itr = total_t.find(totality_id);
        CHECKC( total_itr != total_t.end(),err::RECORD_NOT_FOUND,"totality not found");
    }

    price_nft price_t;

    price_t.price               = price;
    price_t.received            = nasset( 0 ,price.symbol);
    price_t.fee_receiver        = fee_receiver;

    pool_t::tbl_t pool( get_self() ,get_self().value );
    pool.emplace( owner, [&](auto& row){
        row.id                  = ++_gstate.last_pool_id;
        row.owner               = owner;
        row.title               = title;
        row.nft_price           = price_t;
        row.asset_contract      = asset_contract;
        row.asset_type          = asset_type_t::nft;
        row.exchange_type       = exchange_type;
        row.created_at          = current_time_point();
        row.updated_at          = current_time_point();
        row.opended_at          = opended_at;
        row.closed_at           = time_point_sec(opended_at.sec_since_epoch() + opened_days * DAY_SECONDS);
        row.status              = pool_status::enabled;
        row.totality_id         = totality_id;
    });

    account::tbl_t account_tbl(get_self(), get_self().value);
    account_tbl.set(owner.value, owner, [&]( auto& acct ) {
        acct.owner                  = owner;
        acct.last_pool_id           = _gstate.last_pool_id;
    });
    _global.set( _gstate, get_self() );
}


void blindbox::enableplan(const name& owner, const uint64_t& pool_id, bool enabled) {


    pool_t::tbl_t pool_tbl( get_self() ,get_self().value );
    auto pool_itr = pool_tbl.find(pool_id);
    CHECKC( pool_itr != pool_tbl.end(),  err::RECORD_NOT_FOUND,"pool not found: " + to_string(pool_id) )

    CHECKC( has_auth( owner ) || has_auth( _self ), err::NO_AUTH, "not authorized" )

    CHECKC( owner == pool_itr->owner,  err::NO_AUTH, "not authorized" )
    name new_status = enabled ? pool_status::enabled : pool_status::disabled ;
    CHECKC( pool_itr->status != new_status,err::STATUS_ERROR, "pool status is no changed" )

    pool_tbl.modify( pool_itr, same_payer, [&]( auto& pool ) {
        pool.status         = new_status;
        pool.updated_at     = current_time_point();
    });
}


void blindbox::editplantime(const name& owner, const uint64_t& pool_id, const time_point_sec& opended_at, const time_point_sec& closed_at){
    pool_t::tbl_t pool_tbl( get_self() ,get_self().value );
    auto pool_itr = pool_tbl.find(pool_id);
    CHECKC( pool_itr != pool_tbl.end(),  err::RECORD_NOT_FOUND,"pool not found: " + to_string(pool_id) )

    CHECKC( has_auth( owner ) || has_auth( _self ), err::NO_AUTH, "not authorized" )

    CHECKC( owner == pool_itr->owner,  err::NO_AUTH, "not authorized" )

    pool_tbl.modify( pool_itr, same_payer, [&]( auto& pool ) {
        pool.opended_at     = opended_at;
        pool.closed_at      = closed_at;
        pool.updated_at     = current_time_point();
    });
}

// void blindbox::fillpool( const name& owner, const uint64_t& pool_id, const vector<nasset>& assets){

// }


void blindbox::ontokentrans( const name& from, const name& to, const asset& quantity, const string& memo ){
    if (from == get_self() || to != get_self()) return;
    
    vector<string_view> memo_params = split(memo, ":");
    ASSERT(memo_params.size() > 0)

    auto now = time_point_sec(current_time_point());

    if ( memo_params[0] == "open" ){

        CHECKC( memo_params.size() == 2, err::MEMO_FORMAT_ERROR, "ontransfer:issue params size of must be 2")
        auto pool_id            = std::stoul(string(memo_params[1]));

        pool_t::tbl_t pool( get_self() ,get_self().value );
        auto pool_itr           = pool.find( pool_id );
        CHECKC( pool_itr != pool.end(), err::RECORD_NOT_FOUND, "pool not found: " + to_string(pool_id) )
        CHECKC( pool_itr->status == pool_status::enabled, err::STATUS_ERROR, "pool not enabled, status:" + pool_itr->status.to_string() )
        CHECKC( pool_itr->opended_at <= now, err::STATUS_ERROR, "Time is not up ");
        CHECKC( pool_itr->closed_at >= now,err::STATUS_ERROR, "It's too late ");

        auto amount = quantity.amount / pool_itr->token_price.price.amount;

        CHECKC( pool_itr-> exchange_type == exchange_type_t::random && amount == 1,err::PARAM_ERROR, "random type quantity must be 1");

        CHECKC( pool_itr->asset_contract == get_first_receiver(), err::DATA_MISMATCH, "issue asset contract mismatch" );
        CHECKC( pool_itr->asset_type == asset_type_t::token, err::RECORD_NOT_FOUND,"This asset purchase is not supported");
        CHECKC( pool_itr->token_price.price.symbol == quantity.symbol, err::SYMBOL_MISMATCH, "blindbox asset symbol mismatch" );

        CHECKC( pool_itr->not_exchange_nft_amount >= amount ,  err::OVERSIZED, "Remaining blind box:" + to_string( pool_itr->not_exchange_nft_amount) );
        CHECKC( _gstate.max_exchange_num >= amount , err::MISC, "Maximum opening at one time: " + to_string( _gstate.max_exchange_num ));
        
        if ( pool_itr->totality_id != 0){
            totality_t::tbl_t total_t( _self, _self.value);
            auto total_itr = total_t.find(pool_itr->totality_id);
            CHECKC( total_itr != total_t.end(),err::RECORD_NOT_FOUND,"totality not found");
            CHECKC( total_itr-> balance_amount >= amount, err::OVERSIZED,"Quantity exceeds");
            
            total_t.modify( total_itr , same_payer, [&]( auto& row ){
                row.balance_amount -= amount;
                row.updated_at       = now;
            });
        }
        auto p = pool.get( pool_id );
        uint64_t count = 0;
        if ( pool_itr-> exchange_type == exchange_type_t::random){
            count = _rand_nft( p , from);
        } else if (pool_itr-> exchange_type == exchange_type_t::ergodic ) {
            count = _ergodic_nft( p, from );
        }
        
        
        pool.modify( pool_itr, same_payer, [&]( auto& row ) {
            row.not_exchange_nft_amount     -= amount;
            row.exchange_nft_amount         += amount;
            row.updated_at                  = now;
            row.max_table_distance          -= count;
        });
    }
}

void blindbox::onversotrans( const name& from, const name& to, const vector<nasset>& assets, const string& memo){

    if (from == get_self() || to != get_self()) return;
    vector<string_view> memo_params = split(memo, ":");
    ASSERT(memo_params.size() > 0)

    auto now = time_point_sec(current_time_point());
    if (memo_params[0] == "mint" || memo_params[0] == "mint_loop") {

        CHECKC(memo_params.size() == 2, err::MEMO_FORMAT_ERROR, "ontransfer:issue params size of must be 2")
        auto pool_id            = std::stoul(string(memo_params[1]));

        pool_t::tbl_t pool( get_self() ,get_self().value );
        auto pool_itr           = pool.find( pool_id );
        CHECKC( pool_itr != pool.end(), err::RECORD_NOT_FOUND, "pool not found: " + to_string(pool_id) )
        CHECKC( pool_itr->status == pool_status::enabled, err::STATUS_ERROR, "pool not enabled, status:" + pool_itr->status.to_string() )
        CHECKC( pool_itr->opended_at <= now, err::STATUS_ERROR, "Time is not up ");
        CHECKC( pool_itr->blindbox_contract == get_first_receiver(), err::DATA_MISMATCH, "issue asset contract mismatch" );
        // CHECKC( pool_itr->asset_symbol == quantity.symbol, err::SYMBOL_MISMATCH, "issue asset symbol mismatch" );
        CHECKC( pool_itr->owner == from,  err::NO_AUTH, "not authorized" );

        auto new_nft_id = pool_itr->last_nft_id;
        uint64_t max_distance = 0;
        uint64_t amount = 0;
        for( nasset quantity : assets){

            CHECKC( quantity.amount > 0, err::NOT_POSITIVE, "quantity must be positive" )

            amount += quantity.amount;

            if ( memo_params[0] == "mint_loop" ){

                max_distance += quantity.amount; 

                for ( auto i = 0; i < quantity.amount ; i++ ){
                    nft_t::tbl_t nft( get_self(), pool_id);
                    nft.emplace( _self, [&]( auto& row ) {
                        row.id                 = ++new_nft_id;
                        row.asset_contract     = get_first_receiver();
                        row.quantity           = nasset(1,quantity.symbol);              
                    });
                }
            }else {
                max_distance ++;
                nft_t::tbl_t nft( get_self(), pool_id);
                nft.emplace( _self, [&]( auto& row ) {
                    row.id                 = ++new_nft_id;
                    row.asset_contract     = get_first_receiver();
                    row.quantity           = quantity;              
                });
            }
        }

        auto now = current_time_point();
        pool.modify( pool_itr, same_payer, [&]( auto& row ) {
            row.total_nft_amount            += amount;
            row.not_exchange_nft_amount     += amount;
            row.max_table_distance          += max_distance;
            row.last_nft_id                 = new_nft_id;
            row.updated_at                  = now;
        });

    }

}
void blindbox::onnnfttrans( const name& from, const name& to, const vector<nasset>& assets, const string& memo){

    if (from == get_self() || to != get_self()) return;
    //memo params format:
    //1. open:${pool_id}"
    //2. mint:${pool_id}
    //2. mint_loop:${pool_id}

    vector<string_view> memo_params = split(memo, ":");
    ASSERT(memo_params.size() > 0)

    auto now = time_point_sec(current_time_point());

    if ( memo_params[0] == "open" ){
            
        CHECKC( assets.size() == 1, err::OVERSIZED, "Only one NFT is allowed at a time point" );

        nasset quantity = assets[0];
        
        //CHECKC( quantity.amount == 1, err::NOT_POSITIVE, "quantity must be 1" )

        CHECKC( memo_params.size() == 2, err::MEMO_FORMAT_ERROR, "ontransfer:issue params size of must be 2")
        auto pool_id            = std::stoul(string(memo_params[1]));

        pool_t::tbl_t pool( get_self() ,get_self().value );
        auto pool_itr           = pool.find( pool_id );
        CHECKC( pool_itr != pool.end(), err::RECORD_NOT_FOUND, "pool not found: " + to_string(pool_id) )
        CHECKC( pool_itr->status == pool_status::enabled, err::STATUS_ERROR, "pool not enabled, status:" + pool_itr->status.to_string() )
        CHECKC( pool_itr->opended_at <= now, err::STATUS_ERROR, "Time is not up ");
        CHECKC( pool_itr->closed_at >= now,err::STATUS_ERROR, "It's too late ");

        CHECKC( pool_itr-> exchange_type == exchange_type_t::random && quantity.amount == 1, err::PARAM_ERROR,"random type quantity must be 1");

        CHECKC( pool_itr->asset_contract == get_first_receiver(), err::DATA_MISMATCH, "issue asset contract mismatch" );
        CHECKC( pool_itr->asset_type == asset_type_t::nft, err::RECORD_NOT_FOUND,"This asset purchase is not supported");
        CHECKC( pool_itr->nft_price.price.symbol == quantity.symbol, err::SYMBOL_MISMATCH, "blindbox asset symbol mismatch" );

        CHECKC( pool_itr->not_exchange_nft_amount >= quantity.amount ,  err::OVERSIZED, "Remaining blind box:" + to_string( pool_itr->not_exchange_nft_amount) );
        CHECKC( _gstate.max_exchange_num >= quantity.amount , err::MISC, "Maximum opening at one time: " + to_string( _gstate.max_exchange_num ));

        if ( pool_itr->totality_id != 0){
            totality_t::tbl_t total_t( _self, _self.value);
            auto total_itr = total_t.find(pool_itr->totality_id);
            CHECKC( total_itr != total_t.end(),err::RECORD_NOT_FOUND,"totality not found");
            CHECKC( total_itr-> balance_amount >= quantity.amount, err::OVERSIZED,"Quantity exceeds");
            
            total_t.modify( total_itr , same_payer, [&]( auto& row ){
                row.balance_amount -= quantity.amount;
                row.updated_at       = now;
            });
        }

        auto p = pool.get( pool_id );
        uint64_t count = 0;
        if ( pool_itr-> exchange_type == exchange_type_t::random){
            count = _rand_nft( p , from);
        } else if (pool_itr-> exchange_type == exchange_type_t::ergodic ) {
            count = _ergodic_nft( p, from );
        }
        
        auto now = current_time_point();
        pool.modify( pool_itr, same_payer, [&]( auto& row ) {
            row.not_exchange_nft_amount     -= quantity.amount;
            row.exchange_nft_amount         += quantity.amount;
            row.max_table_distance          -= count;
            row.updated_at                  = now;
        });
    }
}

void blindbox::endpool(const name& owner, const uint64_t& pool_id){

    CHECKC( has_auth( owner ) || has_auth( _self ), err::NO_AUTH, "not authorized to end pool" )
    
    pool_t::tbl_t pool( get_self() ,get_self().value );
    auto pool_itr = pool.find( pool_id );

    CHECKC( pool_itr != pool.end(), err::RECORD_NOT_FOUND, "pool not found: " + to_string(pool_id) )
    CHECKC( pool_itr->status == pool_status::enabled, err::STATUS_ERROR, "pool not enabled, status:" + pool_itr->status.to_string() )
    CHECKC( pool_itr->owner == owner , err::NO_AUTH, "not authorized to end pool" );

    int step = 0;
    vector<nasset> quants;

    nft_t::tbl_t nft( get_self(), pool_id);
    auto itr = nft.begin();
    CHECKC( itr != nft.end(), err::RECORD_NOT_FOUND, "Completed");
    uint64_t amount = 0;
    while (itr != nft.end()) {
        if (step > 30) return;
        amount += itr->quantity.amount;
        quants.emplace_back( itr-> quantity);
        itr = nft.erase( itr );
        step++;
    }
    
    auto now = current_time_point();
    pool.modify( pool_itr, same_payer, [&]( auto& row ) {
        row.refund_nft_amount       += amount;
        row.updated_at              = now;
        row.max_table_distance      -= step;
    });
    TRANSFER( pool_itr->blindbox_contract, owner, quants , std::string("end pool"));
}


void blindbox::test( const uint64_t& len){
    
    auto mixid = tapos_block_prefix() * tapos_block_num() + get_self().value + 1 - current_time_point().sec_since_epoch();
    const char *mixedChar = reinterpret_cast<const char *>(&mixid);
    auto hash = sha256( (char *)mixedChar, sizeof(mixedChar));
    string rands = "";
    for ( auto i = 0 ;i < len;  i ++){
        auto r1 = (uint64_t)hash.data()[i];
        uint64_t rand = r1 % len + 1;
        rands += to_string(rand) + "," ;
    }

    CHECK( false,  rands);
    // auto rand = _rand(10, 1, get_self(),1 );
    // nft_t::tbl_t nft( get_self(), 1);
    // auto itr = nft.begin();
    // advance(itr ,rand - 1 );  
    // CHECK( false, "nft :" + to_string( itr->id ));

    // pool_t::tbl_t pool( get_self() ,get_self().value );
    // auto pool_itr           = pool.find( 4 );
    // auto p = pool.get( 4 );
    // _rand_nft( p , get_self());
    // auto now = current_time_point();
    // pool.modify( pool_itr, same_payer, [&]( auto& row ) {
    //     row.not_exchange_nft_amount     -= 1;
    //     row.sealed_blindboxes       += 1;
    //     //row.received_blindbox       += 1;
    //     row.updated_at              = now;
    // });

    // vector<uint64_t> num = { 1, 2,3,4,5,6,7,8,9,10 };

    // uint64_t max = 10;
    // auto itr = num.begin();

    // for ( auto i = 0 ;i < num.size();  i ++){ 

    //     uint64_t rand = _rand( max , 1 , get_self(), i);
    //     advance(itr ,rand - 1 );
    //     get_nft_t::tbl_t getnfts( get_self(), get_self().value);
    //     auto nft_id = getnfts.available_primary_key();
    //     getnfts.emplace( get_self(), [&]( auto& row){

    //         row.id = nft_id;
    //         row.nft_id = rand;
    //     });
           
    //     max --;
    // }
}

uint64_t blindbox::_rand_nft( const pool_t& pool , const name& owner){

    nft_t::tbl_t nft( get_self(), pool.id);
    uint64_t rand = _rand( pool.max_table_distance , 1 , owner, pool.id);
    auto itr = nft.begin();
    advance( itr , rand - 1 );
    vector<nasset> quants = { itr->quantity };
    TRANSFER( itr->asset_contract, owner, quants , std::string("get blindbox"));
    nft.erase( itr );
    
    return 1;
}

uint64_t blindbox::_ergodic_nft( const pool_t& pool , const name& owner){

    uint64_t count = 0;
    nft_t::tbl_t nft( get_self(), pool.id);

    vector<nasset> quants;
    for( auto itr = nft.begin(); itr != nft.end() ; itr ++){
        quants.push_back( nasset( 1, itr-> quantity.symbol));
        if ( itr-> quantity.amount == 0 ){
            nft.erase( itr );
            count++;
        }
    }

    TRANSFER( pool.blindbox_contract, owner, quants , std::string("suit"));
    return count;
}

// void blindbox::_rand_nft( const pool_t& pool , const name& owner, const uint64_t& count){

//     nft_t::tbl_t nft( get_self(), pool.id);

//     uint64_t max_itr = pool.not_exchange_nft_amount;
    
//     for ( auto i = 0 ;i < count; i ++){ 
//         // CHECK( false, " lasdf");
//         uint64_t rand = _rand( max_itr , 1 , owner, i);
//         //auto itr = nft.begin();
//         //advance( itr , rand - 1 );
//         get_nft_t::tbl_t getnfts( get_self(), get_self().value);
//         auto nft_id = getnfts.available_primary_key();
//         getnfts.emplace( get_self(), [&]( auto& row){

//             row.id = nft_id;
//             row.nft_id = rand;
//         });

//         // nft.erase( itr );
//         // max_itr --;
//     }
// }


uint64_t blindbox::_rand(uint64_t max_uint,  uint16_t min_unit, name owner , uint64_t pool_id){

    auto mixid = tapos_block_prefix() * tapos_block_num() + owner.value + pool_id - current_time_point().sec_since_epoch();
    const char *mixedChar = reinterpret_cast<const char *>(&mixid);
    auto hash = sha256( (char *)mixedChar, sizeof(mixedChar));

    auto r1 = (uint64_t)hash.data()[0];
    uint64_t rand = r1 % max_uint + min_unit;
    return rand;
}