# cmake/functions.cmake:jeff
#
# Helper functions for CMake build scripts
#

# Create and link a library module
#
# source parameter should be enclosed within double quotes.
# headers parameter is not implemented; reserved for future implementation.
#
# external_deps parameters should be separated by semicolons when multiple
# dependencies are specified and enclosed within double quotes.
#
# TODO: Future expansion of this macro should strongly consider refactoring with
# the use of the CMakeParseArguments module.
# http://www.cmake.org/cmake/help/v3.0/module/CMakeParseArguments.html
function(nom_add_library target lib_type source headers external_deps )
  # The library version defines the full build version of said library and
  # is used in the actual filename on disk.
  set ( LIB_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}" )

  # This is embedded as the SONAME in the library's ELF header and determinnes
  # which file the dynamic linker searches for at runtime, i.e.:
  # `libnomlib-actions.so.1 -> `libnomlib-actions.so.0.11`
  # The Application Binary Interface (ABI) version; PATCH level versions are
  # intended **not** to break the ABI version.
  set ( LIB_SOVERSION ${PROJECT_VERSION_MAJOR} )

  add_library( ${target} ${lib_type} ${source} )

  set_target_properties( ${target} PROPERTIES VERSION ${LIB_VERSION}
                         SOVERSION ${LIB_SOVERSION} DEBUG_POSTFIX "-d" )
  target_link_libraries( ${target} ${external_deps} )

  if( PLATFORM_OSX AND FRAMEWORK )

    # Create target.framework
    set_target_properties(  ${target} PROPERTIES
                            FRAMEWORK TRUE
                            MACOSX_FRAMEWORK_INFO_PLIST
                            "${CMAKE_TEMPLATE_PATH}/Info.plist.in"
                            MACOSX_FRAMEWORK_NAME
                            "${target}"
                            MACOSX_FRAMEWORK_BUNDLE_VERSION
                            "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}-${CMAKE_BUILD_TYPE}"
                            MACOSX_FRAMEWORK_SHORT_VERSION_STRING
                            "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
                            MACOSX_FRAMEWORK_IDENTIFIER
                            "net.i8degrees.${target}"
                            # PUBLIC_HEADER
                            # "${source}"
    )
  endif( PLATFORM_OSX AND FRAMEWORK )

  # Copy target's library file to $CMAKE_INSTALL_PREFIX/lib
  install(  TARGETS ${target}
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            LIBRARY FRAMEWORK DESTINATION ${CMAKE_INSTALL_PREFIX} )

endfunction(nom_add_library)

# Copy resource files for engine examples and tests.
# nom_install_resources(spath, dpath, options)
function(install_resource_file spath dpath)
  install(
    FILES "${spath}"
    DESTINATION "${dpath}")
endfunction()

function(install_resource_dir spath dpath)
  install(
    DIRECTORY "${spath}"
    DESTINATION "${dpath}"
    PATTERN ".*" EXCLUDE )
endfunction()

# ============================================================================
# nom_probe_third_party()
#
# Centralized detection of all optional/conditional third-party libraries.
#
# Different CMake Find modules produce *_FOUND variables with inconsistent
# naming conventions, and some modules may not be present on all systems.
# This function wraps each find_package call and normalizes the result into
# project-owned boolean variables that the validation macro and source-tree
# CMakeLists can rely on without worrying about Find-module internals.
#
# Output variables (set in parent scope):
#   NOM_HAVE_OPENAL     — TRUE if OpenAL (or OpenAL-Soft / Apple OpenAL)
#                         headers and libraries were located.
#   NOM_HAVE_LIBSNDFILE — TRUE if libsndfile headers and library were located.
#   NOM_HAVE_LIBROCKET  — TRUE if libRocket (Core + Controls + Debugger)
#                         headers and libraries were located.
#
# This function should be called exactly ONCE from the root CMakeLists.txt,
# AFTER platform-specific environment variables (OPENALDIR, LIBSNDFILEDIR,
# LIBROCKETDIR, etc.) have been populated.
# ============================================================================
function(nom_probe_third_party)

  # -- OpenAL ----------------------------------------------------------------
  # CMake built-in FindOpenAL.cmake sets OPENAL_FOUND.
  find_package(OpenAL QUIET)
  if(OPENAL_FOUND)
    set(NOM_HAVE_OPENAL TRUE PARENT_SCOPE)
    message(STATUS "Third-party: OpenAL found (OPENAL_INCLUDE_DIR=${OPENAL_INCLUDE_DIR})")
  else()
    set(NOM_HAVE_OPENAL FALSE PARENT_SCOPE)
    message(STATUS "Third-party: OpenAL NOT found — audio module will be unavailable.")
  endif()

  # -- libsndfile ------------------------------------------------------------
  # Project custom Findlibsndfile.cmake sets LIBSNDFILE_FOUND.
  find_package(libsndfile QUIET)
  if(LIBSNDFILE_FOUND)
    set(NOM_HAVE_LIBSNDFILE TRUE PARENT_SCOPE)
    message(STATUS "Third-party: libsndfile found (LIBSNDFILE_INCLUDE_DIR=${LIBSNDFILE_INCLUDE_DIR})")
  else()
    set(NOM_HAVE_LIBSNDFILE FALSE PARENT_SCOPE)
    message(STATUS "Third-party: libsndfile NOT found — audio module will be unavailable.")
  endif()

  # -- libRocket -------------------------------------------------------------
  # Project custom FindLibRocket.cmake sets LIBROCKET_FOUND.
  find_package(LibRocket QUIET)
  if(LIBROCKET_FOUND)
    set(NOM_HAVE_LIBROCKET TRUE PARENT_SCOPE)
    message(STATUS "Third-party: libRocket found (LIBROCKET_INCLUDE_DIRS=${LIBROCKET_INCLUDE_DIRS})")
  else()
    set(NOM_HAVE_LIBROCKET FALSE PARENT_SCOPE)
    message(STATUS "Third-party: libRocket NOT found — GUI module will be unavailable.")
  endif()

endfunction(nom_probe_third_party)

#
# target parameter is not implemented; reserved for future implementation.
#
# dest parameter is not implemented; reserved for future implementation.
# macro(nom_install_dep target external_deps dest)

#   # Bundle the appropriate external dependencies
#   foreach( dep ${external_deps} )

#     if( IS_DIRECTORY ${dep} )

#       # Bundle frameworks we depend on that are not system library bundles
#       install(  DIRECTORY ${dep}
#                 DESTINATION "nomlib.framework/Frameworks"
#                 PATTERN ".*" EXCLUDE )

#     else( NOT IS_DIRECTORY ${dep} )

#       # if( IS_SYMLINK ${dep} )
#       #   # Resolve real file path when symbolic so CMake's install command
#       #   # copies the real file
#       #   get_filename_component( dep ${dep} REALPATH )
#       # endif( IS_SYMLINK ${dep} )
#       # message( STATUS "DEP IS A FILE: ${dep}" )

#       # Bundle dynamic libraries (*.dylib) that we depend on
#       install(  FILES ${dep}
#                 DESTINATION "nomlib.framework/Frameworks"
#                 PATTERN ".*" EXCLUDE )

#       endif( IS_DIRECTORY ${dep} )
#     endforeach( dep ${external_deps} )

# endmacro(nom_install_dep target external_deps dest)
