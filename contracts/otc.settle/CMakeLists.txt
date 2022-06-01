add_contract(otc.settle otc.settle ${CMAKE_CURRENT_SOURCE_DIR}/src/otc.settle.cpp)

target_include_directories(otc.settle
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(otc.settle
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/otc.settle.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/otc.settle.contracts.md @ONLY )

target_compile_options( otc.settle PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
