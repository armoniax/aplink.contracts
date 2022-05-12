#include <aplink.conf/aplink.conf.hpp>

namespace aplink {

using namespace std;

ACTION settings::init(const name& admin) {
   require_auth( _self );
   
   _gstate.admin = admin;
   _global.remove();
}

ACTION settings::setacctres(const account_res& account_create_res) {
   check( is_account(_gstate.admin), "uninitalized" );
   require_auth( _gstate.admin );
   
   _gstate.account_create_res = account_create_res;

}

ACTION settings::setprices(const vector<pair<symbol_code, asset>> prices) {
   require_auth( _gstate.admin );

   auto priceidx = price_t::tbl_t(_self, _self.value);
   
   for (auto& price_info : prices) {
      auto itr = priceidx.find(price_info.first.raw());
      if (itr == priceidx.end()) {
         priceidx.emplace(_self, [&](auto& rec){
            rec.symb = price_info.first;
            rec.price = price_info.second;
            rec.updated_at = time_point_sec(current_time_point());
         });

      } else {
         priceidx.modify(itr, _self, [&](auto& rec){
            rec.price = price_info.second;
            rec.updated_at = time_point_sec(current_time_point());
         });
      }
   }
}

} /// namespace aplink
