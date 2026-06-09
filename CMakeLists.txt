#cmake_minimum_required( VERSION 2.8.12 )
cmake_minimum_required( VERSION 3.14 )

include ( "${CMAKE_SOURCE_DIR}/version.cmake" )

# Enable the use of MACOSX_RPATH by default; this effectively allows plug 'n'
# play functionality, so to speak -- the resulting shared library files can
# simply be copied over into the end-user's application bundle or framework
# bundle. No mucking around with install_name_tool.
#
# (Minimum required CMake: v2.8.12)
#
#   See also:
#
# cmake --help-policy cmp0042
# http://www.kitware.com/blog/home/post/510
if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif(POLICY CMP0042)

# CMake Environment

# `find_package()` uses upper-case `PACKAGENAME_ROOT` variables.
# cmake --help-policy CMP0144
# Minimum required CMake v3.27
if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif(POLICY CMP0144)

# PythonInterp modules; acknowledgement
# cmake --help-policy CMP0148
#cmake_policy(SET CMP0148 NEW)

# Opt out of using CMake v3.0 PROJECT_VERSION variables management for the
# project.
# http://www.cmake.org/cmake/help/v3.0/command/project.html#command:project
#if( POLICY CMP0048 )
#cmake_policy( SET CMP0048 OLD )
#endif( POLICY CMP0048 )

