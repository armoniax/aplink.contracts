#include <apollo.token/apollo.token.hpp>

namespace aplink {

ACTION token::init() {
   auto tokenstats = tokenstats_t(0);
   _db.del( tokenstats );

   // _gstate.initialized = true;

}

} /// namespace apollo
