file(GLOB_RECURSE SOURCE_CORE
  ${CMAKE_CURRENT_SOURCE_DIR}/thpool.c
  ${CMAKE_CURRENT_SOURCE_DIR}/thpool.h)

add_library(thread_pool STATIC ${SOURCE_CORE})

include_directories(thread_pool ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(thread_pool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(thread_pool
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")