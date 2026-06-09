# Minimal nomlib consumer project
#
# This project is used by cmake/scripts/verify_install.sh to validate that an
# installed nomlib package can be consumed by an external CMake project.
#
# Usage (modern config-file package):
#   cmake -S . -B build -Dnomlib_DIR=<prefix>/lib/cmake/nomlib \
#       -DTEST_COMPONENTS="core;math"
#
# Usage (legacy find-module):
#   cmake -S . -B build \
#       -DCMAKE_MODULE_PATH=<prefix>/share/nomlib/CMake \
#       -Dnomlib_LEGACY_MODE=ON -DTEST_COMPONENTS="core;math"

cmake_minimum_required(VERSION 3.14)
project(nomlib_test_consumer CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(nomlib_LEGACY_MODE "Use the legacy Resources/CMake/nomlib-config.cmake wrapper" OFF)
set(TEST_COMPONENTS "core;math" CACHE STRING "Semicolon-separated list of nomlib components to consume")
set(TEST_MISSING_COMPONENTS "" CACHE STRING "Semicolon-separated list of components expected to be NOT found")

# --- Locate the package -----------------------------------------------------
if(nomlib_LEGACY_MODE)
  find_package(nomlib REQUIRED COMPONENTS ${TEST_COMPONENTS})
else()
  find_package(nomlib CONFIG REQUIRED COMPONENTS ${TEST_COMPONENTS})
endif()

# --- Sanity checks on requested components ----------------------------------
foreach(_c ${TEST_COMPONENTS})
  if(NOT TARGET nomlib::nomlib-${_c})
    message(FATAL_ERROR "Requested component nomlib::nomlib-${_c} does not exist as an imported target")
  endif()
  if(NOT nomlib_${_c}_FOUND)
    message(FATAL_ERROR "Requested component ${_c} is not marked as found (nomlib_${_c}_FOUND=FALSE)")
  endif()
  message(STATUS "  Component ${_c}: OK (target exists + nomlib_${_c}_FOUND=TRUE)")
endforeach()

# --- Sanity checks on MISSING components ------------------------------------
foreach(_c ${TEST_MISSING_COMPONENTS})
  if(TARGET nomlib::nomlib-${_c})
    message(FATAL_ERROR "Component ${_c} was NOT requested but nomlib::nomlib-${_c} target exists!")
  endif()
  if(nomlib_${_c}_FOUND)
    message(FATAL_ERROR "Component ${_c} was NOT requested but nomlib_${_c}_FOUND=TRUE!")
  endif()
  message(STATUS "  Component ${_c}: correctly absent")
endforeach()

# --- Build the test executable ----------------------------------------------
add_executable(test_consumer main.cpp)

# Link against all requested components
set(_link_libs "")
foreach(_c ${TEST_COMPONENTS})
  list(APPEND _link_libs nomlib::nomlib-${_c})
endforeach()
target_link_libraries(test_consumer PRIVATE ${_link_libs})

# --- Print summary ----------------------------------------------------------
message(STATUS "Consumer summary:")
message(STATUS "  NOMLIB_FOUND       = ${nomlib_FOUND}")
message(STATUS "  NOMLIB_INCLUDE_DIR = ${NOMLIB_INCLUDE_DIR}")
message(STATUS "  NOMLIB_RESOURCES_DIR = ${NOMLIB_RESOURCES_DIR}")
message(STATUS "  NOMLIB_LIBRARIES   = ${NOMLIB_LIBRARIES}")
message(STATUS "  TEST_COMPONENTS    = ${TEST_COMPONENTS}")
message(STATUS "  TEST_MISSING_COMPS = ${TEST_MISSING_COMPONENTS}")

# Verify resource file path variable works
if(NOT EXISTS "${NOMLIB_RESOURCES_DIR}/SharedSupport/InputDevices/gamecontrollerdb.txt")
  message(FATAL_ERROR "NOMLIB_RESOURCES_DIR does not point at a valid resources tree")
endif()
message(STATUS "  Resource file check: OK")
