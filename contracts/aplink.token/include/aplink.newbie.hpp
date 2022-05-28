#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>

using namespace eosio;

namespace aplink {

class [[eosio::contract("aplink.newbie")]] newbie : public contract {
public:
    using contract::contract;

    ACTION rewardinvite(const name& to);
    using rewardinvite_action = eosio::action_wrapper<"rewardinvite"_n, &newbie::rewardinvite>;

};

}