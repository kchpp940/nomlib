#!/bin/sh

# DEPRECATED (2024-06): This script is no longer used.
#
# RPATH handling has been migrated to native CMake target properties in
# cmake/functions.cmake (nom_add_library). All nomlib libraries now export:
#   - MACOSX_RPATH TRUE
#   - INSTALL_RPATH_USE_LINK_PATH TRUE
#   - INSTALL_RPATH "@loader_path/../lib;<install-prefix>/lib"
#   - BUILD_RPATH   "<build-dir>/lib"
# plus FRAMEWORK-specific variants. See cmake/functions.cmake for details.
#
# This wrapper intentionally performs NO action and exits 0, so any legacy
# invocation is a harmless no-op. Remove the call site from your build.

echo "WARNING: add_rpath.sh is deprecated and performs no action." 1>&2
echo "         Use native CMake RPATH target properties instead."      1>&2
exit 0
