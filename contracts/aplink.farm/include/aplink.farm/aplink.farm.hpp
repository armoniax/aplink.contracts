#include "aplink.farmdb.hpp"

using namespace std;
using namespace wasm::db;

namespace aplink {

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
   CONTENT_LENGTH_INVALID = 15,
   NOT_DISABLED          = 16,

};

using std::string;
using std::vector;
using namespace eosio;

class [[eosio::contract("aplink.farm")]] farm: public eosio::contract {
private:
    global_singleton    _global;
    global_t            _gstate;
    dbc                 _db;

public:
    using contract::contract;

    farm(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        _db(_self), contract(receiver, code, ds),
        _global(get_self(), get_self().value)
    {
        _gstate = _global.exists() ? _global.get() : global_t{};
    }

    /**
    * @param landlord - who can lend out land
    * @param jamfactory - to which expired/rotten apples will be send to
    */
    ACTION init(const name& landlord, const name& jamfactory);

    /**
     * @brief lease a land to a farmer
     * 
     * @param tenant a contract account who applies for a land and certain amount of apples for his or her farmers to pick
     * @param title the land's name
     * @param land_uri  the details info of the farmer
     * @param opened_at  farmer can crop after opened_at
     * @param closed_at farmer can crop before closed_at
     */
    ACTION lease(const name& tenant, const string& land_title, const string& land_uri, const string& banner_uri, 
                const time_point& opened_at, const time_point& closed_at);

    /**
     * @brief reclaim a lease, only for inactive ones
     * 
     * @param issuer - who reclaims, only landlord allows to issue
     * @param lease_id 
     * @param memo 
     */
    ACTION reclaimlease(const name& issuer, const uint64_t& lease_id, const string& memo);

    /**
     * @brief reclaim a land, only for disabled land
     * 
     * @param issuer - who reclaims
     * @param allot_id 
     * @param memo 
     */
    ACTION reclaimallot(const name& issuer, const uint64_t& allot_id, const string& memo);

    /**
     * @brief 
     * 
     * @param lease_id 
     * @param status - active | inactive | none
     */
    ACTION setstatus(const uint64_t& lease_id, const name& status);

    /**
     * @brief farmer can plant apples to customer
     * 
     * @param lease_id 
     * @param farmer  send apples to account
     * @param quantity
     * @param memo 
     */
    ACTION allot(const uint64_t& lease_id, const name& farmer, const asset& quantity, const string& memo);

    /**
     * @brief pick apples
     * 
     * @param farmer 
     * @param allot_ids array of allot ids, support no more than 20 allots
     */
    ACTION pick(const name& farmer, const vector<uint64_t>& allot_ids);

    /**
     * @brief topup apples for a land
     * @param from 
     * @param to 
     * @param quantity 
     * @param memo  land_id
     */
    [[eosio::on_notify("aplink.token::transfer")]]
    void ontransfer(const name& from, const name& to, const asset& quantity, const string& memo);

    using allot_action = eosio::action_wrapper<"allot"_n, &farm::allot>;

    /**
     * @brief 
     * 
     * @param apl_farm_contract 
     * @param land_id 
     * @param apples - available apples
     */
    static void available_apples( const name& apl_farm_contract, const uint64_t& land_id, asset& apples )
    {
        auto db         = dbc( apl_farm_contract );
        auto lease      = lease_t(land_id);
        auto now        = time_point_sec(current_time_point());

        if (!db.get(lease) ||
            now < lease.opened_at || 
            now > lease.closed_at ||
            lease.status != lease_status::active) 
            apples = asset(0, APLINK_SYMBOL);
        
        apples = lease.available_apples;
    }
};

}