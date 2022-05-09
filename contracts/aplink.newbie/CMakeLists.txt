add_contract(aplink.newbie aplink.newbie ${CMAKE_CURRENT_SOURCE_DIR}/src/aplink.newbie.cpp)

target_include_directories(aplink.newbie
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/include/aplink.newbie
   ${CMAKE_CURRENT_SOURCE_DIR}/../aplink.token/include
)

set_target_properties(aplink.newbie
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( aplink.newbie PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
