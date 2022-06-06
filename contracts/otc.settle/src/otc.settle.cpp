#include "otc.settle.hpp"
#include <eosio/permission.hpp>
#include "utils.hpp"
#include "eosio.token/eosio.token.hpp"
#include "aplink.farm/aplink.farm.hpp"
#include "otcswap.hpp"

using namespace otc;
using namespace eosio;

#define GROW(bank, land_id, customer, quantity, memo) \
    {	aplink::farm::grow_action act{ bank, { {_self, active_perm} } };\
			act.send( land_id, customer, quantity , memo );}

#define SWAP_SETTLE(bank, user, fee, quantity) \
    {	otc::otcswap::settleto_action act{ bank, { {_self, active_perm} } };\
			act.send( user, fee, quantity );}

void settle::setadmin(const name& admin, const name& market, const name& swap){
    require_auth(get_self());
    CHECKC(is_account(admin), err::ACCOUNT_INVALID, "invalid account: " + admin.to_string());
    CHECKC(is_account(market), err::ACCOUNT_INVALID, "invalid account: " + market.to_string());
    CHECKC(is_account(swap), err::ACCOUNT_INVALID, "invalid account: " + swap.to_string());
    _gstate.admin = admin;
    _gstate.market = market;
    _gstate.swap = swap;
    _global.set(_gstate, get_self());
}

void settle::setconfig(const vector<level_config>& configs){
    require_auth(_gstate.admin);
    _gstate.level_config = configs;
    _global.set(_gstate, get_self());
}

void settle::setlevel(const name& user, uint8_t level){
    require_auth(_gstate.admin);
    CHECKC(level >=0, err::PARAM_ERROR, "level must be a positive number");
    CHECKC(level < _gstate.level_config.size(), err::PARAM_ERROR, "level must less than level: " + to_string(_gstate.level_config.size()-1));
    
    auto user_data = settle_t(user);
    _db.get(user_data);
    user_data.level = level;
    _db.set(user_data, _self);
}

void settle::deal(const uint64_t& deal_id,
                  const name& merchant, 
                  const name& user, 
                  const asset& quantity, 
                  const asset& fee, 
                  const uint8_t& arbit_status, 
                  const time_point_sec& start_at, 
                  const time_point_sec& end_at){
    require_auth(_gstate.market);
    CHECKC(is_account(merchant), err::ACCOUNT_INVALID, "invalid account: " + merchant.to_string());
    CHECKC(is_account(user), err::ACCOUNT_INVALID, "invalid account: " + user.to_string());
    CHECKC(_gstate.level_config.size()>0, err::UN_INITIALIZE, "level config hasn't set");
    CHECKC(end_at > start_at, err::PARAM_ERROR, "end time should later than start time");
    CHECKC(quantity.symbol == CASH_SYMBOL && fee.symbol == CASH_SYMBOL, err::PARAM_ERROR, "quantity and fee symbol invalid");

    auto user_settle_data = settle_t(user);
    auto merchant_settle_data = settle_t(merchant);
    _db.get(user_settle_data);
    _db.get(merchant_settle_data);

    if(arbit_status != 0) {
        user_settle_data.sum_arbit_count += 1;
        merchant_settle_data.sum_arbit_count += 1;

        _db.set(merchant_settle_data, _self);
        _db.set(user_settle_data, _self);
        return;
    }
    
    SWAP_SETTLE(_gstate.swap, user, fee, quantity);
    
    merchant_settle_data.sum_deal += quantity.amount;
    merchant_settle_data.sum_fee += fee.amount;
    merchant_settle_data.sum_deal_count += 1;
    merchant_settle_data.sum_deal_time += (end_at - start_at).to_seconds();

    user_settle_data.sum_deal += quantity.amount;
    user_settle_data.sum_fee += fee.amount;
    user_settle_data.sum_deal_count += 1;
    user_settle_data.sum_deal_time += (end_at - start_at).to_seconds();

    _db.set(merchant_settle_data, _self);
    _db.set(user_settle_data, _self);
    
    // only record user data for parent
    auto creator = get_account_creator(user);
    auto creator_data = settle_t(creator);
    _db.get(creator_data);
    creator_data.sum_child_deal += quantity.amount;
    for(int j = _gstate.level_config.size(); j>0; j--){
        auto config = _gstate.level_config.at(j-1);
        if(creator_data.level >= j) break;
        if(creator_data.sum_child_deal >= config.sum_limit){
            creator_data.level = j-1;
            break;
        }
    }
    _db.set(creator_data, _self);

    auto config = _gstate.level_config.at(min(creator_data.level, uint8_t(_gstate.level_config.size()-1)));
    if(config.cash_rate + config.score_rate == 0) return;
    auto rewards = reward_t::idx_t(_self, _self.value);
    auto pid = rewards.available_primary_key();
    auto reward = reward_t(pid);
    reward.deal_id = deal_id;
    reward.reciptian = creator;
    reward.cash = asset(fee.amount*config.cash_rate/RATE_BOOST, CASH_SYMBOL);
    reward.score = asset(fee.amount*config.score_rate/RATE_BOOST, SCORE_SYMBOL);
    reward.created_at = time_point_sec(current_time_point());

    _db.set(reward, _self);

    if(APLINK_FARM_LAND > 0){
        GROW(APLINK_FARM, APLINK_FARM_LAND, user, asset(fee.amount, APLINK_SYMBOL), "metabalance farming: "+to_string(deal_id));
    }
}


void settle::pick(const name& reciptian, vector<uint64_t> rewards){
    require_auth(reciptian);

    auto cash_quantity = asset(0, CASH_SYMBOL);
    auto score_quantity = asset(0, SCORE_SYMBOL);
    auto current = time_point_sec(current_time_point());

    CHECKC( rewards.size() <= 20, err::OVERSIZED, "rewards too long, expect length 20" );

    for (int i = 0; i<rewards.size(); ++i)
	{
        auto reward_id = rewards[i];
        auto reward = reward_t(reward_id);
        CHECKC( _db.get( reward ), err::RECORD_NOT_FOUND, "reward not found: " + to_string(reward_id));
        CHECKC( reward.reciptian == reciptian, err::ACCOUNT_INVALID, "account invalid");
        
        cash_quantity += reward.cash;
        score_quantity += reward.score;
        _db.del(reward);
	}

    if(cash_quantity.amount > 0) TRANSFER( CASH_BANK, reciptian, cash_quantity, "metabalance rewards");
    if(score_quantity.amount > 0) TRANSFER( SCORE_BANK, reciptian, score_quantity, "metabalance rewards");
}
