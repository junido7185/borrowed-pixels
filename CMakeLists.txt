cmake_minimum_required(VERSION 3.25)

project(BorrowedPixels VERSION 2.1 LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC)
  message(FATAL_ERROR "BORROWED PIXELS v2.1 requires Windows x64 with MSVC.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "BORROWED PIXELS v2.1 requires an x64 generator.")
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Test;Contest" CACHE STRING "Supported configurations" FORCE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BP_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${BP_GENERATED_DIR}")

set(BP_WIN32_STUB "${BP_GENERATED_DIR}/bp_win32_stub.cpp")
file(WRITE "${BP_WIN32_STUB}" [=[
#include <windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
  return 0;
}
]=])

set(BP_TEST_STUB "${BP_GENERATED_DIR}/bp_test_stub.cpp")
file(WRITE "${BP_TEST_STUB}" [=[
int main() noexcept {
  return 0;
}
]=])

file(GLOB BP_RUNTIME_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
if(NOT BP_RUNTIME_SOURCES)
  set(BP_RUNTIME_SOURCES "${BP_WIN32_STUB}")
endif()

file(GLOB BP_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
set(BP_TEST_SUPPORT_SOURCES ${BP_RUNTIME_SOURCES})
list(FILTER BP_TEST_SUPPORT_SOURCES EXCLUDE REGEX "/bp_win32\\.cpp$")
if(NOT BP_TEST_SOURCES)
  set(BP_TEST_SOURCES "${BP_TEST_STUB}")
  set(BP_TEST_SUPPORT_SOURCES "")
endif()

add_executable(bp_debug WIN32 ${BP_RUNTIME_SOURCES})
target_include_directories(bp_debug PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(bp_debug PRIVATE BP_DEBUG=1 BP_ENABLE_ASSERTS=1 BP_LEDGER_LOG=1)
target_compile_options(bp_debug PRIVATE /W4 /permissive- /Zc:preprocessor /Od /Zi)
target_link_options(bp_debug PRIVATE /DEBUG /INCREMENTAL:NO)
set_property(TARGET bp_debug PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")

add_executable(bp_test ${BP_TEST_SOURCES} ${BP_TEST_SUPPORT_SOURCES})
target_include_directories(bp_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(bp_test PRIVATE BP_TEST=1 BP_ENABLE_ASSERTS=1)
target_compile_options(bp_test PRIVATE /W4 /permissive- /Zc:preprocessor /Od /Zi)
target_link_options(bp_test PRIVATE /DEBUG /INCREMENTAL:NO)
set_property(TARGET bp_test PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")

add_executable(bp_contest WIN32 ${BP_RUNTIME_SOURCES})
target_include_directories(bp_contest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(bp_contest PRIVATE BP_CONTEST=1)
target_compile_options(bp_contest PRIVATE
  /std:c++20
  /O1
  /GL
  /Gy
  /Gw
  /GR-
  /W4
  /permissive-
  /Zc:inline
  /Zc:preprocessor
  /GS
  /guard:cf
)
target_link_options(bp_contest PRIVATE
  /LTCG
  /OPT:REF
  /OPT:ICF
  /INCREMENTAL:NO
  /SUBSYSTEM:WINDOWS
  /DYNAMICBASE
  /NXCOMPAT
  /guard:cf
  /BREPRO
)
set_property(TARGET bp_contest PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
set_target_properties(bp_contest PROPERTIES OUTPUT_NAME "BorrowedPixels")

foreach(BP_CONFIG IN ITEMS DEBUG TEST CONTEST)
  set_target_properties(bp_debug PROPERTIES
    "RUNTIME_OUTPUT_DIRECTORY_${BP_CONFIG}" "${CMAKE_CURRENT_BINARY_DIR}/bin/${BP_CONFIG}"
    "PDB_OUTPUT_DIRECTORY_${BP_CONFIG}" "${CMAKE_CURRENT_BINARY_DIR}/symbols/${BP_CONFIG}"
    "COMPILE_PDB_OUTPUT_DIRECTORY_${BP_CONFIG}" "${CMAKE_CURRENT_BINARY_DIR}/symbols/${BP_CONFIG}"
  )
  set_target_properties(bp_test PROPERTIES
    "RUNTIME_OUTPUT_DIRECTORY_${BP_CONFIG}" "${CMAKE_CURRENT_BINARY_DIR}/bin/${BP_CONFIG}"
    "PDB_OUTPUT_DIRECTORY_${BP_CONFIG}" "${CMAKE_CURRENT_BINARY_DIR}/symbols/${BP_CONFIG}"
    "COMPILE_PDB_OUTPUT_DIRECTORY_${BP_CONFIG}" "${CMAKE_CURRENT_BINARY_DIR}/symbols/${BP_CONFIG}"
  )
  set_target_properties(bp_contest PROPERTIES
    "RUNTIME_OUTPUT_DIRECTORY_${BP_CONFIG}" "${CMAKE_CURRENT_SOURCE_DIR}/dist/contest"
    "PDB_OUTPUT_DIRECTORY_${BP_CONFIG}" "${CMAKE_CURRENT_BINARY_DIR}/symbols/${BP_CONFIG}"
    "COMPILE_PDB_OUTPUT_DIRECTORY_${BP_CONFIG}" "${CMAKE_CURRENT_BINARY_DIR}/symbols/${BP_CONFIG}"
  )
endforeach()

enable_testing()
add_test(NAME bp_target_skeleton COMMAND "$<TARGET_FILE:bp_test>")
