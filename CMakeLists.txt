
cmake_minimum_required(VERSION 3.16)

project(ggvtogpx VERSION 1.0.0 LANGUAGES CXX)
include(CTest)

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

install(TARGETS ggvtogpx)

add_custom_target(style
  astyle --options=astylerc *.h *.cc)

set (TestsToRun ggv_bin-sample-v2 ggv_bin-sample-v3)

# Add all the ADD_TEST for each test
foreach (test ${TestsToRun})
  add_test (NAME ${test}-generate COMMAND ggvtogpx testdata/${test}.ovl testdata/${test}.out)
  add_test (NAME ${test}-diff COMMAND ${CMAKE_COMMAND} -E compare_files testdata/${test}.gpx testdata/${test}.out)
  set_tests_properties(${test}-generate PROPERTIES ENVIRONMENT "GGVTOGPX_TESTMODE=1")
endforeach ()
