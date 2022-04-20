#include "rewardnewbie.hpp"
#include "utils.hpp"
#include "eosio.token/eosio.token.hpp"

void rewardnewbie::claimreward(const name& newbie)
{
    require_auth( newbie );

    CHECK( _gstate.enable, "not enabled" )
    
    claim_t claim(newbie);
    CHECK( !_db.get(claim), "newbie reward already claimed by: " + newbie.to_string() )
    TRANSFER( APL_BANK, newbie, _gstate.newbie_reward, "newbie reward" )

    claim.claimed_at = current_time_point();
    _db.set( claim );

}

void rewardnewbie::setstate(const bool& enable, const asset& newbie_reward)
{
    require_auth( _self );

    CHECK( newbie_reward.is_valid(), "invalid quantity");
    CHECK( newbie_reward.amount > 0, "reward_value must be positive");
    
    _gstate.newbie_reward = newbie_reward;
    _gstate.enable = enable;
}
