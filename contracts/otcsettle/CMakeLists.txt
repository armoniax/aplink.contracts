add_contract(otcsettle otcsettle ${CMAKE_CURRENT_SOURCE_DIR}/src/otcsettle.cpp)

target_include_directories(otcsettle
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../otcconf/include )

set_target_properties(otcsettle
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/otcsettle.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/otcsettle.contracts.md @ONLY )

target_compile_options( otcsettle PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
