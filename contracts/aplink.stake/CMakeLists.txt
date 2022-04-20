add_contract(aplink.stake stake
   ${CMAKE_CURRENT_SOURCE_DIR}/src/stake.cpp
)

target_include_directories(aplink.stake
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include

set_target_properties(aplink.stake
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/stake.clauses.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/stake.clauses.md @ONLY )
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/stake.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/stake.contracts.md @ONLY )

target_compile_options( aplink.stake PUBLIC -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
