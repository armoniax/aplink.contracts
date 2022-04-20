add_contract(aplink.token aplink.token ${CMAKE_CURRENT_SOURCE_DIR}/src/aplink.token.cpp)

target_include_directories(aplink.token
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(aplink.token
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/aplink.token.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/aplink.token.contracts.md @ONLY )

target_compile_options( aplink.token PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
