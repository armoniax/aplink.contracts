/*
 * @Author: your name
 * @Date: 2022-04-13 15:58:25
 * @LastEditTime: 2022-04-14 16:04:34
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: /deotc.contracts/contracts/otcconf/src/otcconf.cpp
 */
#include <eosio.token/eosio.token.hpp>
#include <otcconf/safemath.hpp>
#include <otcconf/otcconf.hpp>

using namespace eosio;
using namespace std;
using std::string;

namespace otc {

using namespace std;
using namespace eosio;
using namespace wasm::safemath;

/**
 * reset the global with default values
 */
void otcconf::init() {
    _gstate = {};
}


/**
 * update price
 */
void otcconf::setrate(const map<symbol, asset>& prices_quote_cny) {
    require_auth( _self );

    check(!prices_quote_cny.empty(), "prices_quote_cny empty");
    for (const auto& item : prices_quote_cny) {
        CHECK( item.first != CNY, "base symbol can not equal to quote symbol");
        _gstate.prices_quote_cny[item.first] = item.second;
    }
}

void otcconf::setotcname(const name& otc_name) {
    require_auth( _self );
    _gstate.otc_name = otc_name;
    
}

}  //end of namespace:: otc
