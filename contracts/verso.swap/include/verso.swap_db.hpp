#pragma once

#include <amax.ntoken/amax.ntoken.hpp>
#include "commons/wasm_db.hpp"

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

using namespace eosio;
using namespace std;
using std::string;

// using namespace wasm;
#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

static constexpr uint64_t DAY_SECONDS           = 24 * 60 * 60;
enum class err: uint8_t {
   NONE                 = 0,
   RECORD_NOT_FOUND     = 1,
   RECORD_EXISTING      = 2,
   SYMBOL_MISMATCH      = 4,
   PARAM_ERROR          = 5,
   MEMO_FORMAT_ERROR    = 6,
   PAUSED               = 7,
   NO_AUTH              = 8,
   NOT_POSITIVE         = 9,
   NOT_STARTED          = 10,
   OVERSIZED            = 11,
   TIME_EXPIRED         = 12,
   NOTIFY_UNRELATED     = 13,
   ACTION_REDUNDANT     = 14,
   ACCOUNT_INVALID      = 15,
   FEE_INSUFFICIENT     = 16,
   FIRST_CREATOR        = 17,
   STATUS_ERROR         = 18,
   RATE_OVERLOAD        = 19,
   DATA_MISMATCH        = 20,
   MISC                 = 255
};


namespace wasm { namespace db {

using namespace amax;

#define BLINDBOX [[eosio::table, eosio::contract("verso.swap")]]
#define BLINDBOX_NAME(name) [[eosio::table(name), eosio::contract("verso.swap")]]

struct BLINDBOX_NAME("global") global_t {

    uint64_t  max_exchange_num     = 100;
    uint64_t  last_pool_id          = 0;
    uint64_t  last_totality_id      = 0;

    EOSLIB_SERIALIZE( global_t, (max_exchange_num)(last_pool_id)(last_totality_id) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

namespace pool_status {
    static constexpr eosio::name none               = "none"_n;
    static constexpr eosio::name enabled            = "enabled"_n;
    static constexpr eosio::name disabled           = "disabled"_n;
};

namespace asset_type_t {
    static constexpr eosio::name none               = "none"_n;
    static constexpr eosio::name nft                = "nft"_n;
    static constexpr eosio::name token              = "token"_n;
};


namespace exchange_type_t {
    static constexpr eosio::name none               = "none"_n;
    static constexpr eosio::name random             = "random"_n;
    static constexpr eosio::name ergodic            = "ergodic"_n;
};

struct price_token {
    asset               price;
    asset               received;
    name                fee_receiver;
    EOSLIB_SERIALIZE( price_token,  (price)(received)(fee_receiver) )
};

struct price_nft{
    nasset             price;
    nasset             received;
    name               fee_receiver;
    EOSLIB_SERIALIZE( price_nft,  (price)(received)(fee_receiver) )
};


struct totality_t{

    uint64_t           id;
    uint64_t           total_amount;
    uint64_t           balance_amount;
    time_point_sec      created_at;                                         //creation time (UTC time)
    time_point_sec      updated_at;                                         //update time: last updated at

    uint64_t primary_key() const { return id; }

    typedef eosio::multi_index<"totalitys"_n, totality_t
    > tbl_t;

    EOSLIB_SERIALIZE( totality_t,  (id)(total_amount)(balance_amount)(created_at)(updated_at) )
};


struct BLINDBOX pool_t {
    uint64_t            id = 0;                                             //PK
    name                owner;                                              //pool owner
    string              title;                                              //pool title: <=64 chars
    name                asset_contract;                                     
    name                asset_type                  = asset_type_t::none;
    price_token         token_price;                                        
    price_nft           nft_price;
    name                blindbox_contract;                                  // 
    uint64_t            total_nft_amount            = 0;
    uint64_t            exchange_nft_amount         = 0;         
    uint64_t            not_exchange_nft_amount     = 0;       
    uint64_t            refund_nft_amount           = 0;   
    uint64_t            max_table_distance          = 0;                    // table advance max  
    name                exchange_type               = exchange_type_t::none;     
    name                status                      = pool_status::none;    //status, see plan_status_t
    uint64_t            last_nft_id                 = 0;
    uint64_t            totality_id                 = 0;                    //totality_t id
    time_point_sec      created_at;                                         //creation time (UTC time)
    time_point_sec      updated_at;                                         //update time: last updated at
    time_point_sec      opended_at;                                         //opend time: opening time of blind box
    time_point_sec      closed_at;                                          //close time: close time of blind box

    uint64_t primary_key() const { return id; }

    uint128_t by_owner() const { return (uint128_t)owner.value << 64 | (uint128_t)id; }

    typedef eosio::multi_index<"pools"_n, pool_t,
        indexed_by<"owneridx"_n,  const_mem_fun<pool_t, uint128_t, &pool_t::by_owner> >
    > tbl_t;

    EOSLIB_SERIALIZE( pool_t, (id)(owner)(title)(asset_contract)(asset_type)
                              (token_price)(nft_price)(blindbox_contract)(total_nft_amount)(exchange_nft_amount)
                              (not_exchange_nft_amount)(refund_nft_amount)(max_table_distance)(exchange_type)
                              (status)(last_nft_id)(totality_id)(created_at)(updated_at)(opended_at)(closed_at) )

};

// scope = pool_id
struct BLINDBOX nft_t {

    uint64_t        id = 0;                       //PK, unique within the contract
    name            asset_contract;
    nasset          quantity;                 

    uint64_t primary_key() const { return id; }

    typedef eosio::multi_index<"nfts"_n, nft_t
    > tbl_t;

    EOSLIB_SERIALIZE( nft_t,   (id)(asset_contract)(quantity)
                               )
};
// scope = contract self
// table for pool creator accounts
struct BLINDBOX account {
    name    owner;
    uint64_t last_pool_id;

    uint64_t primary_key()const { return owner.value; }

    typedef multi_index_ex< "accounts"_n, account > tbl_t;

    EOSLIB_SERIALIZE( account,  (owner)(last_pool_id) )
};

} }