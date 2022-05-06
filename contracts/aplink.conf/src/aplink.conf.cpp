#include <aplink.conf/aplink.conf.hpp>

namespace aplink {

ACTION settings::init(const name& admin) {
   require_auth( _self );
   
   _gstate.admin = admin;
}

ACTION settings::update(const aplink_settings& settings) {
   check( is_account(_gstate.admin), "uninitalized" );
   require_auth( _gstate.admin );
   
   _gstate.settings = settings;
}

} /// namespace aplink
