project(terminal-player)
cmake_minimum_required(VERSION 3.1.3)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g")
set(TARGET_NAME "terminal-player")


file(GLOB SRC_LIST
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libswscale
    libavutil
)

add_executable(${TARGET_NAME} ${SRC_LIST})
target_link_libraries(${TARGET_NAME} PkgConfig::LIBAV)
# target_compile_options(main PRIVATE -fsanitize=leak)
# target_link_options(main PRIVATE -fsanitize=leak)