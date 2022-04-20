#include <activate/activate.hpp>


namespace eosio {
    activate::activate( name s, name code, datastream<const char*> ds ):contract(s,code,ds),_state(get_self(), get_self().value){}

    void activate::reward(const name& to)
    {
        require_auth(to);
        check(is_account(to), "to account does not exist");
        check(_state.exists(), "state not configured");
        state state=_state.get();

        accounts to_acnts(get_self(), to.value);
        auto itr = to_acnts.find(to.value);
        check(itr == to_acnts.end(), "account has been activated");

        vector<permission_level> p;
        p.push_back(permission_level{get_self(), "active"_n }); 
        action(
            p, 
            aplink_token, 
            "transfer"_n, 
            std::make_tuple(get_self(), to, state.reward, std::string("memo") )
        ).send();

       
        to_acnts.emplace(get_self(), [&]( auto& row ) {
            row.user = to;
        });
        
    }

    void activate::setstate(const bool& enable,const asset& reward)
    {
        require_auth(get_self());
        check(reward.is_valid(), "invalid quantity");
        check(reward.amount > 0, "reward_value must be positive");
        state st = _state.exists() ? _state.get() : state{};
        st.enable = enable;
        st.reward = reward;
        _state.set(st, _self);
    }
    //test
    // void activate::deleteacnt(const name& to)
    // {
    //      require_auth(to);
    //      accounts to_acnts(get_self(), to.value);
    //      auto iterator = to_acnts.find(to.value);
    //      check(iterator != to_acnts.end(), "Record does not exist");
    //      to_acnts.erase(iterator);        
    // }

} 