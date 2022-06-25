#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>

namespace aplink {

using namespace eosio;
using namespace wasm::db;

class [[eosio::contract("aplink.farm")]] farm : public contract {
    public:
    using contract::contract;

    ACTION allot(const uint64_t& land_id, const name& customer, const asset& quantity, const string& memo);
    using allot_action = eosio::action_wrapper<"allot"_n, &farm::allot>;
};

};