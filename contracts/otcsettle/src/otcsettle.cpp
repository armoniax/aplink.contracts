#include "otcsettle.hpp"
#include <eosio/permission.hpp>
#include "eosio.token/eosio.token.hpp"
#include <otcconf/utils.hpp>

using namespace otcsettle;
using namespace eosio;
using namespace otc;

inline int64_t get_precision(const symbol &s) {
    int64_t digit = s.precision();
    CHECK(digit >= 0 && digit <= 18, "precision digit " + std::to_string(digit) + " should be in range[0,18]");
    return calc_precision(digit);
}

void settle::setconf(const name &conf_contract) {
    require_auth( get_self() );    
    CHECKC( is_account(conf_contract), err::ACCOUNT_INVALID, "Invalid account of conf_contract");
    _gstate.conf_contract = conf_contract;
    _conf(true);
}

const settle::conf_t& settle::_conf(bool refresh/* = false*/) {
    if (!_conf_ptr || refresh) {
        CHECK(_gstate.conf_contract.value != 0, "Invalid conf_table");
        _conf_tbl_ptr = make_unique<conf_table_t>(_gstate.conf_contract, _gstate.conf_contract.value);
        CHECK(_conf_tbl_ptr->exists(), "conf table not existed in contract: " + _gstate.conf_contract.to_string());
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}


void settle::setlevel(const name& user, uint8_t level){
    auto conf = _conf();
    require_auth(conf.managers.at(otc::manager_type::admin));
    CHECKC(level >=0, err::PARAM_ERROR, "level must be a positive number");
    CHECKC(level < conf.settle_levels.size(), err::PARAM_ERROR, "level must less than level: " + to_string(conf.settle_levels.size()-1));
    
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
    auto conf = _conf();
    require_auth(conf.managers.at(otc::manager_type::otcbook));
    CHECKC(is_account(merchant), err::ACCOUNT_INVALID, "invalid account: " + merchant.to_string());
    CHECKC(is_account(user), err::ACCOUNT_INVALID, "invalid account: " + user.to_string());
    CHECKC(quantity.amount > 0, err::PARAM_ERROR, "quantity must be positive");
    CHECKC(fee.amount > 0, err::PARAM_ERROR, "quantity must be positive");
    CHECKC(conf.settle_levels.size()>0, err::UN_INITIALIZE, "level config hasn't set");
    CHECKC(end_at > start_at, err::PARAM_ERROR, "end time should later than start time");
    if(quantity.symbol != CASH_SYMBOL || fee.symbol != CASH_SYMBOL) return;

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
    for(int j = conf.settle_levels.size(); j>0; j--){
        auto config = conf.settle_levels.at(j-1);
        if(creator_data.level >= j) break;
        if(creator_data.sum_child_deal >= config.sum_limit){
            creator_data.level = j-1;
            break;
        }
    }
    _db.set(creator_data, _self);

    auto config = conf.settle_levels.at(min(creator_data.level, uint8_t(conf.settle_levels.size()-1)));
    if(config.cash_rate + config.score_rate == 0) return;
    auto rewards = reward_t::idx_t(_self, _self.value);
    auto pid = rewards.available_primary_key();
    auto reward = reward_t(pid);
    reward.deal_id = deal_id;
    reward.reciptian = creator;

    reward.cash = fee*config.cash_rate/RATE_BOOST;
    auto score_amount = multiply_decimal64( fee.amount, get_precision(otc::SCORE_SYMBOL), get_precision(fee.symbol));
    reward.score = asset(score_amount*config.score_rate/RATE_BOOST, SCORE_SYMBOL);
    reward.created_at = time_point_sec(current_time_point());

    _db.set(reward, _self);
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

    if(cash_quantity.amount > 0) TRANSFER( _conf().managers.at(otc::manager_type::cashbank), reciptian, cash_quantity, "metabalance rewards");
    if(score_quantity.amount > 0) TRANSFER( _conf().managers.at(otc::manager_type::scorebank), reciptian, score_quantity, "metabalance rewards");
}
