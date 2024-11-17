cmake_minimum_required(VERSION 3.18)

project(led-matrix-zmq-server)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mtune=native -Wall -Wextra")

find_package(PkgConfig REQUIRED)
find_package(RpiRgbLedMatrix REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

include(FetchContent)
FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/p-ranav/argparse.git
  GIT_TAG v3.1
)
FetchContent_Declare(
  plog
  GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git
  GIT_TAG 1.1.10
)
FetchContent_MakeAvailable(argparse plog)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(led-matrix-zmq-server src/server_main.cpp src/color_temp.cpp)
target_link_libraries(led-matrix-zmq-server PRIVATE
  argparse
  plog
  pthread
  RpiRgbLedMatrix::RpiRgbLedMatrix
  zmq
)

add_executable(led-matrix-zmq-control src/control_main.cpp)
target_link_libraries(led-matrix-zmq-control PRIVATE argparse plog zmq)

add_executable(led-matrix-zmq-pipe src/pipe_main.cpp)
target_link_libraries(led-matrix-zmq-pipe PRIVATE argparse plog zmq)
