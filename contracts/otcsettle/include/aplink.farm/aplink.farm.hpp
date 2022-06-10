using namespace std;
using namespace wasm::db;

namespace aplink {

static constexpr symbol APLINK_SYMBOL              = SYMBOL("APL", 4);
static constexpr name APLINK_BANK                  { "aplink.token"_n };

using std::string;
using std::vector;
using namespace eosio;

class [[eosio::contract("aplink.farm")]] farm: public eosio::contract {

public:
    using contract::contract;

    /**
    * @param lord lord can lend out land
    * @param jamfactory expired apples will send to jamfactory
    */
    [[eosio::action]]
    void setlord(const name& lord, const name& jamfactory);

    /**
     * @brief lease a land to a farmer
     * 
     * @param farmer account who can crop the land, always be a contract
     * @param title the land's name
     * @param uri  the details info of the farmer
     * @param open_at  farmer can crop after open_at
     * @param close_at farmer can crop before close_at
     */
    [[eosio::action]]
    void lease(const name& farmer, const string& title, const string& uri, const time_point& open_at, const time_point& close_at);

    /**
     * @brief reclaim a land, only for disabled land
     * 
     * @param land_id 
     * @param recipient all seeds on this land will send to 
     * @param memo 
     */
    [[eosio::action]]
    void reclaim(const uint64_t& land_id, const name& recipient, const string& memo);

    /**
     * @brief 
     * 
     * @param land_id 
     * @param status  0:NONE 1:Enable 2:Disable
     */
    [[eosio::action]]
    void setstatus(const uint64_t& land_id, const uint8_t& status);

    /**
     * @brief farmer can plant seeds to customer
     * 
     * @param land_id 
     * @param customer  send seeds to account
     * @param quantity
     * @param memo 
     */
    [[eosio::action]]
    void grow(const uint64_t& land_id, const name& customer, const asset& quantity, const string& memo);

    /**
     * @brief pick apples
     * 
     * @param croper 
     * @param appleids apple_id array, support lessthan 20 apples
     */
    [[eosio::action]]
    void pick(const name& croper, vector<uint64_t> appleids);

    /**
     * @brief topup seeds for a land
     * @param from 
     * @param to 
     * @param quantity 
     * @param memo  land_id
     */
    [[eosio::on_notify("aplink.token::transfer")]]
    void ontransfer(const name& from, const name& to, const asset& quantity, const string& memo);
    
    using grow_action = eosio::action_wrapper<"grow"_n, &farm::grow>;
};

}