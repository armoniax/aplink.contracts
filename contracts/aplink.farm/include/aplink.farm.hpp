#include "aplink.farmdb.hpp"

using namespace std;
using namespace wasm::db;

namespace aplink {

static constexpr symbol APLINK_SYMBOL              = SYMBOL("APL", 4);
static constexpr name APLINK_BANK                  { "aplink.token"_n };

#define CHECKC(exp, code, msg) \
   { if (!(exp)) eosio::check(false, string("$$$") + to_string((int)code) + string("$$$ ") + msg); }
   
enum class err: uint8_t {
   NONE                 = 0,
   TIME_INVALID         = 1,
   RECORD_EXISTING      = 2,
   RECORD_NOT_FOUND     = 3,
   SYMBOL_MISMATCH      = 4,
   PARAM_ERROR          = 5,
   PAUSED               = 6,
   NO_AUTH              = 7,
   NOT_POSITIVE         = 8,
   NOT_STARTED          = 9,
   OVERSIZED            = 10,
   TIME_EXPIRED         = 11,
   NOTIFY_UNRELATED     = 12,
   ACTION_REDUNDANT     = 13,
   ACCOUNT_INVALID      = 14,
   CONTENT_LENGTH_INVALID      = 15,
   NOT_DISABLED          = 16,

};

using std::string;
using std::vector;
using namespace eosio;

class [[eosio::contract("aplink.farm")]] farm: public eosio::contract {

public:
    using contract::contract;

    farm(eosio::name lord, eosio::name code, datastream<const char*> ds):
        _db(_self), contract(lord, code, ds),
        _global(get_self(), get_self().value)
    {
        _gstate = _global.exists() ? _global.get() : global_t{};
    }

    /**
    * @param lord * lord can lend out land
    */
    [[eosio::action]]
    void setlord(const name& lord);

    /**
     * @brief lend a land to a farmer
     * 
     * @param farmer account who can crop the land, always be a contract
     * @param title the land's name
     * @param uri  the details info of the farmer
     * @param crop_start_at  farmer can crop after start_at
     * @param crop_end_at farmer can crop before end_at
     */
    [[eosio::action]]
    void lend(const name& farmer, const string& title, const string& uri, const time_point& crop_start_at, const time_point& crop_end_at);

    /**
     * @brief retrieve a land, only for disabled land
     * 
     * @param land_id 
     * @param recipient all seeds on this land will send to 
     * @param memo 
     */
    [[eosio::action]]
    void retrieve(const uint64_t& land_id, const name& recipient, const string& memo);

    /**
     * @brief 
     * 
     * @param land_id 
     * @param status 
     */
    [[eosio::action]]
    void setstatus(const uint64_t& land_id, const uint8_t& status);

    /**
     * @brief farmer can crop seeds to customer
     * 
     * @param land_id 
     * @param customer  send seeds to account
     * @param quantity
     * @param memo 
     */
    [[eosio::action]]
    void crop(const uint64_t& land_id, const name& customer, const asset& quantity, const string& memo);

    /**
     * @brief topup seeds for a land
     * @param from 
     * @param to 
     * @param quantity 
     * @param memo  land_id
     */
    [[eosio::on_notify("aplink.token::transfer")]]
    void ontransfer(const name& from, const name& to, const asset& quantity, const string& memo);
    
private:
    global_singleton    _global;
    global_t            _gstate;
    dbc                 _db;
};

}