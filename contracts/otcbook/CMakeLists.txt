add_contract(otcbook otcbook
   ${CMAKE_CURRENT_SOURCE_DIR}/src/otcbook.cpp
)

target_include_directories(otcbook
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../otcconf/include 
   ${CMAKE_CURRENT_SOURCE_DIR}/../otcsettle/include 
   ${CMAKE_CURRENT_SOURCE_DIR}/../otcswap/include 
   )

set_target_properties(otcbook
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/otcbook.clauses.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/otcbook.clauses.md @ONLY )
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/otcbook.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/otcbook.contracts.md @ONLY )

target_compile_options( otcbook PUBLIC -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
