#include "otc.settle.hpp"
#include <eosio/permission.hpp>
#include "utils.hpp"
#include "eosio.token/eosio.token.hpp"

using namespace otc;
using namespace eosio;


void settle::setadmin(const name& admin, const name& market){
    require_auth(get_self());
    _gstate.admin = admin;
    _gstate.market = market;
    _global.set(_gstate, get_self());
}

void settle::setconfig(const vector<tuple<uint32_t, uint16_t, uint16_t>>& configs){
    require_auth(_gstate.admin);
    _gstate.level_config = configs;
    _global.set(_gstate, get_self());
}

void settle::deal(const name& merchant, 
                  const name& user, 
                  const asset& quantity, 
                  const asset& fee, 
                  const uint8_t& arbit_staus, 
                  const time_point_sec& start_at, 
                  const time_point_sec& end_at){
    require_auth(_gstate.market);
    CHECKC(is_account(merchant), err::ACCOUNT_INVALID, "invalid account: " + merchant.to_string());
    CHECKC(is_account(user), err::ACCOUNT_INVALID, "invalid account: " + user.to_string());
    CHECKC(_gstate.level_config.size()>0, err::UN_INITIALIZE, "level config hasn't set");
    
    auto user_settle_data = settle_t(user);
    auto merchant_settle_data = settle_t(merchant);
    _db.get(user_settle_data);
    _db.get(merchant_settle_data);

    for(int i = 0; i < 2; i++){
        auto settle_data = i == 0? merchant_settle_data:user_settle_data;
        if(arbit_staus == 0){
            settle_data.sum_deal += quantity.amount;
            settle_data.sum_fee += fee.amount;
            settle_data.sum_deal_count += 1;
            settle_data.sum_deal_time += end_at-start_at;
        }
        else{
            settle_data.sum_arbit_count += 1;
        }
        _db.set(settle_data, _self);
    }

    //only record user data for parent
    auto parent = get_account_creator(user);
    auto parent_data = settle_t(parent);
    _db.get(parent_data);
    parent_data.sum_child_deal += quantity.amount;
    for(int j = _gstate.level_config.size(); j>0; j--){
        auto config = _gstate.level_config.at(j-1);
        if(parent_data.level >= j) break;
        if(parent_data.sum_child_deal >= get<0>(config)){
            parent_data.level = j;
            break;
        }
    }
    _db.set(parent_data, _self);

    auto apples = reward_t::idx_t(_self, _self.value);
    auto pid = apples.available_primary_key();
    auto apple = reward_t(pid);
    apple.reciptian = user;
    auto config = _gstate.level_config.at(min(user_settle_data.level, uint8_t(_gstate.level_config.size()-1)));
    apple.cash = asset(fee.amount*get<1>(config)/RATE_BOOST, CASH_SYMBOL);
    apple.cash = asset(fee.amount*get<2>(config)/RATE_BOOST, CASH_SYMBOL);
    apple.created_at = time_point_sec(current_time_point());

    _db.set(apple, _self);
            
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
        CHECKC( _db.get( reward ), err::RECORD_NOT_FOUND, "apple not found: " + to_string(reward_id));
        CHECKC( reward.reciptian == reciptian, err::ACCOUNT_INVALID, "account invalid");
        
        cash_quantity += reward.cash;
        score_quantity += reward.score;
        _db.del(reward);
	}

    if(cash_quantity.amount > 0) TRANSFER( CASH_BANK, reciptian, cash_quantity, "metabalance rewards");
    if(score_quantity.amount > 0) TRANSFER( SCORE_BANK, reciptian, score_quantity, "metabalance rewards");
}
