#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>

#include <string>

namespace amax {

using std::string;
using std::vector;

using namespace eosio;

constexpr name active_perm = "active"_n;

#define TRANSFER_N(bank, to, quants, memo) \
    {	ntoken::transfer_action act{ bank, { {_self, active_perm} } };\
			act.send( _self, to, quants , memo );}


struct nsymbol {
    uint32_t id;
    uint32_t parent_id;

    nsymbol() {}
    nsymbol(const uint32_t& i): id(i),parent_id(0) {}
    nsymbol(const uint32_t& i, const uint32_t& pid): id(i), parent_id(pid) {}

    friend bool operator==(const nsymbol&, const nsymbol&);
    bool is_valid()const { return( id > parent_id ); }
    uint64_t raw()const { return( (uint64_t) parent_id << 32 | id ); }

    EOSLIB_SERIALIZE( nsymbol, (id)(parent_id) )
};

bool operator==(const nsymbol& symb1, const nsymbol& symb2) {
    return( symb1.id == symb2.id && symb1.parent_id == symb2.parent_id );
}

struct nasset {
    int64_t         amount;
    nsymbol         symbol;

    nasset() {}
    nasset(const uint32_t& id): symbol(id), amount(0) {}
    nasset(const uint32_t& id, const uint32_t& pid): symbol(id, pid), amount(0) {}
    nasset(const uint32_t& id, const uint32_t& pid, const int64_t& am): symbol(id, pid), amount(am) {}
    nasset(const int64_t& amt, const nsymbol& symb): amount(amt), symbol(symb) {}

    friend bool operator==(const nasset& n1, const nasset& n2) {
        return( n1.symbol == n2.symbol && n1.amount == n2.amount );
    }

    nasset& operator+=(const nasset& quantity) {
        check( quantity.symbol.raw() == this->symbol.raw(), "nsymbol mismatch");
        this->amount += quantity.amount; return *this;
    }
    nasset& operator-=(const nasset& quantity) {
        check( quantity.symbol.raw() == this->symbol.raw(), "nsymbol mismatch");
        this->amount -= quantity.amount; return *this;
    }

    bool is_valid()const { return symbol.is_valid(); }

    EOSLIB_SERIALIZE( nasset, (amount)(symbol) )
};


/**
 * The `amax.ntoken` sample system contract defines the structures and actions that allow users to create, issue, and manage tokens for AMAX based blockchains. It demonstrates one way to implement a smart contract which allows for creation and management of tokens. It is possible for one to create a similar contract which suits different needs. However, it is recommended that if one only needs a token with the below listed actions, that one uses the `amax.ntoken` contract instead of developing their own.
 *
 * The `amax.ntoken` contract class also implements two useful public static methods: `get_supply` and `get_balance`. The first allows one to check the total supply of a specified token, created by an account and the second allows one to check the balance of a token for a specified account (the token creator account has to be specified as well).
 *
 * The `amax.ntoken` contract manages the set of tokens, accounts and their corresponding balances, by using two internal multi-index structures: the `accounts` and `stats`. The `accounts` multi-index table holds, for each row, instances of `account` object and the `account` object holds information about the balance of one token. The `accounts` table is scoped to an eosio account, and it keeps the rows indexed based on the token's symbol.  This means that when one queries the `accounts` multi-index table for an account name the result is all the tokens that account holds at the moment.
 *
 * Similarly, the `stats` multi-index table, holds instances of `currency_stats` objects for each row, which contains information about current supply, maximum supply, and the creator account for a symbol token. The `stats` table is scoped to the token symbol.  Therefore, when one queries the `stats` table for a token symbol the result is one single entry/row corresponding to the queried symbol token if it was previously created, or nothing, otherwise.
 */
class [[eosio::contract("amax.ntoken")]] ntoken : public contract {
   public:
      using contract::contract;

	/**
	 * @brief Transfers one or more assets.
	 *
    * This action transfers one or more assets by changing scope.
    * Sender's RAM will be charged to transfer asset.
    * Transfer will fail if asset is offered for claim or is delegated.
    *
    * @param from is account who sends the asset.
    * @param to is account of receiver.
    * @param assetids is array of assetid's to transfer.
    * @param memo is transfers comment.
    * @return no return value.
    */
   ACTION transfer( name from, name to, vector< nasset >& assets, string memo );
   using transfer_action = action_wrapper< "transfer"_n, &ntoken::transfer >;

};
} //namespace amax
