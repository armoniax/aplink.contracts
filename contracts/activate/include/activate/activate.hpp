#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/singleton.hpp>

#include <eosio/binary_extension.hpp>
#include <eosio/privileged.hpp>
#include <eosio/producer_schedule.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include <string>
#include <vector>
#include <deque>
#include <optional>
#include <type_traits>
namespace eosio {

    using std::string;
    using std::vector;
    

    class [[eosio::contract("activate")]] activate : public contract {
    

    public:
        using contract::contract;

        activate( name s, name code, datastream<const char*> ds );

        static constexpr name aplink_token = name("aplink.token");

        [[eosio::action]]
        void reward(const name& to);

        [[eosio::action]]
        void setstate(const bool&  enable, const asset& reward_value);

        // [[eosio::action]]
        // void deleteacnt(const name& to);

        using reward_action = eosio::action_wrapper<"reward"_n, &activate::reward>;
        using setstate_action = eosio::action_wrapper<"setstate"_n, &activate::setstate>;
        // using deleteacnt_action = eosio::action_wrapper<"deleteacnt"_n, &activate::deleteacnt>;

    private:

        struct [[eosio::table]] account {
            name     user;
            uint64_t     primary_key()const { return user.value; }
        };

        struct [[eosio::table("global")]] state {
            state(){}

            bool     enable=false;
            asset    reward;

            EOSLIB_SERIALIZE( state, (enable)(reward))
        };

        typedef eosio::singleton< "global"_n, state >   state_singleton;
        typedef eosio::multi_index< "accounts"_n, account > accounts;

        state_singleton   _state;
    };

}