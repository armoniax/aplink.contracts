#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>
#include <algorithm>
#include "otcswapdb.hpp"
#include <otcconf/otcconf.hpp>


using namespace std;
using namespace eosio;
using namespace wasm::db;

#define SWAP_SETTLE(bank, user, fee, quantity) \
    {	otc::otcswap::settleto_action act{ bank, { {_self, active_perm} } };\
			act.send( user, fee, quantity );}

namespace otc
{
using std::pair;
using std::string;

class [[eosio::contract("otcswap")]] otcswap : public contract
{
    using contract::contract;
    using conf_t = otc::global_t;
    using conf_table_t = otc::global_singleton;

private:
    gswap_singleton    _global;
    gswap_t            _gstate;
    dbc           _db;

    std::unique_ptr<conf_table_t> _conf_tbl_ptr;
    std::unique_ptr<conf_t> _conf_ptr;

    const conf_t& _conf(bool refresh = false);

public:
    otcswap(eosio::name receiver, eosio::name code, datastream<const char *> ds) : _db(_self),contract(receiver, code, ds), _global(_self, _self.value)
    {
        if (_global.exists()) _gstate = _global.get();
        else _gstate = gswap_t{};
        
    }

    ~otcswap() { _global.set(_gstate, get_self()); }

    [[eosio::action]]
    void setconf(const name &conf_contract);

    /**
     * ontransfer, trigger by recipient of transfer()
     */
    [[eosio::on_notify("amax.arc::transfer")]] 
    void ontransfer(name from, name to, asset quantity, string memo);

    /**
     * @brief Increase the balance swap quotes of integral
     * 
     * @param user update user
     * @param fee order fee
     * @param quantity order trade quantity
     */
    [[eosio::action]] 
    void settleto(const name &user, const asset& fee,  asset quantity);

    using settleto_action = eosio::action_wrapper<"settleto"_n, &otcswap::settleto>;
};

}
