# nomlib legacy find-module (DEPRECATED)
#
# This file is installed only for backward compatibility. New projects should
# rely on the modern CMake config-file package, which is installed to:
#
#   POSIX:      <prefix>/lib/cmake/nomlib/nomlib-config.cmake
#   OS X Framew.: nomlib.framework/Resources/CMake/nomlib-config.cmake
#
# This wrapper simply delegates to the modern config if it can be found, so
# that existing projects using:
#
#   list(APPEND CMAKE_MODULE_PATH "<prefix>/share/nomlib/CMake")
#   find_package(nomlib)
#
# continue to work transparently.
#
# Copyright (c) 2014-2024 Jeffrey Carpenter <i8degrees@gmail.com>
# Distributed under the Simplified BSD License; see accompanying file LICENSE.md.

# Look for the modern config next to this file (framework/share layout) and
# in the canonical lib/cmake/<name> location.
get_filename_component(_nomlib_legacy_dir "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

set(_nomlib_modern_search_paths
  "${_nomlib_legacy_dir}"
  "${_nomlib_legacy_dir}/../../lib/cmake/nomlib"
  "${_nomlib_legacy_dir}/../../../lib/cmake/nomlib"
  "${NOMLIB_ROOT}/lib/cmake/nomlib"
  "$ENV{NOMLIB_ROOT}/lib/cmake/nomlib"
  ~/Library/Frameworks/nomlib.framework/Resources/CMake
  /Library/Frameworks/nomlib.framework/Resources/CMake
)

find_file(_nomlib_modern_config
  NAMES nomlib-config.cmake
  PATHS ${_nomlib_modern_search_paths}
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)

if(_nomlib_modern_config)
  if(NOT nomlib_FIND_QUIETLY)
    message(STATUS
      "[nomlib legacy wrapper] Delegating to modern package config: ${_nomlib_modern_config}"
    )
  endif()
  include("${_nomlib_modern_config}")
  unset(_nomlib_modern_config)
  unset(_nomlib_modern_search_paths)
  unset(_nomlib_legacy_dir)
  return()
endif()

unset(_nomlib_modern_config)
unset(_nomlib_modern_search_paths)
unset(_nomlib_legacy_dir)

# The modern config was not found; we cannot proceed. The legacy hand-written
# find-module logic has been removed in favor of the single source of truth
# generated at install time by configure_package_config_file().
message(FATAL_ERROR
  "nomlib legacy find-module could not locate the modern nomlib-config.cmake. "
  "Please ensure nomlib is installed, or set NOMLIB_ROOT to its install prefix. "
  "For new projects prefer: find_package(nomlib CONFIG)."
)
