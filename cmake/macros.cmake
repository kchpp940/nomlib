# cmake/macros.cmake:jeff
#
# Helper macros for CMake build scripts
#

# Add runtime search path to our application bundle so that we can find its
# dependencies at launch.
#
# REMOVED: The manual install_name_tool functionality has been removed.
#          RPATH is now handled entirely by native CMake target properties
#          inside nom_add_library() (cmake/functions.cmake):
#            set_target_properties(target PROPERTIES
#              MACOSX_RPATH TRUE
#              INSTALL_RPATH_USE_LINK_PATH TRUE
#              INSTALL_RPATH "@loader_path/../lib;${CMAKE_INSTALL_PREFIX}/lib"
#              BUILD_RPATH   "${CMAKE_BINARY_DIR}/lib")
#          Calling this macro now does nothing except emit a warning.
macro ( add_rpath target rpath binary_path )
  message( WARNING
    "add_rpath() has been removed; RPATH is now configured via native CMake "
    "target properties in nom_add_library(). Remove this call from your build." )
endmacro ( add_rpath target rpath binary_path )

# Modify runtime search path for a library or application
#
# REMOVED: See add_rpath() above. RPATH is now handled natively by CMake.
macro ( change_rpath old_rpath new_rpath binary_path )
  message( WARNING
    "change_rpath() has been removed; RPATH is now configured via native "
    "CMake target properties in nom_add_library(). Remove this call." )
endmacro ( change_rpath rpath binary_path )

# Change the install name path of a library
#
# REMOVED: See add_rpath() above. Install names are now handled natively by
#          CMake via BUILD_WITH_INSTALL_NAME_DIR / INSTALL_NAME_DIR properties.
macro ( install_name_rpath rpath binary_path )
  message( WARNING
    "install_name_rpath() has been removed; install names are now configured "
    "via native CMake target properties. Remove this call." )
endmacro ( install_name_rpath rpath binary_path )

# Helper function for adding tests through CTest
#
# IMPORTANT: We cannot use the GTEST_ADD_TESTS macro here for adding tests that
# rely on the nom::VisualUnitTest framework because of the way that the macro
# breaks up the test run -- it ends up executing each individual test in a
# separate process, i.e.: 'SpriteTest.SpriteInterfaceWithTextureReference' and
# 'SpriteTest.SpriteInterfaceWithTextureRawPointer' are treated as two
# separated executable binaries.
#  This is bad for us because our screen-dumping creates new timestamped
# directories on every new instance of the framework, which normally is OK
# because this yields one directory, but in the case of multiple executable
# runs ... spawns an awful lot more than I'd prefer.
#   I hope to one day figure out a proper solution for this work flow issue,
# but in the mean time ... this is the best I can come up with.
macro( nom_add_visual_test test_name executable )
  add_test( ${test_name} ${executable}
            --gtest_filter=${test_name}.* ${ARGN} )
endmacro()

# Helper function for adding an engine unit test
#
# IMPORTANT: Avoid using the newer add_test syntax, i.e.:
# add_test(NAME <name> COMMAND <command>), because these tests are not
# added to the default test configuration! Using the newer add_test
# syntax leads me to this err message when running ctest from the project's
# build directory (CMake generated XCode project files):
#     "Test not available without configuration. (Missing "-C <config>"?)"
macro( nom_add_test test_name test_executable )
  add_test( ${test_name} ${test_executable} ${ARGN} )
endmacro()

macro(NOM_LOG_INFO msg)
  message( STATUS "INFO: ${msg}" )
endmacro(NOM_LOG_INFO msg)

macro(NOM_LOG_WARN msg)
  message( WARNING "WARN: ${msg}" )
endmacro(NOM_LOG_WARN msg)

macro(NOM_LOG_CRIT msg)
  message( FATAL_ERROR "CRITICAL: ${msg}" )
endmacro(NOM_LOG_CRIT msg)
