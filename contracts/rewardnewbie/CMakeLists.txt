add_contract(rewardnewbie rewardnewbie ${CMAKE_CURRENT_SOURCE_DIR}/src/rewardnewbie.cpp)

target_include_directories(rewardnewbie
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/include/rewardnewbie )

set_target_properties(rewardnewbie
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( rewardnewbie PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
