#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>

using namespace eosio;


namespace otc {

using std::string;

class [[eosio::contract("otc.settle")]] settle : public contract {    

public:
    using contract::contract;

    [[eosio::action]]
    void deal(const uint64_t& deal_id,
                const name& merchant, 
                const name& user, 
                const asset& quantity, 
                const asset& fee,
                const uint8_t& arbit_status, 
                const time_point_sec& start_at, 
                const time_point_sec& end_at);

    using deal_action = eosio::action_wrapper<"grow"_n, &settle::deal>;
};
}