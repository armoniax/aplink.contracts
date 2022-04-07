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
    _gstate.arbiters.insert("casharbitoo1"_n);
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

}  //end of namespace:: otc
