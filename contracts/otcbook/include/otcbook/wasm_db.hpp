#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

namespace wasm { namespace db {

using namespace eosio;

template<eosio::name::raw TableName, typename T, typename... Indices>
class multi_index_ex: public eosio::multi_index<TableName, T, Indices...> {
public:
    using base = eosio::multi_index<TableName, T, Indices...>;
    using base::base;

    template<typename Lambda>
    void set(uint64_t pk, eosio::name payer, Lambda&& setter ) {
        auto itr = base::find(pk);
        if (itr == base::end()) {
            base::emplace(payer, setter);
        } else {
            base::modify(itr, payer, setter);
        }
    }

    bool erase_by_pk(uint64_t pk) {
        auto itr = base::find(pk);
        if (itr != base::end()) {
            base::erase(itr);
            return true;
        }
        return false;
    }
};

enum return_t{
    NONE    = 0,
    MODIFIED,
    APPENDED,
};

class dbc {
private:
    name code;   //contract owner

public:
    dbc(const name& code): code(code) {}

    template<typename RecordType>
    bool get(RecordType& record) {
        auto scope = record.scope();
        if (scope == 0) scope = code.value;

        typename RecordType::idx_t tbl(code, scope);
        if (tbl.find(record.primary_key()) == tbl.end())
            return false;

        record = tbl.get(record.primary_key());
        return true;
    }

    template<typename RecordType>
    auto get_tbl(RecordType& record) {
        auto scope = record.scope();
        if (scope == 0) scope = code.value;

        typename RecordType::idx_t tbl(code, scope);
        return tbl;
    }

    template<typename RecordType>
    return_t set(const RecordType& record) {
        auto scope = record.scope();
        if (scope == 0) scope = code.value;

        typename RecordType::idx_t tbl(code, scope);
        auto itr = tbl.find( record.primary_key() );
        if ( itr != tbl.end()) {
            tbl.modify( itr, code, [&]( auto& item ) {
                item = record;
            });
            return return_t::MODIFIED;

        } else {
            tbl.emplace( code, [&]( auto& item ) {
                item = record;
            });
            return return_t::APPENDED;
        }
    }

    template<typename RecordType>
    void del(const RecordType& record) {
        auto scope = record.scope();
        if (scope == 0) scope = code.value;

        typename RecordType::idx_t tbl(code, scope);
        auto itr = tbl.find(record.primary_key());
        if ( itr != tbl.end() ) {
            tbl.erase(itr);
        }
    }

};

}}//db//wasm