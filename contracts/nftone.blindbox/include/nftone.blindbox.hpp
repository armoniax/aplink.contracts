#include "nftone.blindbox_db.hpp"
#include <amax.ntoken/amax.ntoken.hpp>

using namespace std;
using namespace wasm::db;

class [[eosio::contract("nftone.blindbox")]] blindbox: public eosio::contract {
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
    void createpool( const name& owner,const string& title,const name& asset_contract, const nsymbol& asset_symbol,const time_point_sec& opended_at);
    
    // [[eosio::action]]
    // void test();
    [[eosio::on_notify("*::transfer")]] void onnfttrans(const name& from, const name& to, const vector<nasset>& assets, const string& memo);
private:
    uint64_t _rand(uint64_t max_uint,  uint16_t min_unit, name owner , uint64_t pool_id);
    void _rand_nft( const pool_t& pool , const name& owner);

}; //contract one.blindbox