add_contract(aplink.event aplink.event ${CMAKE_CURRENT_SOURCE_DIR}/src/aplink.event.cpp)

target_include_directories(aplink.event
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/include/aplink.event
   ${CMAKE_CURRENT_SOURCE_DIR}/../aplink.token/include
)

set_target_properties(aplink.event
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( aplink.event PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
