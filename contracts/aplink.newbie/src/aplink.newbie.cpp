#include "aplink.newbie.hpp"
#include "utils.hpp"
#include "eosio.token/eosio.token.hpp"

void newbie::claimreward(const name& newbie)
{
    require_auth( newbie );

    CHECK( _gstate.enable, "not enabled" )
    
    claim_t claim(newbie);
    CHECK( !_db.get(claim), "newbie reward already claimed by: " + newbie.to_string() )
    TRANSFER( _gstate.contract_name, newbie, _gstate.newbie_reward, "newbie reward" )

    claim.claimed_at = current_time_point();
    _db.set( claim );

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
