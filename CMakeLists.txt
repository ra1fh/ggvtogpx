
cmake_minimum_required(VERSION 3.16)

project(ggvtogpx VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)
qt_standard_project_setup()

add_executable(ggvtogpx
  fatal.cc
  ggv_bin.cc
  ggvtogpx.cc
)

target_link_libraries(ggvtogpx PRIVATE Qt6::Core)
