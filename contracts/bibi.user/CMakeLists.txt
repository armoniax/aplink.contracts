add_contract(bibiuser bibiuser ${CMAKE_CURRENT_SOURCE_DIR}/src/bibiuser.cpp)

target_include_directories(bibiuser
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(bibiuser
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/bibiuser.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/bibiuser.contracts.md @ONLY )

target_compile_options( bibiuser PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
