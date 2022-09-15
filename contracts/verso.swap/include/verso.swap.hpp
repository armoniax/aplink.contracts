#include "verso.swap_db.hpp"
#include <amax.ntoken/amax.ntoken.hpp>

using namespace std;
using namespace wasm::db;

class [[eosio::contract("verso.swap")]] blindbox: public eosio::contract {
private:
    global_singleton    _global;
    global_t            _gstate;

public:
    using contract::contract;

    blindbox(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        contract(receiver, code, ds),
        _global(get_self(), get_self().value)
    {
        _gstate = _global.exists() ? _global.get() : global_t{};
    }

    ~blindbox() { _global.set( _gstate, get_self() ); }

    [[eosio::action]]
    void init();

    [[eosio::action]]
    void createtotal( const uint64_t& amount );

    [[eosio::action]]
    void bindtotal( const vector<uint64_t>& pool_ids, const uint64_t& totality_id );

    [[eosio::action]]
    void addpooltoken( const name& owner,const string& title,const name& asset_contract, const name& blindbox_contract,
                           const asset& price, const name& fee_receiver,
                           const name& exchange_type, const uint64_t& totality_id,
                           const time_point_sec& opended_at, const uint64_t& opened_days);


    [[eosio::action]]
    void addpoolnft( const name& owner,const string& title,const name& asset_contract, const name& blindbox_contract,
                           const nasset& price, const name& fee_receiver,
                           const name& exchange_type, const uint64_t& totality_id,
                           const time_point_sec& opended_at, const uint64_t& opened_days);
    
    [[eosio::action]]
    void enableplan(const name& owner, const uint64_t& pool_id, bool enabled);
    
    [[eosio::action]]
    void editplantime(const name& owner, const uint64_t& pool_id, const time_point_sec& opended_at, const time_point_sec& closed_at);

    [[eosio::action]]
    void endpool(const name& owner, const uint64_t& pool_id);
    
    [[eosio::action]]
    void test(const uint64_t& len);


    [[eosio::on_notify("amax.ntoken::transfer")]] void onnnfttrans(const name& from, const name& to, const vector<nasset>& assets, const string& memo);

    [[eosio::on_notify("versontoken2::transfer")]] void onversotrans(const name& from, const name& to, const vector<nasset>& assets, const string& memo);

    [[eosio::on_notify("amax.mtoken::transfer")]] void ontokentrans( const name& from, const name& to, const asset& quantity, const string& memo );
    
private:
    uint64_t _rand(uint64_t max_uint,  uint16_t min_unit, name owner , uint64_t pool_id);
    uint64_t _rand_nft( const pool_t& pool , const name& owner);
    uint64_t _ergodic_nft( const pool_t& pool , const name& owner);

}; //contract one.blindbox