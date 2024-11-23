cmake_minimum_required(VERSION 3.28)

project(led-matrix-zmq-server)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(LMZ_COMPILE_FEATURES cxx_std_20)
set(LMZ_COMPILE_OPTIONS -O3 -mtune=native -Wall -Wextra)

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

add_executable(led-matrix-zmq-server
  src/server_main.cpp
  src/color_temp.cpp
)
target_link_libraries(led-matrix-zmq-server PRIVATE
  argparse
  plog
  pthread
  RpiRgbLedMatrix::RpiRgbLedMatrix
  zmq
)
target_compile_features(led-matrix-zmq-server PRIVATE ${LMZ_COMPILE_FEATURES})
target_compile_options(led-matrix-zmq-server PRIVATE ${LMZ_COMPILE_OPTIONS})

add_executable(led-matrix-zmq-control
  src/control_main.cpp
)
target_link_libraries(led-matrix-zmq-control PRIVATE argparse plog zmq)
target_compile_features(led-matrix-zmq-control PRIVATE ${LMZ_COMPILE_FEATURES})

add_executable(led-matrix-zmq-pipe src/pipe_main.cpp)
target_link_libraries(led-matrix-zmq-pipe PRIVATE argparse plog zmq)
target_compile_features(led-matrix-zmq-pipe PRIVATE ${LMZ_COMPILE_FEATURES})
