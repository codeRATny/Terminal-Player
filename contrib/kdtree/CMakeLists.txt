file(GLOB_RECURSE SOURCE_CORE
  ${CMAKE_CURRENT_SOURCE_DIR}/kdtree.c
  ${CMAKE_CURRENT_SOURCE_DIR}/kdtree.h )

add_library(kd_tree STATIC ${SOURCE_CORE})

include_directories(kd_tree ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(kd_tree PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(kd_tree
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")