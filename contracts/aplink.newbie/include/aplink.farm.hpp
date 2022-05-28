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

    ACTION grow(const name& to, const uint64_t& land_id, const asset& quantity);
    using grow_action = eosio::action_wrapper<"grow"_n, &farm::grow>;
};

};