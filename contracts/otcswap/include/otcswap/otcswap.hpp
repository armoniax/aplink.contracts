#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>
#include <algorithm>
#include "otcswapdb.hpp"


using namespace std;
using namespace eosio;
using namespace wasm::db;

namespace otc
{

    using std::pair;
    using std::string;

    class [[eosio::contract("otcswap")]] otcswap : public contract
    {
    public:
        using contract::contract;
        otcswap(eosio::name receiver, eosio::name code, datastream<const char *> ds) : _db(_self),contract(receiver, code, ds), _global(_self, _self.value)
        {
            if (_global.exists())
            {
                _gstate = _global.get();
            }
            else
            { // first init
                _gstate = global_t{};
                _gstate.admin = _self;
            }
        }

        ~otcswap() { _global.set(_gstate, get_self()); }

        ACTION setrates(const vector<balance_config> rates);

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

    private:
        global_singleton _global;
        global_t _gstate;
        dbc  _db;
        
    };

}
