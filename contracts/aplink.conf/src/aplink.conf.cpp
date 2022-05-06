#include <aplink.conf/aplink.conf.hpp>

namespace aplink {

ACTION settings::init(const aplink_settings& settings) {
   require_auth( _self );
   _gstate.settings = settings;

}

} /// namespace aplink
