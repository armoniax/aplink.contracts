#include "bibiuser.hpp"
#include "utils.hpp"
#include "eosio.token.hpp"

void bibiuser::create(const string& pubkey, const name& owner)
{
    require_auth( owner );

    chatuser chatuser_t(owner);
    CHECK( !_db.get(chatuser_t), "the user is registered");

    chatuser_t.owner=owner;
    chatuser_t.status=0;
    chatuser_t.pubkey=pubkey;
    chatuser_t.enable=true;
    chatuser_t.is_topup=false;
    chatuser_t.vip_ex_time=time_point(microseconds(static_cast<int64_t>(0))); 

    _db.set(chatuser_t);
}

void bibiuser::update(const string& pubkey, const string& nickname, const uint16_t& status, const string& portrait, const name& owner)
{
    require_auth( owner );

    chatuser chatuser_t(owner);
    CHECK( _db.get(chatuser_t), "the user is not registered");
    CHECK(chatuser_t.enable==true,"the user has been disabled");
    if (chatuser_t.vip_ex_time <= current_time_point()){
        chatuser_t.is_topup=false;
    }
    if (chatuser_t.is_topup){    
        chatuser_t.nickname=nickname;
        chatuser_t.portrait=portrait;
        chatuser_t.pubkey=pubkey;
        chatuser_t.status=status;

        _db.set(chatuser_t);
    }else{
        CHECK(nickname.empty(), "the user is not top-up and cannot change the name");
        CHECK(portrait.empty(), "the user is not top-up and cannot change the portrait");
        chatuser_t.pubkey=pubkey;
        chatuser_t.status=status;

        _db.set(chatuser_t);
    }
}

void bibiuser::top_up(name from, name to, asset quantity, string memo)
{
    eosio::print("from: ", from, ", to:", to, ", quantity:" , quantity, ", memo:" , memo);

    if (_self == from ){
        return;
    }
    if (to != _self){
        return;
    }

    CHECK( _gstate.enable, "not enabled" )
    
    chatuser chatuser_t(topup_u);
    CHECK( _db.get(chatuser_t), "the user is not registered");

    // TRANSFER( _gstate.contract_name, topup_u, _gstate.topup_val, "bibichat user top-up" );
    
    if (chatuser_t.vip_ex_time == time_point(microseconds(static_cast<int64_t>(0)))){
       chatuser_t.vip_ex_time= current_time_point()+eosio::days(_gstate.effective_days);
    } else {
       if (chatuser_t.is_topup){
            chatuser_t.vip_ex_time+=eosio::days(_gstate.effective_days);
       }else{
           chatuser_t.vip_ex_time= current_time_point()+eosio::days(_gstate.effective_days);
       }
    }
    chatuser_t.is_topup=true;

    _db.set(chatuser_t);
}

void bibiuser::destory(const name& owner)
{
    require_auth(owner);

    chatuser chatuser_t(owner);
    CHECK( _db.get(chatuser_t), "the user is not registered");

    _db.del(chatuser_t);
}

void bibiuser::settopupconf(const bool& enable, const asset& topup_val, const name& contract_name, uint16_t days)
{
    require_auth( _self );

    CHECK( topup_val.is_valid(), "invalid quantity");
    CHECK( topup_val.amount > 0, "topup_val must be positive");

    _gstate.topup_val = topup_val;
    _gstate.contract_name = contract_name;
    _gstate.enable = enable;
    _gstate.effective_days = days;

}