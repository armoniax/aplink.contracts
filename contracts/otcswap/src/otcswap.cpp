#include <otcswap/utils.hpp>
#include <otcswap/otcswap.hpp>
#include "eosio.token/eosio.token.hpp"

static constexpr eosio::name active_permission{"active"_n};
namespace otc
{
    using namespace std;
    using namespace eosio;

    void otcswap::ontransfer(name from, name to, asset quantity, string memo)
    {
        if(from == get_self() || to == get_self()) return;
        check(quantity.amount > 0, "must transfer positive quantity");

        check(SCORE_SYMBOL==quantity.symbol, "invalid symbol");

        auto account = account_t(from);
        check(_db.get(account), "account does not exist");
        check(quantity.amount <= account.balance, "overdrawn balance");
        auto topup_quantity = asset(quantity.amount, CNYD_SYMBOL);
        TRANSFER(CNYD_BANK, from, topup_quantity, memo);
        account.balance -= quantity.amount;

        _db.set(account);
    }

    void otcswap::settleto(const name &user, const asset &fee, asset quantity)
    {
        require_auth(_gstate.settle_contract);
        check(quantity.amount > 0, "quantity must be positive");
        check(fee.amount > 0, "quantity must be positive");
        check(is_account(user), "owner account does not exist");
        auto sym = quantity.symbol;
        auto account = account_t(user);
        vector<balance_config> fee_rates = _gstate.fee_rates;

        uint16_t percent = 0;
        for(auto &fee_info : fee_rates){
            if(quantity.amount >= fee_info.quantity_limit) percent = fee_info.balance_precent;
            else break;
        }
        int64_t amount = fee.amount * percent/percent_boost;
        account.balance += amount;
        account.sum += amount;
        _db.set(account, _self);
    }

    ACTION otcswap::setrates(const vector<balance_config> rates)
    {
        require_auth(_gstate.admin);
        _gstate.fee_rates = rates;
        _global.set(_gstate, get_self());
    }

    void otcswap::setadmin(const name &admin, const name &settle_contract)
    {
        require_auth(get_self());
        _gstate.admin = admin;
        _gstate.settle_contract = settle_contract;
        _global.set(_gstate, get_self());
    }
} /// namespace eosio
