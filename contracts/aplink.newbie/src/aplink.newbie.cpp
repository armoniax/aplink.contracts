#include <eosio/permission.hpp>
#include "aplink.newbie.hpp"
#include "utils.hpp"
#include <aplink.token/aplink.token.hpp>
#include <aplink.farm.hpp>

#define TRANSFER(bank, to, quantity, memo) \
    {	aplink::token::transfer_action act{ bank, { {_self, active_perm} } };\
			act.send( _self, to, quantity, memo );}


using allot_action = aplink::farm::allot_action;
#define ALLOT_APPLE(farm, land_id, to, quantity, memo) \
    {   allot_action(farm, { {_self, active_perm} }).send( \
            land_id, to, quantity , memo);}

void newbie::claimreward(const name& newbie)
{
    require_auth( newbie );
    CHECK( _gstate.enable, "not enabled" )
    CHECK( !aplink::token::account_exist(_gstate.aplink_token_contract, newbie, _gstate.newbie_reward.symbol.code()),
           "newbie reward already claimed by: " + newbie.to_string() )

    TRANSFER( _gstate.aplink_token_contract, newbie, _gstate.newbie_reward, "newbie reward" )
}

void newbie::rewardinvite(const name& to)
{
    require_auth( _gstate.aplink_token_contract );

    auto parent_inviter = get_account_creator( to );
    if(parent_inviter == SYS_ACCT) 
        return;
    ALLOT_APPLE( _gstate.apl_farm.contract, _gstate.apl_farm.land_id, parent_inviter, _gstate.apl_farm.parent_inviter_reward, "inviter reward")

    auto grand_parent_inviter = get_account_creator( parent_inviter );
    if(grand_parent_inviter == SYS_ACCT) 
        return;
    ALLOT_APPLE( _gstate.apl_farm.contract, _gstate.apl_farm.land_id, grand_parent_inviter, _gstate.apl_farm.grandparent_inviter_reward , "grand inviter reward")
}

void newbie::setstate(const bool& enable, const asset& newbie_reward, const name& aplink_token_contract)
{
    require_auth( _self );

    CHECK( newbie_reward.is_valid(), "invalid quantity");
    CHECK( newbie_reward.amount > 0, "reward_value must be positive");

    _gstate.enable = enable;
    _gstate.newbie_reward = newbie_reward;
    _gstate.aplink_token_contract = aplink_token_contract;
}

void newbie::recycledb(uint32_t max_rows) {
    require_auth( _self );
    claim_t::tbl_t claim_tbl(_self, _self.value);
    auto claim_itr = claim_tbl.begin();
    for (size_t count = 0; count < max_rows && claim_itr != claim_tbl.end(); count++) {
        claim_itr = claim_tbl.erase(claim_itr);
    }
}