cmake_minimum_required(VERSION 3.18)

project(matryx_zmq)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(matryx_zmq src/main.cpp)

target_include_directories(matryx_zmq PRIVATE ../rpi-rgb-led-matrix/include)
target_link_directories(matryx_zmq PRIVATE ../rpi-rgb-led-matrix/lib)
target_link_libraries(matryx_zmq PRIVATE zmq rgbmatrix pthread)

target_compile_options(matryx_zmq PRIVATE
  -O3
  -march=native
  -Wall
)

add_executable(matryx_zmq_control src/control.cpp)
target_link_libraries(matryx_zmq_control PRIVATE zmq)

target_compile_options(matryx_zmq_control PRIVATE
  -O3
  -march=native
  -Wall
)
