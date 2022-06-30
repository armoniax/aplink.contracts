 #pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>
#include <eosio/name.hpp>
#include <otcconf/wasm_db.hpp>

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

static constexpr name MIRROR_BANK = name("amax.mtoken");
static constexpr name AMAX_BANK = name("amax.token");
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

static constexpr symbol SCORE_SYMBOL  = SYMBOL("METAS", 4);
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
    name app_name;
    string app_version;
    string url;
    string logo;
};

typedef set<symbol> symbol_set;
typedef set<name> name_set;

namespace manager_type {
    static constexpr eosio::name admin         = "admin"_n;
    static constexpr eosio::name feetaker         = "feetaker"_n;
    static constexpr eosio::name arbiter         = "arbiter"_n;
    static constexpr eosio::name otcbook        = "otcbook"_n;
    static constexpr eosio::name settlement         = "settlement"_n;
    static constexpr eosio::name swaper         = "swaper"_n;
    static constexpr eosio::name cashbank         = "cashbank"_n;
    static constexpr eosio::name scorebank         = "scorebank"_n;
    static constexpr eosio::name aplinkfarm         = "aplinkfarm"_n;
};

enum class status_type: uint8_t {
   UN_INITIALIZE        = 0,
   INITIALIZED          = 1,
   RUNNING              = 2,
   MAINTAINING          = 9
};

struct settle_level_config {
    uint64_t sum_limit;
    uint16_t cash_rate;
    uint16_t score_rate;
};

struct swap_step_config {
    uint64_t quantity_step;
    uint16_t quote_reward_pct;
};

struct [[eosio::table("global"), eosio::contract("otcconf")]] global_t {
    uint8_t status = 0;
    AppInfo_t app_info;
    map<name,name> managers;

    // for book config
    set<name> pay_type;
    symbol fiat_type;
    uint64_t fee_pct;
    map<symbol, name> stake_assets_contract; //get the contract 
    map<symbol, symbol> coin_as_stake;  //get stake asset for a coin
    symbol_set buy_coins_conf;  //crypto coins that OTC merchants can buy in orders
    symbol_set sell_coins_conf; //crypto coins that OTC merchants can sell in orders
    uint64_t accepted_timeout;
    uint64_t payed_timeout;

    // for settle config
    vector<settle_level_config> settle_levels;
    uint64_t farm_id = 0;
    uint32_t farm_scale = 0;

    // for swap config
    vector<swap_step_config> swap_steps;

    global_t() {}
    EOSLIB_SERIALIZE( global_t, (status)(app_info)(managers)
                                (pay_type)(fiat_type)(fee_pct)
                                (stake_assets_contract)(coin_as_stake)
                                (buy_coins_conf)(sell_coins_conf)
                                (accepted_timeout)(payed_timeout)
                                (settle_levels)(farm_id)(farm_scale)
                                (swap_steps)
    )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

} // OTC