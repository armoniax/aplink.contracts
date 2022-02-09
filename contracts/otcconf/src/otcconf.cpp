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

void otcconf::init() {
}


/**
 * update price
 */ 
void otcconf::setrate(const vector<asset>& prices_quote_cny) {
    require_auth( _self );
    
    check(!prices_quote_cny.empty(), "prices_quote_cny empty");
    for (const auto& price : prices_quote_cny) {
        CHECK( price.symbol != CNY, "base price symbol not allowed");
        _gstate.prices_quote_cny[price.symbol] = price;
    }
}

}  //end of namespace:: otc
