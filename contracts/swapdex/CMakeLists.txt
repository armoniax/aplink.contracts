add_contract(swapdex swapdex ${CMAKE_CURRENT_SOURCE_DIR}/evolutiondex.cpp ${CMAKE_CURRENT_SOURCE_DIR}/token_functions.cpp)

target_include_directories(swapdex
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR} )

set_target_properties(swapdex
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( swapdex PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
