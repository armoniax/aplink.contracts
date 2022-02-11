add_contract(otcconf otcconf
   ${CMAKE_CURRENT_SOURCE_DIR}/src/otcconf.cpp
)

target_include_directories(otcconf
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(otcconf
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/otcconf.clauses.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/otcconf.clauses.md @ONLY )
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/otcconf.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/otcconf.contracts.md @ONLY )

target_compile_options( otcconf PUBLIC -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