if(POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif(POLICY CMP0072)

# set ( CMAKE_VERBOSE_MAKEFILE OFF CACHE PATH "Verbose Makefile" )
set ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" )
set ( CMAKE_TEMPLATE_PATH ${CMAKE_TEMPLATE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates" )

# Project Options
option ( DEBUG "Build with all debugging features" off )
option ( DEBUG_ASSERT "Build with run-time assertions enabled" off )
# ?? Replace DEBUG_ASSERT option with the following; this should result in assertions
# ?? being left on unless explicitly being disabled when DEBUG builds are present.
#option( DISABLE_DEBUG_ASSERTIONS "" off)
option( EXAMPLES "Build nomlib usage examples" off )
option( NOM_BUILD_TESTS "Build unit tests" off )
option( NOM_INSTALL_GENERATED_DOCS "Build docs" off )

option( NOM_BUILD_ACTIONS_UNIT "Animations subsystem" ON)
option( NOM_BUILD_CORE_UNIT "Engine core" ON)
option( NOM_BUILD_MATH_UNIT "Math utilities" ON)
option( NOM_BUILD_FILE_UNIT "Filesystem access" ON)
option( NOM_BUILD_AUDIO_UNIT "Audio subsystem" OFF)
option( NOM_BUILD_GRAPHICS_UNIT "Graphics core" ON)
option( NOM_BUILD_EXTRA_RESCALE_ALGO_UNIT "Build with scale2x & hqx algorithms" ON)
option( NOM_BUILD_SYSTEM_UNIT "System unit" ON)
option( NOM_BUILD_PTREE_UNIT "Generic container for serialization" ON)
option( NOM_BUILD_SERIALIZERS_UNIT "Serialization for JSON, XML, HTML" ON)
option( NOM_BUILD_GUI_UNIT "GUI subsystem" ON)

# IMPORTANT(JEFF): This initializes our engine versioning variables;
# `PROJECT_VERSION_MAJOR` & friends at `include/nomlib/version.hpp.in`
project( nomlib
  VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
  DESCRIPTION "C++11 2D game framework using SDL2"
  #HOMEPAGE_URL "https://github.com/i8degrees/nomlib"
  #SPDX_LICENSE "BSD-2-Clause-FreeBSD"
  #LANGUAGES "en"
)

# Platform detection
include ( "${CMAKE_SOURCE_DIR}/cmake/platform.cmake" )
# Utility macro helpers
include ( "${CMAKE_CURRENT_SOURCE_DIR}/cmake/functions.cmake" )
include ( "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macros.cmake" )
# Package configuration helpers
include(CMakePackageConfigHelpers)

if ( PLATFORM_WINDOWS )
  # Building nomlib on Windows is only supported as a static library for the
  # moment
  set ( BUILD_SHARED_LIBS off )
else ( NOT PLATFORM_WINDOWS )
  # We only support building nomlib as a dynamic library at the moment
  set ( BUILD_SHARED_LIBS on )
endif ( PLATFORM_WINDOWS )

# TODO: We should be setting CMAKE_BUILD_TYPE directly from the command line
# instead of DEBUG
set ( CMAKE_VERBOSE_MAKEFILE ON )
if ( DEBUG )
  set ( CMAKE_BUILD_TYPE "Debug" )
  # >> Explicit `DEBUG_ASSERT` option will be deprecated in a future version...
  if ( NOT DISABLE_DEBUG_ASSERTIONS )
    set ( DEBUG_ASSERT on )
  endif ( NOT DISABLE_DEBUG_ASSERTIONS )

  if ( NOT PLATFORM_WINDOWS )
    # TODO: Enable -Wsign-conversion, -Wextra
    # TODO: Consider enabling some of these run-time checks that clang offers,
    # i.e.:: -fsanitize=address, -fsanitize=integer, -fsanitize=undefined,
    # -fsanitize=unsigned-integer-overflow, -fsanitize=float-divide-by-zero, etc.
    # See also: http://clang.llvm.org/docs/UsersManual.html#controlling-code-generation
    # See also: Malloc Debugging under OSX: https://developer.apple.com/library/mac/documentation/Performance/Conceptual/ManagingMemory/Articles/MallocDebug.html#//apple_ref/doc/uid/20001884-CJBJFIDD
    set ( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D NOM_DEBUG -Wall -Wunused -O0" )

    # Measure the time it takes per source file to build, so we can try
    # speeding up slow builds!
    # set ( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ftime-report" )
  else ( PLATFORM_WINDOWS )
    # TODO: split these options up like we have for other platforms
    set ( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D NOM_DEBUG /D NOM_DEBUG_ASSERT" )
  endif( NOT PLATFORM_WINDOWS )

  message ( STATUS "Building ${PROJECT_NAME} with debugging." )
  message ( STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}" )
else ( NOT DEBUG ) # Build with optimizations for maximum speed and minimal size
  set ( CMAKE_BUILD_TYPE "Release" )
  message ( STATUS "Building ${PROJECT_NAME} with high speed, low drag!" )
  message ( STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}" )
endif ( DEBUG )

if ( DEBUG_ASSERT )
  add_definitions ( "-DNOM_DEBUG_ASSERT" ) # Enable NOM_ASSERT macros
  message ( STATUS "Run-time assertions are ON." )
else ( NOT DEBUG_ASSERT )
  add_definitions ( "-DNDEBUG" ) # Disable NOM_ASSERT macros
  message ( STATUS "Run-time assertions are OFF." )
endif ( DEBUG_ASSERT )

if( BUILD_SHARED_LIBS )
  set( LIBRARY_OUTPUT_TYPE "SHARED" )
else( NOT BUILD_SHARED_LIBS )
  set( LIBRARY_OUTPUT_TYPE "STATIC" )
endif( BUILD_SHARED_LIBS )

# TODO: Rename to NOMLIB_SRC_DIR..?
set( SRC_DIR "${PROJECT_SOURCE_DIR}/src" )
set( NOMLIB_RESOURCES_DIR "${PROJECT_SOURCE_DIR}/Resources" )

set( NOMLIB_SHARED_SUPPORT_DIR "${NOMLIB_RESOURCES_DIR}/SharedSupport" )

# TODO: Rename to NOMLIB_INCLUDE_ROOT_DIR..?
set( INC_ROOT_DIR "${PROJECT_SOURCE_DIR}/include" )

# TODO: Rename to NOMLIB_INCLUDE_DIR..?
set( INC_DIR "${INC_ROOT_DIR}/nomlib" )

set( NOMLIB_DEPS_DIR "${PROJECT_SOURCE_DIR}/third-party" )

# Relative path from project root to nomlib demonstration examples
set ( EXAMPLES_SRC_DIR "${PROJECT_SOURCE_DIR}/examples" )

# Resources path for engine examples
set( NOM_EXAMPLES_RESOURCES_DIR "${NOMLIB_RESOURCES_DIR}/examples" )

# Unit testing resources path
set( NOM_TESTS_RESOURCES_DIR "${NOMLIB_RESOURCES_DIR}/tests" )

# Installation layout directories
if( PLATFORM_OSX AND FRAMEWORK )
  set( NOMLIB_INCLUDE_INSTALL_DIR "${PROJECT_NAME}.framework/Headers" )
  set( NOMLIB_LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}" )
  set( NOMLIB_SHARE_INSTALL_DIR "${PROJECT_NAME}.framework/Resources" )
  set( NOMLIB_CMAKE_INSTALL_DIR "${PROJECT_NAME}.framework/Resources/CMake" )
  set( NOMLIB_RESOURCES_INSTALL_DIR "${PROJECT_NAME}.framework/Resources" )
else()
  set( NOMLIB_INCLUDE_INSTALL_DIR "include" )
  set( NOMLIB_LIB_INSTALL_DIR "lib" )
  set( NOMLIB_SHARE_INSTALL_DIR "share/${PROJECT_NAME}" )
  set( NOMLIB_CMAKE_INSTALL_DIR "lib/cmake/${PROJECT_NAME}" )
  set( NOMLIB_RESOURCES_INSTALL_DIR "share/${PROJECT_NAME}" )
endif()

# Additional flags to pass add_executable -- used when library examples are
# built
set ( EXECUTABLE_FLAGS "" )

# Files used with documentation generation
set ( PROJECT_DOXYGEN_DIR "${NOMLIB_RESOURCES_DIR}/doxygen" )

# CMAKE_SYSTEM_PREFIX_PATH is searched to find libraries when the find_package
# command is used.
message ( STATUS "Library Search Prefix: ${CMAKE_SYSTEM_PREFIX_PATH}" )

# Installation prefix path set for our project
message ( STATUS "Installation Prefix: ${CMAKE_INSTALL_PREFIX}" )

# Doxygen configuration
include ( "${PROJECT_SOURCE_DIR}/cmake/doxygen.cmake" )

# Third-party dependencies

if ( PLATFORM_WINDOWS )
  if(NOT DEFINED ENV{SDL2DIR})
    set(ENV{SDL2DIR} "${NOMLIB_DEPS_DIR}/windows/SDL2")
  endif(NOT DEFINED ENV{SDL2DIR})

  if(NOT DEFINED ENV{SDL2IMAGEDIR})
    set(ENV{SDL2IMAGEDIR} "${NOMLIB_DEPS_DIR}/windows/SDL2_image")
  endif(NOT DEFINED ENV{SDL2IMAGEDIR})

  if(NOT DEFINED ENV{SDL2TTFDIR})
    set(ENV{SDL2TTFDIR} "${NOMLIB_DEPS_DIR}/windows/SDL2_ttf")
  endif(NOT DEFINED ENV{SDL2TTFDIR})

  if(NOT DEFINED ENV{OPENALDIR})
    set(ENV{OPENALDIR} "${NOMLIB_DEPS_DIR}/windows/OpenAL")
  endif(NOT DEFINED ENV{OPENALDIR})

  if(NOT DEFINED ENV{LIBSNDFILEDIR})
    set(ENV{LIBSNDFILEDIR} "${NOMLIB_DEPS_DIR}/windows/libsndfile")
  endif(NOT DEFINED ENV{LIBSNDFILEDIR})

  if(NOT DEFINED ENV{LIBROCKETDIR})
    set(ENV{LIBROCKETDIR} "${NOMLIB_DEPS_DIR}/windows/libRocket")
  endif(NOT DEFINED ENV{LIBROCKETDIR})

  # Redistribution paths must be absolute, a directory and end with a trailing
  # slash character (these are fed to CMake's install command). Only filenames
  # with the extension of '.dll' and '.lib' (case-insensitive) will be copied.

  set( SDL2_REDIST_DIR "${NOMLIB_DEPS_DIR}/windows/SDL2/lib/${PLATFORM_ARCH}/" )
  set( SDL2_IMAGE_REDIST_DIR "${NOMLIB_DEPS_DIR}/windows/SDL2_image/lib/${PLATFORM_ARCH}/" )
  set( SDL2_TTF_REDIST_DIR "${NOMLIB_DEPS_DIR}/windows/SDL2_ttf/lib/${PLATFORM_ARCH}/" )

  # The CMake-distributed FindOpenAL.cmake doesn't recognize lib/x86, lib/x64,
  # so in order to avoid creating our own, we break from our norm and conform
  # to libs/Win32, libs/Win64
  if( PLATFORM_ARCH STREQUAL "x86" )
    set( OPENAL_REDIST_DIR "${NOMLIB_DEPS_DIR}/windows/OpenAL/libs/Win32/" )
  elseif( PLATFORM_ARCH STREQUAL "x64" )
    set( OPENAL_REDIST_DIR "${NOMLIB_DEPS_DIR}/windows/OpenAL/libs/Win64/" )
  endif( PLATFORM_ARCH STREQUAL "x86" )

  set( LIBSNDFILE_REDIST_DIR "${NOMLIB_DEPS_DIR}/windows/libsndfile/lib/${PLATFORM_ARCH}/" )

  # FreeType2
  set( LIBROCKET_EXT_REDIST_DIR "${NOMLIB_DEPS_DIR}/windows/libRocket/bin/${PLATFORM_ARCH}/" )

  set( LIBROCKET_REDIST_DIR "${NOMLIB_DEPS_DIR}/windows/libRocket/lib/${PLATFORM_ARCH}/" )

  set( MSVCPP_REDIST_DIR "${NOMLIB_DEPS_DIR}/windows/msvcpp2013/${PLATFORM_ARCH}/" )
endif ( PLATFORM_WINDOWS )

if ( PLATFORM_OSX )

  if(NOT DEFINED ENV{SDL2DIR})
    set(ENV{SDL2DIR} "${NOMLIB_DEPS_DIR}/osx")
  endif(NOT DEFINED ENV{SDL2DIR})

  if(NOT DEFINED ENV{SDL2IMAGEDIR})
    set(ENV{SDL2IMAGEDIR} "${NOMLIB_DEPS_DIR}/osx")
  endif(NOT DEFINED ENV{SDL2IMAGEDIR})

  if(NOT DEFINED ENV{SDL2TTFDIR})
    set(ENV{SDL2TTFDIR} "${NOMLIB_DEPS_DIR}/osx")
  endif(NOT DEFINED ENV{SDL2TTFDIR})

  if(NOT DEFINED ENV{OPENALDIR})
    # Use platform-distributed headers for OpenAL; should automatically use the
    # proper platform SDK version as configured by CMAKE_OSX_SYSROOT
    set(ENV{OPENALDIR} "/System/Library/Frameworks/OpenAL.framework")
  endif(NOT DEFINED ENV{OPENALDIR})

  if(NOT DEFINED ENV{LIBSNDFILEDIR})
    set(ENV{LIBSNDFILEDIR} "${NOMLIB_DEPS_DIR}/osx")
  endif(NOT DEFINED ENV{LIBSNDFILEDIR})

  if(NOT DEFINED ENV{LIBROCKETDIR})
    set(ENV{LIBROCKETDIR} "${NOMLIB_DEPS_DIR}/osx/librocket")
  endif(NOT DEFINED ENV{LIBROCKETDIR})

  # FIXME: This file needs its RPATH modified for redistribution; it is **not**
  # being used as the run-time library for executables yet. We are using brew's
  # version installed under /usr/local
  set(  LIBROCKET_EXT_REDIST_FILES
        "${NOMLIB_DEPS_DIR}/osx/librocket/bin/libfreetype.6.dylib" )
endif ( PLATFORM_OSX )

if ( PLATFORM_LINUX OR PLATFORM_POSIX )

  if(NOT DEFINED ENV{SDL2DIR})
    set(ENV{SDL2DIR} "${NOMLIB_DEPS_DIR}/linux/sdl2")
  endif(NOT DEFINED ENV{SDL2DIR})

  if(NOT DEFINED ENV{SDL2IMAGEDIR})
    set(ENV{SDL2IMAGEDIR} "${NOMLIB_DEPS_DIR}/linux/sdl2_image")
  endif(NOT DEFINED ENV{SDL2IMAGEDIR})

  if(NOT DEFINED ENV{SDL2TTFDIR})
    set(ENV{SDL2TTFDIR} "${NOMLIB_DEPS_DIR}/linux/sdl2_ttf")
  endif(NOT DEFINED ENV{SDL2TTFDIR})

  if(NOT DEFINED ENV{OPENALDIR})
    # Use platform-distributed headers for OpenAL
    #set(ENV{OPENALDIR} "${NOMLIB_DEPS_DIR}/linux/openal")
    # FIXME(JEFF): This is a temporary workaround until we have a proper distribution
    # build of OpenAL-Soft, sourced from `vendor/openal-soft.git`
    set(ENV{OPENALDIR} "/usr/include/AL")
  endif(NOT DEFINED ENV{OPENALDIR})

  if(NOT DEFINED ENV{LIBSNDFILEDIR})
    set(ENV{LIBSNDFILEDIR} "${NOMLIB_DEPS_DIR}/linux/libsndfile")
  endif(NOT DEFINED ENV{LIBSNDFILEDIR})

  if(NOT DEFINED ENV{LIBROCKETDIR})
    set(ENV{LIBROCKETDIR} "${NOMLIB_DEPS_DIR}/linux/libRocket")
  endif(NOT DEFINED ENV{LIBROCKETDIR})
endif( PLATFORM_LINUX OR PLATFORM_POSIX )

# List of our source code to be compiled
add_subdirectory( ${SRC_DIR} )

# =============================================================================
# Installation phase: unified install/package configuration
# =============================================================================

# --- Bundle third-party dependencies into framework bundles (OSX only) ------
if( PLATFORM_OSX AND FRAMEWORK )

  if( NOM_BUILD_CORE_UNIT )
    if( IS_DIRECTORY "${SDL2_LIBRARY}" )
      install(  DIRECTORY ${SDL2_LIBRARY}
                DESTINATION "${PROJECT_NAME}-core.framework/Frameworks"
                PATTERN ".*" EXCLUDE
      )
    else()
      install(  FILES ${SDL2_LIBRARY}
                DESTINATION "${PROJECT_NAME}-core.framework/Frameworks"
      )
    endif()
  endif( NOM_BUILD_CORE_UNIT )

  if( NOM_BUILD_SYSTEM_UNIT )
    foreach( _lib ${SDL2_IMAGE_LIBRARY} ${SDL2_TTF_LIBRARY} )
      if( IS_DIRECTORY "${_lib}" )
        install(  DIRECTORY ${_lib}
                  DESTINATION "${PROJECT_NAME}-system.framework/Frameworks"
                  PATTERN ".*" EXCLUDE
        )
      else()
        install(  FILES ${_lib}
                  DESTINATION "${PROJECT_NAME}-system.framework/Frameworks"
        )
      endif()
    endforeach()
  endif( NOM_BUILD_SYSTEM_UNIT )

  if( NOM_BUILD_AUDIO_UNIT )
    if( IS_DIRECTORY "${LIBSNDFILE_LIBRARY}" )
      install(  DIRECTORY ${LIBSNDFILE_LIBRARY}
                DESTINATION "${PROJECT_NAME}-audio.framework/Frameworks"
                PATTERN ".*" EXCLUDE
      )
    else()
      install(  FILES ${LIBSNDFILE_LIBRARY}
                DESTINATION "${PROJECT_NAME}-audio.framework/Frameworks"
      )
    endif()
  endif( NOM_BUILD_AUDIO_UNIT )

  if( NOM_BUILD_GUI_UNIT )
    install(  FILES ${LIBROCKET_CORE_LIBRARY} ${LIBROCKET_CONTROLS_LIBRARY}
              ${LIBROCKET_DEBUGGER_LIBRARY}
              DESTINATION "${PROJECT_NAME}-gui.framework/Libraries"
    )
  endif( NOM_BUILD_GUI_UNIT )

endif( PLATFORM_OSX AND FRAMEWORK )

# --- Header files -----------------------------------------------------------
if( PLATFORM_OSX AND FRAMEWORK )
  # Install under nomlib.framework/Headers/nomlib/... so consumers use
  # #include <nomlib/core.hpp> consistently with POSIX layout.
  install(  DIRECTORY ${INC_DIR}
            DESTINATION "${NOMLIB_INCLUDE_INSTALL_DIR}"
            COMPONENT Development
            PATTERN "." EXCLUDE
            PATTERN "*.in" EXCLUDE
  )
else()
  install(  DIRECTORY ${INC_DIR}
            DESTINATION "${NOMLIB_INCLUDE_INSTALL_DIR}"
            COMPONENT Development
            PATTERN "." EXCLUDE
            PATTERN "*.in" EXCLUDE
  )
endif()

# --- Resources (icon, shared support data) ----------------------------------
install(  FILES
          "${NOMLIB_RESOURCES_DIR}/nomlib.icns"
          DESTINATION "${NOMLIB_RESOURCES_INSTALL_DIR}"
          OPTIONAL
          COMPONENT Runtime
)

install(  DIRECTORY "${NOMLIB_SHARED_SUPPORT_DIR}/InputDevices"
          DESTINATION "${NOMLIB_RESOURCES_INSTALL_DIR}/SharedSupport"
          COMPONENT Runtime
          OPTIONAL
          PATTERN ".*" EXCLUDE
)

# --- Documentation and project info -----------------------------------------
if( PLATFORM_OSX AND FRAMEWORK )
  install(  FILES
            ${PROJECT_SOURCE_DIR}/README.md
            ${PROJECT_SOURCE_DIR}/LICENSE.md
            DESTINATION "${NOMLIB_SHARE_INSTALL_DIR}"
            COMPONENT Runtime
  )
else()
  install(  FILES
            ${PROJECT_SOURCE_DIR}/README.md
            ${PROJECT_SOURCE_DIR}/LICENSE.md
            DESTINATION "share/doc/${PROJECT_NAME}"
            COMPONENT Runtime
          )
endif()

if( NOM_INSTALL_GENERATED_DOCS )
  if( PLATFORM_OSX AND FRAMEWORK )
    install(  DIRECTORY ${PROJECT_BINARY_DIR}/docs/html
              DESTINATION "${NOMLIB_SHARE_INSTALL_DIR}/Documentation"
              COMPONENT Documentation
              PATTERN ".*" EXCLUDE
    )
  else()
    install(  DIRECTORY ${PROJECT_BINARY_DIR}/docs/html
              DESTINATION "share/doc/${PROJECT_NAME}"
              COMPONENT Documentation
              PATTERN ".*" EXCLUDE
    )
  endif()
endif()

# --- CMake package configuration (modern config-file package) ---------------
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/nomlib-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  "${CMAKE_TEMPLATE_PATH}/nomlib-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/nomlib-config.cmake"
  INSTALL_DESTINATION "${NOMLIB_CMAKE_INSTALL_DIR}"
  PATH_VARS
    NOMLIB_INCLUDE_INSTALL_DIR
    NOMLIB_LIB_INSTALL_DIR
    NOMLIB_SHARE_INSTALL_DIR
    NOMLIB_RESOURCES_INSTALL_DIR
)

install(  FILES
          "${CMAKE_CURRENT_BINARY_DIR}/nomlib-config.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/nomlib-config-version.cmake"
          DESTINATION "${NOMLIB_CMAKE_INSTALL_DIR}"
          COMPONENT Development
)

# Also install legacy find-module for backward compatibility.
# In FRAMEWORK layout the modern config already lives at
# nomlib.framework/Resources/CMake (NOMLIB_CMAKE_INSTALL_DIR), so we must not
# install the legacy wrapper to the same place (it would overwrite the modern
# config). Instead install it to the same share/nomlib/CMake location used by
# the POSIX layout.
if( PLATFORM_OSX AND FRAMEWORK )
  set( NOMLIB_LEGACY_CMAKE_INSTALL_DIR "share/${PROJECT_NAME}/CMake" )
else()
  set( NOMLIB_LEGACY_CMAKE_INSTALL_DIR "${NOMLIB_SHARE_INSTALL_DIR}/CMake" )
endif()

install(  DIRECTORY
          "${NOMLIB_RESOURCES_DIR}/CMake"
          DESTINATION "${NOMLIB_LEGACY_CMAKE_INSTALL_DIR}/.."
          COMPONENT Development
          PATTERN ".*" EXCLUDE
)
# Module-mode lookup (CMAKE_MODULE_PATH) searches for Find<Name>.cmake while
# the installed file is named nomlib-config.cmake (Config-mode style). Install
# a forwarding Findnomlib.cmake next to it so either lookup style works.
install(  FILES
          "${NOMLIB_RESOURCES_DIR}/CMake/nomlib-config.cmake"
          DESTINATION "${NOMLIB_LEGACY_CMAKE_INSTALL_DIR}"
          RENAME "Findnomlib.cmake"
          COMPONENT Development
)

if( PLATFORM_WINDOWS AND NOT FRAMEWORK )
  install(  DIRECTORY
            "${NOMLIB_RESOURCES_DIR}/CMake/"
            DESTINATION "CMake"
            COMPONENT Development
            PATTERN ".*" EXCLUDE
          )
endif()

# --- Export targets ----------------------------------------------------------
install(  EXPORT nomlib-targets
          FILE nomlib-targets.cmake
          NAMESPACE nomlib::
          DESTINATION "${NOMLIB_CMAKE_INSTALL_DIR}"
          COMPONENT Development
)

# nomlib examples configuration
if( EXAMPLES )
  add_subdirectory("examples")
endif( EXAMPLES )

# NOTE: we *MUST* enable testing from the top-level build script -- that's this
# file -- or else 'make test' breaks. (Dated 2014-04-05)
if( NOM_BUILD_TESTS )
  enable_testing()
  add_subdirectory("tests")
endif( NOM_BUILD_TESTS )

# Make a version file containing the current version from git
#
# Source: http://brianmilco.blogspot.com/2012/11/cmake-automatically-use-git-tags-as.html
#
# NOTE: This has been removed from the project due to wrong version output from
# `git describe` when using our git branching strategy -- also known as Vincent
# Driessen’s "Git branching model".
#
# In short, the problem is introduced during the "Finishing a release branch"
# section of the model, when we create our release tag. The algorithm
# `git describe` uses to determine the "nearest" tag is, I believe, the root
# cause of the problem. It's especially worth mentioning that the problem only
# occurs on non-HEAD branches -- i.e.: development branches, such as dev or
# feature/<name>.
#
# Full problem description: http://www.xerxesb.com/2010/git-describe-and-the-tale-of-the-wrong-commits/
#
# include( GetGitRevisionDescription )
# git_describe( GIT_REVISION )
# string( REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+(.*)" "\\1" VERSION_SHA1 "${GIT_REVISION}" )

# Auto-generate a build revision file containing the last recorded git revision
# number (top of branch).
include(GetGitRevisionNumber)
# git_rev_number(GIT_REVISION) # Full SHA
git_rev_number(GIT_REVISION "--short")
set(GIT_REVISION "${GIT_REVISION}")

# TODO(JEFF): This needs to be tested!
#configure_file( ${INCLUDE_DIR}/version.hpp.in ${INCLUDE_DIR}/version.hpp )

configure_file( ${SRC_DIR}/revision.cpp.in ${SRC_DIR}/revision.cpp )

# Uninstall target support; 'make uninstall'
configure_file  (
                  "${CMAKE_TEMPLATE_PATH}/uninstall.cmake.in"
                  "${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake"
                  IMMEDIATE @ONLY
                )

add_custom_target ( uninstall COMMAND ${CMAKE_COMMAND} -P
                    ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake
                  )

# ---------------------------------------------------------------------------
# Install/package verification targets
#
# Run the POSIX-layout end-to-end verification (configure, build, install,
# consume from external CMake project, run the binary, exercise uninstall).
add_custom_target( nomlib-verify
  COMMAND
    "${PROJECT_SOURCE_DIR}/cmake/scripts/verify_install.sh"
  COMMENT "Running nomlib install verification (POSIX layout)..."
  USES_TERMINAL
)

# Run the FRAMEWORK-layout end-to-end verification (OS X only).
if( PLATFORM_OSX )
  add_custom_target( nomlib-verify-framework
    COMMAND
      "${PROJECT_SOURCE_DIR}/cmake/scripts/verify_install.sh" --framework
    COMMENT "Running nomlib install verification (FRAMEWORK layout)..."
    USES_TERMINAL
  )
endif()

# TODO: Windows platform support
if ( NOT PLATFORM_WINDOWS )
  # CPack configuration
  include ( "${PROJECT_SOURCE_DIR}/cmake/CPackConfig.cmake" )

  include ( InstallRequiredSystemLibraries )

  # Platform specific generator presets
  if ( PLATFORM_OSX )
    set ( CPACK_GENERATOR "DragNDrop" )
  elseif ( PLATFORM_LINUX )
    set ( CPACK_GENERATOR "DEB" )
  elseif ( PLATFORM_UNKNOWN )
    set ( CPACK_GENERATOR "ZIP" )
  endif ( PLATFORM_OSX )

  include ( CPack )
endif ( NOT PLATFORM_WINDOWS )
