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

    void otcswap::settleto(const name &to, const asset &fee, asset quantity)
    {
        require_auth(_gstate.settle_contract);
        check(quantity.amount > 0, "quantity must be positive");
        check(is_account(to), "owner account does not exist");
        auto sym = quantity.symbol;
        auto account = account_t(to);
        vector<pair<uint64_t, double>> fee_rates = _gstate.fee_rates;

        double balance;

        for (auto &fee_info : fee_rates)
        {
            if (fee_info.first < (quantity.amount))
            {
                balance = fee_info.second;
            }
        }
        int64_t amount = fee.amount * balance;
        account.balance += amount;
        account.sum += amount;
        _db.set(account, _self);
    }

    ACTION otcswap::setrates(const vector<pair<uint64_t, double>> rates)
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
