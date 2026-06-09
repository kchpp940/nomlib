# Minimal nomlib consumer project
#
# This project is used by cmake/scripts/verify_install.sh to validate that an
# installed nomlib package can be consumed by an external CMake project.
#
# Usage (modern config-file package):
#   cmake -S . -B build -Dnomlib_DIR=<prefix>/lib/cmake/nomlib
#
# Usage (legacy find-module):
#   cmake -S . -B build -DCMAKE_MODULE_PATH=<prefix>/share/nomlib/CMake -Dnomlib_LEGACY_MODE=ON

cmake_minimum_required(VERSION 3.14)
project(nomlib_test_consumer CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(nomlib_LEGACY_MODE "Use the legacy Resources/CMake/nomlib-config.cmake wrapper" OFF)

if(nomlib_LEGACY_MODE)
  # Legacy path: CMAKE_MODULE_PATH should already point at the installed
  # share/nomlib/CMake directory, so find_module uses our old wrapper which
  # in turn delegates to the modern config.
  find_package(nomlib REQUIRED COMPONENTS core math)
else()
  # Modern path: nomlib_DIR should point at the installed lib/cmake/nomlib
  # directory (or set CMAKE_PREFIX_PATH to the install prefix).
  find_package(nomlib CONFIG REQUIRED COMPONENTS core math)
endif()

add_executable(test_consumer main.cpp)
target_link_libraries(test_consumer PRIVATE nomlib::nomlib-core nomlib::nomlib-math)

# Print a sanity summary for the verification script
message(STATUS "Consumer summary:")
message(STATUS "  NOMLIB_FOUND       = ${nomlib_FOUND}")
message(STATUS "  NOMLIB_INCLUDE_DIR = ${NOMLIB_INCLUDE_DIR}")
message(STATUS "  NOMLIB_RESOURCES_DIR = ${NOMLIB_RESOURCES_DIR}")
message(STATUS "  NOMLIB_LIBRARIES   = ${NOMLIB_LIBRARIES}")
message(STATUS "  nomlib_core_FOUND  = ${nomlib_core_FOUND}")
message(STATUS "  nomlib_math_FOUND  = ${nomlib_math_FOUND}")

# Verify resource file path variable works
if(NOT EXISTS "${NOMLIB_RESOURCES_DIR}/SharedSupport/InputDevices/gamecontrollerdb.txt")
  message(FATAL_ERROR "NOMLIB_RESOURCES_DIR does not point at a valid resources tree")
endif()
message(STATUS "  Resource file check: OK")
