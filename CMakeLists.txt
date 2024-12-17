cmake_minimum_required(VERSION 3.28)

project(led-matrix-zmq-server)

option(BUILD_SERVER "Build the server" ON)
option(BUILD_TOOLS "Build tools" ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(COMPILE_FEATURES cxx_std_20)
set(COMPILE_OPTIONS -O3 -Wall -Wextra)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq cppzmq)

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

if(
  BUILD_SERVER
  AND (
    CMAKE_SYSTEM_PROCESSOR MATCHES "arm"
    OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64"
  )
)
  find_package(RpiRgbLedMatrix REQUIRED)

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
  target_compile_features(led-matrix-zmq-server PRIVATE ${COMPILE_FEATURES})
  target_compile_options(led-matrix-zmq-server PRIVATE ${COMPILE_OPTIONS})
endif()

if (BUILD_TOOLS)
  add_executable(led-matrix-zmq-control src/control_main.cpp)
  target_link_libraries(led-matrix-zmq-control PRIVATE argparse plog zmq)
  target_compile_features(led-matrix-zmq-control PRIVATE ${COMPILE_FEATURES})

  add_executable(led-matrix-zmq-pipe src/pipe_main.cpp)
  target_link_libraries(led-matrix-zmq-pipe PRIVATE argparse plog zmq)
  target_compile_features(led-matrix-zmq-pipe PRIVATE ${COMPILE_FEATURES})
endif()
