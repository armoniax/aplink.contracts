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
        eosio::print("from: ", from, ", to:", to, ", quantity:", quantity, ", memo:", memo);
        check(quantity.amount > 0, "must transfer positive quantity");

        auto sym = quantity.symbol;
        auto account = account_t(from);
        check(_db.get(account), "account does not exist");
        check(quantity.amount <= account.balance, "overdrawn balance");
        auto topup_quantity = asset(quantity.amount, CNYD_SYMBOL);
        TRANSFER(CNYD_BANK, from, topup_quantity, memo);
        account.balance -= quantity.amount;

        _db.set(account);
    }

    void otcswap::settleto(const name &to, const asset &fee_pct, asset quantity)
    {
        require_auth(to);
        check(quantity.amount > 0, "quantity must be positive");
        check(is_account(to), "owner account does not exist");
        auto sym = quantity.symbol;
        auto account = account_t(to);
        vector<pair<uint64_t, double>> fee_rates = _gstate.fee_rates;

        double fee;
        if (_db.get(account))
        {
            for (auto &fee_info : fee_rates)
            {
                if (fee_info.first < (account.balance + quantity.amount/percent_boost))
                {
                    fee = fee_info.second;
                }
            }
        }
        else
        {
            for (auto &fee_info : fee_rates)
            {
                if (fee_info.first < ( quantity.amount/percent_boost))
                {
                    fee = fee_info.second;
                }
            }
        }
        eosio::print("fee: ", fee, ", fee_pct:", fee_pct.amount);

        int64_t amount = quantity.amount * fee_pct.amount * fee / 1000;
        // check( sym.is_valid(), "invalid symbol name" );
        auto topup_quantity = asset(amount, sym);

        if (!_db.get(account))
        {
            eosio::print("emplace");
            account.balance = amount;
            account.sum = amount;
            _db.set(account, _self);
        }
        else
        {

            eosio::print("modify");
            account.balance += amount;
            account.sum += amount;
            _db.set(account);
        }
    }

    ACTION otcswap::setrates(const vector<pair<uint64_t, double>> rates)
    {
        require_auth(_gstate.admin);
        //_global.remove();
        _gstate.fee_rates = rates;
        _global.set(_gstate, get_self());
    }
    
} /// namespace eosio