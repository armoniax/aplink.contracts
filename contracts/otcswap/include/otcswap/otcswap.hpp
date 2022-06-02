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
    /**
     * The `amax.token` sample system contract defines the structures and actions that allow users to create, issue, and manage tokens for AMAX based blockchains. It demonstrates one way to implement a smart contract which allows for creation and management of tokens. It is possible for one to create a similar contract which suits different needs. However, it is recommended that if one only needs a token with the below listed actions, that one uses the `amax.token` contract instead of developing their own.
     *
     * The `amax.token` contract class also implements two useful public static methods: `get_supply` and `get_balance`. The first allows one to check the total supply of a specified token, created by an account and the second allows one to check the balance of a token for a specified account (the token creator account has to be specified as well).
     *
     * The `amax.token` contract manages the set of tokens, accounts and their corresponding balances, by using two internal multi-index structures: the `accounts` and `stats`. The `accounts` multi-index table holds, for each row, instances of `account` object and the `account` object holds information about the balance of one token. The `accounts` table is scoped to an eosio account, and it keeps the rows indexed based on the token's symbol.  This means that when one queries the `accounts` multi-index table for an account name the result is all the tokens that account holds at the moment.
     *
     * Similarly, the `stats` multi-index table, holds instances of `currency_stats` objects for each row, which contains information about current supply, maximum supply, and the creator account for a symbol token. The `stats` table is scoped to the token symbol.  Therefore, when one queries the `stats` table for a token symbol the result is one single entry/row corresponding to the queried symbol token if it was previously created, or nothing, otherwise.
     */
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

        ACTION setrates(const vector<pair<uint64_t, double>> rates);

     

        /**
         * ontransfer, trigger by recipient of transfer()
         * @param memo - memo format:
         * 1. charge:${receiver} Eg: "charge:receiver1234"
         *    @param receiver - charge to receiver, send to &from if empty memo
         *
         *    transfer() params:
         *    @param from - default eletricity receiver
         *    @param to   - must be contract self
         *    @param quantity - issued quantity
         */

        [[eosio::on_notify("amax.arc::transfer")]] void ontransfer(name from, name to, asset quantity, string memo);


        /**
         * @require run by taker only
         * Increase the balance of integral
         */
        [[eosio::action]] void settleto(const name &to,const asset& fee_pct,  asset quantity);
     
        /**
         * set conf contract by admin
         * @param conf_contract conf contract
         * @note require admin auth
         */
        //[[eosio::action]] void setconf(const name &conf_contract);


        using settleto_action = eosio::action_wrapper<"settleto"_n, &otcswap::settleto>;

    private:
        global_singleton _global;
        global_t _gstate;
        dbc                 _db;
        
    };

}
