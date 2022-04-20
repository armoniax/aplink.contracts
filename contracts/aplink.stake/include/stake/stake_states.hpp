 #pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include <deque>
#include <optional>
#include <string>
#include <map>
#include <set>
#include <type_traits>

namespace amax {

using namespace std;
using namespace eosio;

#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

static constexpr eosio::name active_perm{"active"_n};
static constexpr eosio::name SYS_BANK{"eosio.token"_n};

static constexpr symbol APL_SYMBOL              = SYMBOL("APL", 4);
static constexpr name   APL_BANK                { "aplink.token"_n };   //NTT token

// crypto assets
static constexpr symbol   CNYD_SYMBOL           = SYMBOL("CNYD", 6);
static constexpr symbol   STAKE_SYMBOL          = CNYD_SYMBOL;

#define STAKE_TBL [[eosio::table, eosio::contract("stake")]]

struct [[eosio::table("global"), eosio::contract("stake")]] global_t {
    asset       first_level_reward ;        //= ASSET(200.0000, APL_SYMBOL);
    asset       second_level_reward;        // = ASSET(100.000000, APL_SYMBOL);
    asset       stake_reward;               //  = ASSET(500.000000, APL_SYMBOL);
    uint64_t    stake_days = 21;            // the amount hold will be unfrozen upon expiry
    asset       stake_amount;               //= ASSET(1000.0000, CNYD_SYMBOL);
    uint8_t     status;                     // = 0;
    name        admin;                      // default is contract self
    bool        initialized = false; 

    EOSLIB_SERIALIZE( global_t, (first_level_reward)(second_level_reward)
                                (stake_reward)(stake_days)
                                (stake_amount)(status)(admin)(initialized)
    )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;


enum class reward_type_t: uint8_t {
    NONE                        = 0,
    STAKING_REWRARD             = 1,
    FIRST_LEVEL_REWARD          = 11,
    SECOND_LEVEL_REWARD         = 12
};

enum class staking_status_t: uint8_t {
    STAKING                 = 0,
    RELEASED                = 1
};

/**
 * buy/sell deal
 *
 */
struct STAKE_TBL staking_t {
    name owner;              // maker, merchant
    asset amount;                   // order price, deal price
    uint64_t stake_days;
    time_point_sec created_at;      // create time at
    time_point_sec expired_time;     // expire time at
    uint8_t status;
    staking_t() {}

    staking_t(const name& o): owner(o) {}
    uint64_t primary_key() const { return owner.value; }
    uint64_t scope() const { return 0; }

    EOSLIB_SERIALIZE(staking_t, (owner)(amount)(stake_days)
                                (created_at)(expired_time)(status))
};

/**
 * deposit log
 */
struct STAKE_TBL reward_t {
    uint64_t id = 0;            // PK: available_primary_key, auto increase
    name owner;                 // owner
    uint8_t type;               // reward_type_t
    asset amount;               // maybe positive(plus) or negative(minus)
    time_point_sec created_at;  // created time

    reward_t() {}
    reward_t(uint64_t i): id(i) {}
    uint64_t primary_key() const { return id; }
    uint64_t scope() const { return  0; }
    uint64_t by_owner()     const { return owner.value; }

    typedef eosio::multi_index
    <"staking"_n, reward_t,
        indexed_by<"owner"_n,   const_mem_fun<reward_t, uint64_t, &reward_t::by_owner> >
    > table_t;

    EOSLIB_SERIALIZE(reward_t, (id)(owner)(type)(amount)(created_at))
};

} // STAKE
