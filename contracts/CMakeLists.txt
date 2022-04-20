cmake_minimum_required( VERSION 3.5 )

project(contracts)

include(${CONTRACT_VERSION_FILE})
message(STATUS "Contract Version: ${CONTRACT_VERSION}")

set(AMAX_WASM_OLD_BEHAVIOR "Off")
find_package(amax.cdt)

set(ICON_BASE_URL "http://127.0.0.1/ricardian_assets/amax.contracts/icons")

# add_subdirectory(amax.vstaking)
# add_subdirectory(amax.devshare)
# add_subdirectory(amax.ecoshare)
# add_subdirectory(amax.bpvoting)
# add_subdirectory(otcbook)
# add_subdirectory(otcconf)
# add_subdirectory(amax.exchange)
# add_subdirectory(rewardnewbie)
add_subdirectory(aplink.token)