#include <amax.ntoken/amax.ntoken.hpp>
#include "nftone.blindbox.hpp"
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

    // get_nft_t::tbl_t deals(_self,_self.value);
	// auto itr1 = deals.begin();
	// while(itr1 != deals.end()){
	// 	itr1 = deals.erase(itr1);
	// }
    CHECKC( false, err::MISC ,"error");
}

void blindbox::createpool( const name& owner,const string& title,const name& asset_contract, const nsymbol& asset_symbol,const time_point_sec& opended_at){

    require_auth(get_self());
    
    pool_t::tbl_t pool( get_self() ,get_self().value );
    pool.emplace( get_self(), [&](auto& row){
        row.id                  = ++_gstate.last_pool_id;
        row.owner               = owner;
        row.title               = title;
        row.asset_contract      = asset_contract;
        row.asset_symbol        = asset_symbol;
        row.received_blindbox   = nasset( 0, asset_symbol );
        row.created_at          = current_time_point();
        row.updated_at          = current_time_point();
        row.opended_at          = opended_at;
        row.status              = pool_status::enabled;
    });
}

void blindbox::onnfttrans( const name& from, const name& to, const vector<nasset>& assets, const string& memo){

    if (from == get_self() || to != get_self()) return;

    CHECKC( assets.size() == 1, err::OVERSIZED, "Only one NFT is allowed at a time point" );
   
    //memo params format:
    //1. open:${pool_id}"
    //2. mint:${pool_id}

    vector<string_view> memo_params = split(memo, ":");
    ASSERT(memo_params.size() > 0)
    if (memo_params[0] == "mint") {

        for( nasset quantity : assets){
            
            CHECKC( quantity.amount > 0, err::NOT_POSITIVE, "quantity must be positive" )

            CHECKC(memo_params.size() == 2, err::MEMO_FORMAT_ERROR, "ontransfer:issue params size of must be 2")
            auto pool_id            = std::stoul(string(memo_params[1]));

            pool_t::tbl_t pool( get_self() ,get_self().value );
            auto pool_itr           = pool.find( pool_id );
            CHECKC( pool_itr != pool.end(), err::RECORD_NOT_FOUND, "pool not found: " + to_string(pool_id) )
            CHECKC( pool_itr->status == pool_status::enabled, err::STATUS_ERROR, "pool not enabled, status:" + pool_itr->status.to_string() )

            CHECKC( pool_itr->asset_contract == get_first_receiver(), err::DATA_MISMATCH, "issue asset contract mismatch" );
            // CHECKC( pool_itr->asset_symbol == quantity.symbol, err::SYMBOL_MISMATCH, "issue asset symbol mismatch" );
            CHECKC( pool_itr->owner == from,  err::NO_AUTH, "not authorized" );

            auto new_nft_id = pool_itr->last_nft_id + 1;

            for ( auto i = 0; i < quantity.amount ; i++ ){
            
                nft_t::tbl_t nft( get_self(), pool_id);
                nft.emplace( _self, [&]( auto& row ) {
                    row.id                 = new_nft_id;
                    row.asset_contract     = get_first_receiver();
                    row.asset_symbol       = quantity.symbol;              
                    //row.owner              = from ;
                    //row.target_nft         = quantity;
                    //row.status             = nft_status::unsealed;
                    //row.create_at          = now;
                    //row.updated_at         = now;
                });

                new_nft_id ++;
            }

            auto now = current_time_point();
                pool.modify( pool_itr, same_payer, [&]( auto& row ) {
                    row.total_blindboxes       += quantity.amount;
                    row.unsealed_blindboxes    += quantity.amount;
                    row.last_nft_id             = new_nft_id;
                    row.updated_at              = now;
                });

        }
    }else if ( memo_params[0] == "open" ){

        nasset quantity = assets[0];
        
        CHECKC( quantity.amount == 1, err::NOT_POSITIVE, "quantity must be 1" )

        CHECKC( memo_params.size() == 2, err::MEMO_FORMAT_ERROR, "ontransfer:issue params size of must be 2")
        auto pool_id            = std::stoul(string(memo_params[1]));

        pool_t::tbl_t pool( get_self() ,get_self().value );
        auto pool_itr           = pool.find( pool_id );
        CHECKC( pool_itr != pool.end(), err::RECORD_NOT_FOUND, "pool not found: " + to_string(pool_id) )
        CHECKC( pool_itr->status == pool_status::enabled, err::STATUS_ERROR, "pool not enabled, status:" + pool_itr->status.to_string() )

        CHECKC( pool_itr->asset_contract == get_first_receiver(), err::DATA_MISMATCH, "issue asset contract mismatch" );
        CHECKC( pool_itr->asset_symbol == quantity.symbol, err::SYMBOL_MISMATCH, "blindbox asset symbol mismatch" );
        //CHECKC( pool_itr->owner == from,  err::NO_AUTH, "not authorized" );
        CHECKC( pool_itr->unsealed_blindboxes >= quantity.amount ,  err::OVERSIZED, "Remaining blind box:" + to_string( pool_itr->unsealed_blindboxes) );
        CHECKC( _gstate.max_sealed_number >= quantity.amount , err::MISC, "Maximum opening at one time: " + to_string( _gstate.max_sealed_number ));

        auto p = pool.get( pool_id );

        _rand_nft( p , from);

        auto now = current_time_point();
        pool.modify( pool_itr, same_payer, [&]( auto& row ) {
            row.unsealed_blindboxes     -= quantity.amount;
            row.sealed_blindboxes       += quantity.amount;
            row.received_blindbox       += quantity;
            row.updated_at              = now;
        });
    }
}

