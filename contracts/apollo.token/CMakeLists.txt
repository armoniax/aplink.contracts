add_contract(apollo.token apollo.token ${CMAKE_CURRENT_SOURCE_DIR}/src/apollo.token.cpp)

target_include_directories(apollo.token
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(apollo.token
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/apollo.token.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/apollo.token.contracts.md @ONLY )

target_compile_options( apollo.token PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
