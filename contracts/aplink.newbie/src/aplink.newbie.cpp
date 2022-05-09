#include "aplink.newbie.hpp"
#include "utils.hpp"
#include <aplink.token/aplink.token.hpp>

#define TRANSFER(bank, to, quantity, memo) \
    {	aplink::token::transfer_action act{ bank, { {_self, active_perm} } };\
			act.send( _self, to, quantity , memo );}

void newbie::claimreward(const name& newbie)
{
    require_auth( newbie );
    CHECK( _gstate.enable, "not enabled" )
    CHECK( !aplink::token::account_exist(_gstate.contract_name, newbie, _gstate.newbie_reward.symbol.code()),
           "newbie reward already claimed by: " + newbie.to_string() )

    TRANSFER( _gstate.contract_name, newbie, _gstate.newbie_reward, "newbie reward" )
}

void newbie::setstate(const bool& enable, const asset& newbie_reward, const name& contract_name)
{
    require_auth( _self );

    CHECK( newbie_reward.is_valid(), "invalid quantity");
    CHECK( newbie_reward.amount > 0, "reward_value must be positive");

    _gstate.newbie_reward = newbie_reward;
    _gstate.enable = enable;
    _gstate.contract_name = contract_name;
}
