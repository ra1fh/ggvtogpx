
cmake_minimum_required(VERSION 3.16)

project(ggvtogpx VERSION 1.0.0 LANGUAGES CXX)
include(CTest)
include(FindPkgConfig)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ENABLE_CLANG_TIDY OFF CACHE BOOL "Add clang-tidy automatically to builds")
if (ENABLE_CLANG_TIDY)
  find_program (CLANG_TIDY_EXE NAMES "clang-tidy")
  if (CLANG_TIDY_EXE)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
    set(CLANG_TIDY_CHECKS "google-*,cppcoreguidelines-*,-cppcoreguidelines-avoid-magic-numbers")
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}"
      CACHE STRING "" FORCE)
	list(
	  APPEND CMAKE_CXX_CLANG_TIDY
	  "-checks=${CLANG_TIDY_CHECKS}"
	  "-header-filter='${CMAKE_SOURCE_DIR}/*'")
  else()
    message(AUTHOR_WARNING "clang-tidy not found!")
    set(CMAKE_CXX_CLANG_TIDY "" CACHE STRING "" FORCE) # delete it
  endif()
endif()

set(ENABLE_CPPCHECK OFF CACHE BOOL "Add cppcheck automatically to builds")
if (ENABLE_CPPCHECK)
  find_program (CPPCHECK_EXE NAMES "cppcheck")
  if (CPPCHECK_EXE)
    message(STATUS "Found cppcheck: ${CPPCHECK_EXE}")
	set(CMAKE_CXX_CPPCHECK "${CPPCHECK_EXE}")
	list(
	  APPEND CMAKE_CXX_CPPCHECK
	  "--language=c++"
	  "--std=c++17"
	  "--enable=all"
	  "--suppress=missingIncludeSystem"
	  "--suppress=unusedFunction")
  else()
    message(AUTHOR_WARNING "cppcheck not found!")
    set(CMAKE_CXX_CPPCHECK "" CACHE STRING "" FORCE) # delete it
  endif()
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Xml)

find_library(
    LIBZIP_LIBRARY
    NAMES libzip)

pkg_check_modules (LIBZIP REQUIRED libzip)

add_compile_options(-Wall -Wextra)

add_executable(ggvtogpx
  format.cc
  geodata.cc
  gpx.cc
  ggv_bin.cc
  ggv_ovl.cc
  ggv_xml.cc
  ggvtogpx.cc
  )

target_include_directories(ggvtogpx SYSTEM PUBLIC
  ${LIBZIP_INCLUDE_DIRS})

target_link_directories(ggvtogpx PRIVATE
  ${LIBZIP_LIBRARY_DIRS})

target_link_libraries(ggvtogpx PRIVATE
  ${LIBZIP_LIBRARIES}
  Qt${QT_VERSION_MAJOR}::Core)

target_link_libraries(ggvtogpx PRIVATE Qt${QT_VERSION_MAJOR}::Xml)

install(TARGETS ggvtogpx)

add_custom_target(style
  astyle --options=astylerc *.h *.cc)

set (BinTestsToRun
  ggv_bin-sample-v2
  ggv_bin-sample-v3
  ggv_bin-sample-v4
  ggv_ovl-sample-1
  ggv_ovl-sample-2
  ggv_xml-sample-1
  ggv_xml-sample-2)

# Add all the ADD_TEST for each test
foreach (test ${BinTestsToRun})
  add_test (NAME ${test}-generate COMMAND ggvtogpx testdata/${test}.ovl testdata/${test}.out)
  add_test (NAME ${test}-diff COMMAND ${CMAKE_COMMAND} -E compare_files testdata/${test}.gpx testdata/${test}.out)
  set_tests_properties(${test}-generate PROPERTIES ENVIRONMENT "GGVTOGPX_TESTMODE=1")
endforeach ()
