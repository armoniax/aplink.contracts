#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace amax {

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
   class [[eosio::contract("amax.token")]] token : public contract {
      public:
         using contract::contract;

         static bool is_blacklisted( const name& token_contract, const name& target ) {
            blackaccounts black_accts( token_contract, token_contract.value );
            return ( black_accts.find( target.value ) != black_accts.end() );
         }

      private:
      

         struct [[eosio::table]] blacklist_t {
            name     account;

            uint64_t primary_key()const { return account.value; }
         };

         typedef eosio::multi_index< "blacklist"_n, blacklist_t > blackaccounts;

   };

}
