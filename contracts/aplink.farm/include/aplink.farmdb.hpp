#pragma once

#include "wasm_db.hpp"

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

using namespace eosio;
using namespace std;
using std::string;

using namespace wasm;
#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

static constexpr uint32_t MAX_CONTENT_SIZE        = 64;

namespace wasm { namespace db {

#define FARM_TBL [[eosio::table, eosio::contract("aplink.farm")]]
#define FARM_TBL_NAME(name) [[eosio::table(name), eosio::contract("aplink.farm")]]

struct FARM_TBL_NAME("global") global_t {
    name lord;
    EOSLIB_SERIALIZE( global_t, (lord) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;


enum land_status_t {
    LAND_NONE          = 0,
    LAND_ENABLED       = 1,
    LAND_DISABLED      = 2
};

struct FARM_TBL land_t {
    uint64_t        id;
    name            farmer;                     //land farmer
    string          title;                      //land title: <=64 chars
    string          uri;                        //land uri: <=64 chars
    time_point      crop_start_at;              //customer can crop at
    time_point      crop_end_at;                //customer stop crop at
    asset           seeds;
    uint8_t         status = LAND_ENABLED;         //status of land, see land_status_t
    time_point      created_at;                 //creation time (UTC time)
    time_point      updated_at;                 //update time: last updated atuint8_t  
    
    land_t() {}
    land_t(const uint64_t& pid): id(pid) {}

    uint64_t primary_key() const { return id; }

    uint64_t by_updatedid() const { return ((uint64_t)updated_at.sec_since_epoch() << 32) | (id & 0x00000000FFFFFFFF); }
    uint128_t by_farmer() const { return (uint128_t)farmer.value << 64 | (uint128_t)id; }

    typedef eosio::multi_index<"lands"_n, land_t,
        indexed_by<"updatedid"_n,  const_mem_fun<land_t, uint64_t, &land_t::by_updatedid> >,
        indexed_by<"farmeridx"_n,  const_mem_fun<land_t, uint128_t, &land_t::by_farmer> >
    > idx_t;

    EOSLIB_SERIALIZE( land_t, (id)(farmer)(title)(uri)(crop_start_at)(crop_end_at)
                              (seeds)(status)(created_at)(updated_at) )

};
} }