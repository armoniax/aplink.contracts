#include <eosio.token/eosio.token.hpp>
#include <stake/stake.hpp>

using namespace eosio;
using namespace std;
using std::string;

namespace mgp {

using namespace std;
using namespace eosio;
using namespace wasm::safemath;

/**
 * This happens when a merchant decides to open sell orders
 */
void stake::deposit(name from, name to, asset quantity, string memo) {
    eosio::print("from: ", from, ", to:", to, ", quantity:" , quantity, ", memo:" , memo);
    if(_self == from ){
        return;
    }
    if (to != _self)
        return;
    check(_gstate.status == 0, "stake already closed");
    check(get_first_receiver() == SYS_BANK, "receiver is error: " + get_first_receiver().to_string());
    check(quantity.symbol == CNYD_SYMBOL, "transfer coin symbol error");
    check(_gstate.stake_amount == quanity, "stake amount is not matched");
   
    staking_t staking_raw(from);
    check(!_dbc.get( staking_raw ),"staking is already exsited.");

    staking_raw.amount = quantity;
    staking_raw.stake_days = _gstate.stake_days;
    staking_raw.created_at = time_point_sec(current_time_point());
    staking_raw.expired_time = time_point_sec(current_time_point()) + 24 * 60 * 60 * _gstate.stake_days;
    staking_raw.status = (uint8_t)staking_status_t::STAKING;
    _dbc.set(staking_raw);

    _token_transfer(first_reward, STAKING_REWRARD, _gstate.stake_reward);
   
    name first_reward = get_account_creator(from);
    _token_transfer(first_reward, FIRST_LEVEL_REWARD, _gstate.first_level_reward);

    name second_reward = get_account_creator(first_reward);
    if(second_reward) {
        _token_transfer(second_reward, SECOND_LEVEL_REWARD, _gstate.second_level_reward);   
    }
}

void stake::_token_transfer(const name& to, const uint8_t& type, const asset &quantity) {
    vector<permission_level> p;
    p.push_back(permission_level{get_self(), "active"_n }); 
    action(
        p, 
        APLINK_TOKEN, 
        "transfer"_n, 
        std::make_tuple(get_self(), to, quantity, std::string("memo") )
    ).send();
     _add_reward_log(to, type, quantity);
}

void stake::_add_reward_log(const name& owner, const uint8_t& type, const asset &quantity) {
    auto now = time_point_sec(current_time_point());
    reward_t::table_t stake_log_tbl(_self, _self.value);
    auto id = stake_log_tbl.available_primary_key();
    
    stake_log_tbl.emplace( _self, [&]( auto& row ) {
        row.id 					= id;
        row.owner 			    = owner;
        row.type			    = type;
        row.amount		        = amount;
        row.created_at		    = now;
    }); 
}

/**
 *  提取
 *
 */
void stake::redeem(const name& owner){
    require_auth( owner );

    staking_t staking_raw(owner);
    check(_dbc.get(staking_raw),"staking is not exsited.");

    check(staking_raw.status == (uint8_t)staking_status_t::STAKING, "staking is already redeem.");
    staking_raw.status = (uint8_t)staking_status_t::RELEASED;
    _dbc.set(staking_raw);

    TRANSFER( SYS_BANK, owner, staking_raw.amount, "redeem" );
}

}  //end of namespace:: mgpbpvoting
