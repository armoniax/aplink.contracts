#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>

namespace apollo {

using std::string;

/**
 * The `apollo.token` is NFT-1155 token contract
 * 
 */
class [[eosio::contract("apollo.token")]] token : public contract {
   public:
      using contract::contract;

   /*
   * Update version.
   *
   * This action updates the version string of this SimpleAssets deployment for 3rd party wallets,
               * marketplaces, etc.
   *
   * @param version is version number of SimpleAssetst deployment.
   * @return no return value.
   */
   ACTION updatever( string version );
   using updatever_action = action_wrapper< "updatever"_n, &SimpleAssets::updatever >;

   /*
	 * Change author.
	 *
	 * This action change author. This action replaces author name
	 *
	 * @return no return value.
	 */ 
	ACTION changeauthor( name author, name newauthor, name owner, vector<uint64_t>& assetids, string memo );
	using changeauthor_action = action_wrapper< "changeauthor"_n, &SimpleAssets::changeauthor >;

	/*
    * Transfers one or more assets.
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
   ACTION transfer( name from, name to, vector< uint64_t >& assetids, string memo );
   using transfer_action = action_wrapper< "transfer"_n, &SimpleAssets::transfer >;

   private:
      struct token_info_t {
         uint64_t    id;           //token ID
         string      name;         //token name
         string      symobl;       //token symbol
         string      uri;          //token metadata uri

         token_info_t(const uint64_t& i, const string& name, const string& sy, const string& u):
         id(i), name(n), symobl(sy), uri(u) {};

         // EOSLIB_SERIALIZE( token_info_t, (id)(name)(symbol)(uri) )
      };

      struct [[eosio::table]] nftoken_t {
         token_info_t   token_info;
         name           owner;
         uint64_t       balance;       //token count
         
         uint64_t scope() const { return 0; }
         uint64_t primary_key() const { return token.id; }

         nftoken_t() {}
         nftoken_t(const uint64_t& i, const string& n, const string& sy, const string& u, const name& o, const uint64_t& b):
         token_info(i,n,sy,u),owner(o),balance(b) {}

         EOSLIB_SERIALIZE( nftoken_t, (owner)(token_info)(balance) )

         typedef eosio::multi_index< "nftokens"_n, nftoken_t > tbl_t;
      }
};
} //namespace apollo
