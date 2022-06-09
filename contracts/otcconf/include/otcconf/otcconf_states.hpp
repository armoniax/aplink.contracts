 #pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>
#include <eosio/name.hpp>

#include <optional>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <type_traits>

#include <otcconf/utils.hpp>

namespace otc {

using namespace std;
using namespace eosio;
using namespace wasm;

#define CONTRACT_TBL [[eosio::table, eosio::contract("otcconf")]]

#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

static constexpr name MIRROR_BNAK = name("amax.mtoken");
static constexpr name AMAX_BNAK = name("amax.token");
static constexpr name CNYD_BANK = name("cnyd.token");

// crypto coins for staking
static constexpr symbol STAKE_CNYD = SYMBOL("CNYD",4);
static constexpr symbol STAKE_AMAX = SYMBOL("AMAX",8);
static constexpr symbol STAKE_USDT = SYMBOL("MUSDT",6);


// crypto coins for trading
static constexpr symbol USDT_ERC20 = SYMBOL("USDTERC", 6);
static constexpr symbol USDT_TRC20 = SYMBOL("USDTTRC", 6);
static constexpr symbol USDT_BEP20 = SYMBOL("USDTBEP", 6);
static constexpr symbol CNYD_BEP20 = SYMBOL("CNYDBEP", 4);
static constexpr symbol CNYD_ARC20 = SYMBOL("CNYDARC", 4);
static constexpr symbol AMAX_ARC20 = SYMBOL("AMAXARC", 8);

// static constexpr symbol AMAX       = SYMBOL("AMAX", 8);
// static constexpr symbol BTC        = SYMBOL("BTC", 8);
// static constexpr symbol ETH        = SYMBOL("ETH", 8);
static constexpr symbol CNYD       = SYMBOL("CNYD", 4);

// fiat currency symbols
static constexpr symbol   CNY      = SYMBOL("CNY", 4);
static constexpr symbol   USD      = SYMBOL("USD", 4);
static constexpr symbol   EUR      = SYMBOL("EUR", 4);
static constexpr symbol   INR      = SYMBOL("INR", 4);

// pay type
static constexpr name BANK          = "bank"_n;
static constexpr name WECHAT        = "wechat"_n;
static constexpr name ALIPAY        = "alipay"_n;
static constexpr name CNYDPAY       = "cnyd"_n;
static constexpr name MASTER        = "master"_n;
static constexpr name VISA          = "visa"_n;
static constexpr name PAYPAL        = "paypal"_n;

/**
 * App upgrade info
 */
struct AppInfo_t {
    string latest_app_version;
    string android_apk_download_url; //ipfs url
    string upgrade_log;
    bool force_upgrade;
};

typedef set<symbol> symbol_set;
typedef set<name> name_set;

struct [[eosio::table("global"), eosio::contract("otcconf")]] global_t {
    name otc_name = "meta.balance"_n;

    AppInfo_t app_info = {
        "0.1.0",
        "https://ipfs.io/ipfs/QmZUmzu96uKBLcCjNcnbD12hmjtMnaTs7ymLDHio3qbeDi",
        "initial beta testing release",
        false
    };

    set<name> pay_type = { CNYDPAY, BANK, WECHAT, ALIPAY };

    name_set arbiters { "casharbitoo1"_n };

    symbol_set coin_type = { AMAX_ARC20, CNYD_ARC20, USDT_ERC20, USDT_TRC20, USDT_BEP20 };
    symbol fiat_type = CNY;

    name fee_recv_addr = "oxo.feeadmin"_n;
    uint64_t fee_pct   = 50;

    map<symbol, name> stake_coins_conf = {
        {STAKE_AMAX, AMAX_BNAK},
        {STAKE_CNYD, CNYD_BANK},
        {STAKE_USDT, MIRROR_BNAK}
    };

    map<symbol, symbol> coin_to_asset = {
        {AMAX_ARC20, STAKE_AMAX},
        {CNYD_ARC20, STAKE_CNYD},
        {USDT_ERC20, STAKE_USDT},
        {USDT_TRC20, STAKE_USDT},
        {USDT_BEP20, STAKE_USDT}
    };

    /**
     * crypto coins that OTC merchants can buy in orders
     */
    symbol_set buy_coins_conf = {
        AMAX_ARC20,
        CNYD_ARC20,
        USDT_ERC20,
        USDT_TRC20,
        USDT_BEP20
    };

    /**
     * crypto coins that OTC merchants can sell in orders
     */
    symbol_set sell_coins_conf = {
        AMAX_ARC20,
        CNYD_ARC20,
        USDT_ERC20,
        USDT_TRC20,
        USDT_BEP20
    };

    uint64_t accepted_timeout   = 1800;
    uint64_t payed_timeout   = 10800;


    global_t() {

    }

    EOSLIB_SERIALIZE( global_t, (otc_name)(app_info)(pay_type)(arbiters)(coin_type)
                                (fiat_type)(fee_recv_addr)(fee_pct)
                                (stake_coins_conf)(coin_to_asset)
                                (buy_coins_conf)(sell_coins_conf)
                                (accepted_timeout)(payed_timeout)
    )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;


} // OTC