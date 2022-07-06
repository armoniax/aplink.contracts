#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>
#include <algorithm>


using namespace std;
using namespace eosio;

namespace otc
{

    using std::pair;
    using std::string;

    class [[eosio::contract("otcswap")]] otcswap : public contract
    {
    public:

        ACTION setrates(const vector<pair<uint64_t, double>> rates);

        /**
         * ontransfer, trigger by recipient of transfer()
         */

        [[eosio::on_notify("amax.arc::transfer")]] 
        void ontransfer(name from, name to, asset quantity, string memo);

        /**
         * @brief Increase the balance swap quotes of integral
         * 
         * @param user update user
         * @param fee order fee
         * @param quantity order trade quantity
         */
        [[eosio::action]] 
        void settleto(const name &user, const asset& fee,  asset quantity);
     
        /**
         * set conf contract by admin
         * @param admin account can update settle rates
         * @param settle_contract can submit settle data
         */
        [[eosio::action]]
        void setadmin(const name& admin, const name& settle_contract);

        using settleto_action = eosio::action_wrapper<"settleto"_n, &otcswap::settleto>;
    };

}
