cmake_minimum_required( VERSION 3.5 )

project(contracts)

include(${CONTRACT_VERSION_FILE})
message(STATUS "Contract Version: ${CONTRACT_VERSION}")

set(AMAX_WASM_OLD_BEHAVIOR "Off")
find_package(amax.cdt)

set(ICON_BASE_URL "http://127.0.0.1/ricardian_assets/mgp.contracts/icons")

# add_subdirectory(mgp.vstaking)
# add_subdirectory(mgp.devshare)
# add_subdirectory(mgp.ecoshare)
# add_subdirectory(mgp.bpvoting)
add_subdirectory(otcbook)
add_subdirectory(otcconf)
# add_subdirectory(mgp.exchange)