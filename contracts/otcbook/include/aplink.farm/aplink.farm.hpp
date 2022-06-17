#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace std;

namespace aplink {

static constexpr symbol APLINK_SYMBOL              = SYMBOL("APL", 4);
static constexpr name APLINK_BANK                  { "aplink.token"_n };

using namespace eosio;

class [[eosio::contract("aplink.farm")]] farm: public eosio::contract {

public:
    /**
     * @brief farmer can plant seeds to customer
     * 
     * @param land_id 
     * @param customer  send seeds to account
     * @param quantity
     * @param memo 
     */
    [[eosio::action]]
    void allot(const uint64_t& land_id, const name& customer, const asset& quantity, const string& memo);

    using allot_action = eosio::action_wrapper<"allot"_n, &farm::allot>;
};

}