cmake_minimum_required( VERSION 3.5 )

project(contracts)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

set(ICON_BASE_URL "http://127.0.0.1/ricardian_assets/mgp.contracts/icons")

# add_subdirectory(mgp.vstaking)
# add_subdirectory(mgp.devshare)
# add_subdirectory(mgp.ecoshare)
# add_subdirectory(mgp.bpvoting)
add_subdirectory(otcbook)
# add_subdirectory(mgp.exchange)