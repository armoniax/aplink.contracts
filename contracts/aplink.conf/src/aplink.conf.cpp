#include <aplink.conf/aplink.conf.hpp>

namespace aplink {

ACTION settings::init(const name& admin) {
   require_auth( _self );
   
   _gstate.admin = admin;
   _global.remove();
}

ACTION settings::update(const account_res& account_create_res) {
   check( is_account(_gstate.admin), "uninitalized" );
   require_auth( _gstate.admin );
   
   _gstate.account_create_res = account_create_res;

}

} /// namespace aplink
