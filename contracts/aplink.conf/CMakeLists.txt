add_contract(aplink.conf aplink.conf ${CMAKE_CURRENT_SOURCE_DIR}/src/aplink.conf.cpp)

target_include_directories(aplink.conf
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(aplink.conf
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/aplink.conf.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/aplink.conf.contracts.md @ONLY )

target_compile_options( aplink.conf PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
