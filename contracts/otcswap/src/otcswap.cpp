#include <otcconf/utils.hpp>
#include <otcswap.hpp>
#include "eosio.token/eosio.token.hpp"

static constexpr eosio::name active_permission{"active"_n};
namespace otc
{
using namespace std;
using namespace eosio;
inline int64_t get_precision(const symbol &s) {
    int64_t digit = s.precision();
    CHECK(digit >= 0 && digit <= 18, "precision digit " + std::to_string(digit) + " should be in range[0,18]");
    return calc_precision(digit);
}

void otcswap::setconf(const name &conf_contract) {
    require_auth( get_self() );    
    CHECKC( is_account(conf_contract), err::ACCOUNT_INVALID, "Invalid account of conf_contract");
    _gstate.conf_contract = conf_contract;
    _conf(true);
}

const otcswap::conf_t& otcswap::_conf(bool refresh/* = false*/) {
    if (!_conf_ptr || refresh) {
        CHECK(_gstate.conf_contract.value != 0, "Invalid conf_table");
        _conf_tbl_ptr = make_unique<conf_table_t>(_gstate.conf_contract, _gstate.conf_contract.value);
        CHECK(_conf_tbl_ptr->exists(), "conf table not existed in contract: " + _gstate.conf_contract.to_string());
        _conf_ptr = make_unique<conf_t>(_conf_tbl_ptr->get());
    }
    return *_conf_ptr;
}

void otcswap::ontransfer(name from, name to, asset quantity, string memo)
{
    if(from == get_self() || to != get_self()) return;

    check(quantity.amount > 0, "must transfer positive quantity");
    check(SCORE_SYMBOL==quantity.symbol, "invalid symbol");

    auto account = account_t(from);
    check(_db.get(account), "account does not exist");
    check(quantity.amount <= account.balance, "overdrawn balance");
    auto value = multiply_decimal64( quantity.amount, get_precision(STAKE_USDT), get_precision(quantity.symbol));
    auto topup_quantity = asset(quantity.amount, STAKE_USDT);
    TRANSFER(MIRROR_BANK, from, topup_quantity, memo);
    account.balance -= quantity.amount;

    _db.set(account);
}

void otcswap::settleto(const name &user, const asset &fee, asset quantity)
{
    require_auth(_conf().managers.at(otc::manager_type::otcbook));
    check(quantity.amount > 0, "quantity must be positive");
    check(fee.amount > 0, "quantity must be positive");
    check(STAKE_USDT == quantity.symbol, "only support settling for MUSDT");
    check(is_account(user), "owner account does not exist");
    auto sym = quantity.symbol;
    auto account = account_t(user);

    auto value = multiply_decimal64( quantity.amount, get_precision(STAKE_USDT), get_precision(quantity.symbol));
    uint16_t percent = 0;
    for(auto &step : _conf().swap_steps){
        if(quantity.amount >= step.quantity_step) percent = step.quote_reward_pct;
        else break;
    }
    int64_t amount = fee.amount * percent/percent_boost;
    account.balance += amount;
    account.sum += amount;
    _db.set(account, _self);
}
} /// namespace eosio
