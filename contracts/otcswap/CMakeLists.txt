add_contract(otcswap otcswap
   ${CMAKE_CURRENT_SOURCE_DIR}/src/otcswap.cpp
)

target_include_directories(otcswap
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../otcconf/include 
 )

set_target_properties(otcswap
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/otcswap.clauses.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/otcswap.clauses.md @ONLY )
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/otcswap.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/otcswap.contracts.md @ONLY )

target_compile_options( otcswap PUBLIC -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
