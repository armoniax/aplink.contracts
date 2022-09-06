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

#define BLINDBOX [[eosio::table, eosio::contract("nftone.blindbox")]]
#define BLINDBOX_NAME(name) [[eosio::table(name), eosio::contract("nftone.blindbox")]]

struct BLINDBOX_NAME("global") global_t {

    uint64_t  max_sealed_number = 100;
    uint64_t  last_pool_id  = 0;

    EOSLIB_SERIALIZE( global_t, (last_pool_id) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

namespace pool_status {
    static constexpr eosio::name none               = "none"_n;
    static constexpr eosio::name enabled            = "enabled"_n;
    static constexpr eosio::name disabled           = "disabled"_n;
};

namespace nft_status {
    static constexpr eosio::name none               = "none"_n;
    static constexpr eosio::name sealed             = "sealed"_n;
    static constexpr eosio::name unsealed           = "unsealed"_n;
};

struct BLINDBOX pool_t {
    uint64_t            id = 0;                     //PK
    name                owner;                      //pool owner
    string              title;                      //poo; title: <=64 chars
    name                asset_contract;             //blindbox asset issuing contract (ARC20)
    nsymbol             asset_symbol;               //blindbox NFT Asset Symbol, E.g. [10001, 0]
    uint64_t            total_blindboxes = 0;
    uint64_t            unsealed_blindboxes = 0;         
    uint64_t            sealed_blindboxes = 0;              
    nasset              received_blindbox;            
    name                status = pool_status::none; //status, see plan_status_t
    uint64_t            last_nft_id = 0;

    time_point_sec      created_at;                 //creation time (UTC time)
    time_point_sec      updated_at;                 //update time: last updated at
    time_point_sec      opended_at;                 //opend time: opening time of blind box 
    uint64_t primary_key() const { return id; }

    uint128_t by_owner() const { return (uint128_t)owner.value << 64 | (uint128_t)id; }

    typedef eosio::multi_index<"pools"_n, pool_t,
        indexed_by<"owneridx"_n,  const_mem_fun<pool_t, uint128_t, &pool_t::by_owner> >
    > tbl_t;

    EOSLIB_SERIALIZE( pool_t, (id)(owner)(title)(asset_contract)(asset_symbol)(total_blindboxes)(unsealed_blindboxes)
                              (sealed_blindboxes)(received_blindbox)(status)(last_nft_id)(created_at)(updated_at)(opended_at) )

};

// scope = pool_id
struct BLINDBOX nft_t {

    uint64_t        id = 0;                       //PK, unique within the contract
    name            asset_contract;
    nsymbol         asset_symbol;                 

    uint64_t primary_key() const { return id; }

    typedef eosio::multi_index<"nfts"_n, nft_t
    > tbl_t;

    EOSLIB_SERIALIZE( nft_t,   (id)(asset_contract)(asset_symbol)
                               )
};
struct BLINDBOX get_nft_t{

    uint64_t   id = 0;
    uint64_t   nft_id ;

    uint64_t primary_key() const { return id; }

    typedef eosio::multi_index<"getnfts"_n, get_nft_t
    > tbl_t;

    EOSLIB_SERIALIZE( get_nft_t,   (id)(nft_id)
                               )
};
// scope = contract self
// table for pool creator accounts
struct BLINDBOX account {
    name    owner;
    uint64_t last_pool_id;

    uint64_t primary_key()const { return owner.value; }

    typedef multi_index_ex< "payaccounts"_n, account > tbl_t;

    EOSLIB_SERIALIZE( account,  (owner)(last_pool_id) )
};

} }