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

// crypto coins for trading
static constexpr symbol USDT_ERC20 = SYMBOL("USDTERC", 6);
static constexpr symbol USDT_TRC20 = SYMBOL("USDTTRC", 6);
static constexpr symbol USDT_BEP20 = SYMBOL("USDTBEP", 6);
static constexpr symbol CNYD_BEP20 = SYMBOL("CNYDBEP", 6);
static constexpr symbol CNYD_ARC20 = SYMBOL("CNYDARC", 6);
static constexpr symbol BTC        = SYMBOL("BTC", 8);
static constexpr symbol ETH        = SYMBOL("ETH", 18);
static constexpr symbol CNYD       = SYMBOL("CNYD", 6);

// fiat currency symbols
static constexpr symbol   CNY      = SYMBOL("CNY", 4);
static constexpr symbol   USD      = SYMBOL("USD", 4);
static constexpr symbol   EUR      = SYMBOL("EUR", 4);
static constexpr symbol   INR      = SYMBOL("INR", 4);

// pay type
static constexpr name BANK        = "bank"_n;
static constexpr name WECHAT      = "wechat"_n;
static constexpr name ALIPAY      = "alipay"_n;
static constexpr name MASTER      = "master"_n;
static constexpr name VISA        = "visa"_n;
static constexpr name PAYPAL      = "paypal"_n;

/** 
 * App upgrade info
 */
struct AppInfo_t {
    string latest_app_version;
    string android_apk_download_url; //ipfs url
    string upgrade_log;
    bool force_upgrade;
};

struct [[eosio::table("global"), eosio::contract("otcconf")]] global_t {
    AppInfo_t app_info = { 
        "0.1.0",
        "https://ipfs.io/ipfs/QmZUmzu96uKBLcCjNcnbD12hmjtMnaTs7ymLDHio3qbeDi", 
        "initial beta testing release", 
        false 
    };

    set<name> pay_type = { BANK, WECHAT, ALIPAY, MASTER, VISA, PAYPAL };

    set<symbol> coin_type = { USDT_ERC20, USDT_TRC20, USDT_BEP20, CNYD_BEP20, CNYD_ARC20 };
    set<symbol> fiat_type = { CNY, USD, EUR, INR };

    /** 
     * OTC merchants to make sell orders with cypto
     */
    map<symbol, set<symbol>> coin_to_fiat_conf = {
        { CNYD_BEP20,  { CNY } },
        { CNYD_ARC20,  { CNY } },
        { BTC,         { CNY } },
        { ETH,         { CNY } }
    };
    
    /** 
     * OTC merchants to make buy orders with fiat
     */
    map<symbol, set<symbol>> fiat_to_coin_conf = {
        { CNY, {
            CNYD_BEP20, 
            CNYD_ARC20,  
            USDT_ERC20, 
            USDT_BEP20,
            BTC,
            ETH }
        }
    };


    map<symbol, asset> prices_quote_cny {
        { USD,  ASSET(6.3000, CNY) },
        { CNYD, ASSET(1.0000, CNY) }
    };

    global_t() {
        
    }

    EOSLIB_SERIALIZE( global_t, (app_info)(pay_type)(coin_type)(fiat_type)
                                (coin_to_fiat_conf)(fiat_to_coin_conf)(prices_quote_cny)
    )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;


} // OTC