// void blindbox::test(){

//     // auto rand = _rand(10, 1, get_self(),1 );
//     // nft_t::tbl_t nft( get_self(), 1);
//     // auto itr = nft.begin();
//     // advance(itr ,rand - 1 );  
//     // CHECK( false, "nft :" + to_string( itr->id ));

//     pool_t::tbl_t pool( get_self() ,get_self().value );
//     auto pool_itr           = pool.find( 4 );
//     auto p = pool.get( 4 );
//     _rand_nft( p , get_self());
//     auto now = current_time_point();
//     pool.modify( pool_itr, same_payer, [&]( auto& row ) {
//         row.unsealed_blindboxes     -= 1;
//         row.sealed_blindboxes       += 1;
//         //row.received_blindbox       += 1;
//         row.updated_at              = now;
//     });

//     // vector<uint64_t> num = { 1, 2,3,4,5,6,7,8,9,10 };

//     // uint64_t max = 10;
//     // auto itr = num.begin();

//     // for ( auto i = 0 ;i < num.size();  i ++){ 

//     //     uint64_t rand = _rand( max , 1 , get_self(), i);
//     //     advance(itr ,rand - 1 );
//     //     get_nft_t::tbl_t getnfts( get_self(), get_self().value);
//     //     auto nft_id = getnfts.available_primary_key();
//     //     getnfts.emplace( get_self(), [&]( auto& row){

//     //         row.id = nft_id;
//     //         row.nft_id = *itr;
//     //     });
           
//     //     max --;
//     // }
// }

void blindbox::_rand_nft( const pool_t& pool , const name& owner){

    nft_t::tbl_t nft( get_self(), pool.id);
    uint64_t rand = _rand( pool.unsealed_blindboxes , 1 , owner, pool.id);
    auto itr = nft.begin();
    advance( itr , rand - 1 );
    //CHECK( false, to_string(rand)); 
    // get_nft_t::tbl_t getnfts( get_self(), get_self().value);
    // auto nft_id = getnfts.available_primary_key();
    // getnfts.emplace( get_self(), [&]( auto& row){

    //     row.id = nft_id;
    //     row.nft_id = itr->id;
    // });
    vector<nasset> quants = { nasset(1,itr->asset_symbol) };
    TRANSFER( itr->asset_contract, owner, quants , std::string("get blindbox"));
    nft.erase( itr );
 
}

// void blindbox::_rand_nft( const pool_t& pool , const name& owner, const uint64_t& count){

//     nft_t::tbl_t nft( get_self(), pool.id);

//     uint64_t max_itr = pool.unsealed_blindboxes;
    
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