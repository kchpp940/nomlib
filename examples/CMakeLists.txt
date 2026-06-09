# TODO: Remove checks for the necessary build units in order to compile
# examples. In hindsight, this check is worse than no check at all, because we
# have no warning / error when the build unit check is FALSE -- this can lead
# to confusion.
#
# TODO: consider adding a custom POST_BUILD command here so that we do not have
# to manually execute the 'install' target in order to get the example's
# Resources, etc. to copy.
#
#   OS X with cmake generated Xcode project files:
#
# xcodebuild -target install
#
#   MSBuild with Visual Studio 2012 project files:
#
# msbuild /t:build INSTALL.vcxproj
#
#   See also,
#
# http://stackoverflow.com/questions/13422202/adding-distinct-xcode-resources-for-multiple-executables-using-cmake
# http://stackoverflow.com/questions/6077810/how-to-correctly-add-resources-to-a-project?rq=1

# -----------------------------------------------------------------------------
# nomlib examples: supports both in-tree build and out-of-tree (installed) use
# -----------------------------------------------------------------------------
#
# When built as part of the nomlib source tree, examples link against the
# locally-built nomlib-* targets. When built standalone (e.g. from an
# installed copy), examples use find_package(nomlib) to locate the
# installed libraries, headers, and resource directory.

cmake_minimum_required(VERSION 3.14)

# Detect whether we are building inside the nomlib source tree *before*
# calling project(), which would re-root PROJECT_SOURCE_DIR.
if( TARGET nomlib-core OR TARGET nomlib::nomlib-core )
  set( NOMLIB_BUILDING_IN_TREE TRUE )
  set( NOMLIB_PARENT_SOURCE_DIR "${PROJECT_SOURCE_DIR}" )
else()
  set( NOMLIB_BUILDING_IN_TREE FALSE )
  set( NOMLIB_PARENT_SOURCE_DIR "" )
endif()

project(nomlib_examples CXX)

if( NOT NOMLIB_BUILDING_IN_TREE )
  # Standalone build -- locate installed nomlib package using the modern
  # config-file package (nomlib_DIR or CMAKE_PREFIX_PATH).
  find_package( nomlib CONFIG REQUIRED COMPONENTS core math file )

  # Auto-detect which optional units were installed by inspecting the exported
  # nomlib::* targets.
  set( NOM_BUILD_AUDIO_UNIT FALSE )
  if( TARGET nomlib::nomlib-audio )
    set( NOM_BUILD_AUDIO_UNIT TRUE )
  endif()
  set( NOM_BUILD_GUI_UNIT FALSE )
  if( TARGET nomlib::nomlib-gui )
    set( NOM_BUILD_GUI_UNIT TRUE )
  endif()
  set( NOM_BUILD_SERIALIZERS_UNIT FALSE )
  if( TARGET nomlib::nomlib-serializers )
    set( NOM_BUILD_SERIALIZERS_UNIT TRUE )
  endif()
  set( NOM_BUILD_SYSTEM_UNIT FALSE )
  if( TARGET nomlib::nomlib-system )
    set( NOM_BUILD_SYSTEM_UNIT TRUE )
  endif()
  set( NOM_BUILD_GRAPHICS_UNIT FALSE )
  if( TARGET nomlib::nomlib-graphics )
    set( NOM_BUILD_GRAPHICS_UNIT TRUE )
  endif()
  set( NOM_BUILD_ACTIONS_UNIT FALSE )
  if( TARGET nomlib::nomlib-actions )
    set( NOM_BUILD_ACTIONS_UNIT TRUE )
  endif()

  # Re-expose the same variables that the in-tree build provides
  set( INC_ROOT_DIR "${NOMLIB_INCLUDE_DIR}" )
  set( NOMLIB_RESOURCES_DIR "${NOMLIB_RESOURCES_DIR}" )
  set( NOM_EXAMPLES_RESOURCES_DIR "${NOMLIB_RESOURCES_DIR}/examples" )
  set( NOMLIB_SHARED_SUPPORT_DIR "${NOMLIB_RESOURCES_DIR}/SharedSupport" )

  include_directories( "${NOMLIB_INCLUDE_DIR}" )

  # Alias imported targets to the names used by in-tree code so the rest of
  # this file is identical for both build modes.
  foreach( _mod core math file system graphics gui actions audio serializers ptree )
    if( NOT TARGET nomlib-${_mod} AND TARGET nomlib::nomlib-${_mod} )
      add_library(nomlib-${_mod} ALIAS nomlib::nomlib-${_mod})
    endif()
  endforeach()

  # TCLAP is a header-only third-party dep used by audio/ and renderfont/
  # examples. When building standalone we try to find it on the system; if
  # unavailable, those examples are simply skipped.
  find_path( TCLAP_INCLUDE_DIR NAMES tclap/CmdLine.h
             PATHS /usr/local/include /opt/homebrew/include /usr/include )
  if( TCLAP_INCLUDE_DIR )
    include_directories( "${TCLAP_INCLUDE_DIR}" )
    message( STATUS "TCLAP headers found: ${TCLAP_INCLUDE_DIR}" )
  else()
    message( STATUS "TCLAP headers not found -- skipping audio/renderfont examples" )
    set( NOM_BUILD_AUDIO_EXAMPLE OFF )
    set( NOM_BUILD_FONTS_EXAMPLE OFF )
  endif()

  # When building standalone, disable examples that require modules not
  # installed with the found nomlib package.
  if( NOT NOM_BUILD_SYSTEM_UNIT OR NOT NOM_BUILD_GRAPHICS_UNIT )
    set( NOM_BUILD_EVENTS_EXAMPLE OFF )
    set( NOM_BUILD_JOYSTICK_EXAMPLE OFF )
    set( NOM_BUILD_GAME_CONTROLLER_EXAMPLE OFF )
    set( NOM_BUILD_FONTS_EXAMPLE OFF )
    set( NOM_BUILD_MOUSE_CURSORS_EXAMPLE OFF )
    set( NOM_BUILD_DEVICE_INFO_EXAMPLE OFF )
  endif()
  if( NOT NOM_BUILD_GUI_UNIT OR NOT NOM_BUILD_ACTIONS_UNIT )
    set( NOM_BUILD_APP_EXAMPLE OFF )
  endif()
  if( NOT NOM_BUILD_AUDIO_UNIT OR NOT NOM_BUILD_SERIALIZERS_UNIT )
    set( NOM_BUILD_AUDIO_EXAMPLE OFF )
  endif()
endif()

# Set default example enable flags (only if not already set by the standalone
# build auto-detection logic above, or by the user via -D flags).
if( NOT DEFINED NOM_BUILD_APP_EXAMPLE )
  set( NOM_BUILD_APP_EXAMPLE ON )
endif()
if( NOT DEFINED NOM_BUILD_AUDIO_EXAMPLE )
  set( NOM_BUILD_AUDIO_EXAMPLE ON )
endif()
if( NOT DEFINED NOM_BUILD_DEVICE_INFO_EXAMPLE )
  set( NOM_BUILD_DEVICE_INFO_EXAMPLE ON )
endif()
if( NOT DEFINED NOM_BUILD_EVENTS_EXAMPLE )
  set( NOM_BUILD_EVENTS_EXAMPLE ON )
endif()
if( NOT DEFINED NOM_BUILD_JOYSTICK_EXAMPLE )
  set( NOM_BUILD_JOYSTICK_EXAMPLE ON )
endif()
if( NOT DEFINED NOM_BUILD_GAME_CONTROLLER_EXAMPLE )
  set( NOM_BUILD_GAME_CONTROLLER_EXAMPLE ON )
endif()
if( NOT DEFINED NOM_BUILD_FONTS_EXAMPLE )
  set( NOM_BUILD_FONTS_EXAMPLE ON )
endif()
if( NOT DEFINED NOM_BUILD_MOUSE_CURSORS_EXAMPLE )
  set( NOM_BUILD_MOUSE_CURSORS_EXAMPLE ON )
endif()
if( NOT DEFINED NOM_BUILD_MACROS_EXAMPLE )
  set( NOM_BUILD_MACROS_EXAMPLE ON )
endif()

if( EXISTS "${CMAKE_CURRENT_LIST_DIR}/local_env.cmake" )
  include( "${CMAKE_CURRENT_LIST_DIR}/local_env.cmake" )
endif()

# Use common build output directories for MSVCPP && Xcode project files.
#
# IMPORTANT: Debug and Release build targets **must** be kept in separate build
# trees!
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/examples/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/examples/Release")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/examples/Debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/examples/Release")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/examples/Debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/examples/Release")

if(DEBUG)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
else() # Release builds
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})
endif()

# This path is used for the local installation of required dependencies for
# running examples, such as dependent resource files. Additionally, when
# building on Windows, this will be the path that dependent DLLs are copied to.
set( EXAMPLES_INSTALL_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" )

if( NOMLIB_BUILDING_IN_TREE )
  # Project headers files inclusions; our header files namespace, 'nomlib'
  include_directories( "${INC_ROOT_DIR}" )

  # Third-party dependencies

  # Absolute path to project's third-party 'common' directory
  set( NOM_THIRD_PARTY_COMMON_DIR "${NOMLIB_PARENT_SOURCE_DIR}/third-party/common" )

  # nomlib-core external deps
  find_package( SDL2 REQUIRED )
  if( SDL2_FOUND )
    include_directories( ${SDL2_INCLUDE_DIR} )
  endif( SDL2_FOUND )

  # nomlib-audio external deps
  if ( NOM_BUILD_AUDIO_UNIT )
    # nomlib-audio external deps; OpenAL-Soft or Apple's OpenAL
    find_package( OpenAL REQUIRED )
    if( OPENAL_FOUND )
      include_directories( ${OPENAL_INCLUDE_DIR} )
      message ( STATUS "Found OpenAL headers: ${OPENAL_INCLUDE_DIR}." )
    else ( NOT OPENAL_FOUND )
      message ( FATAL_ERROR "Missing OpenAL headers at ${OPENAL_INCLUDE_DIR}." )
    endif ( OPENAL_FOUND )
  endif()

  # nomlib-gui external deps
  if( NOM_BUILD_GUI_UNIT )
    find_package( LibRocket REQUIRED )
    if( LIBROCKET_FOUND )
      # Add development header files
      include_directories( ${LIBROCKET_INCLUDE_DIRS} )
    endif( LIBROCKET_FOUND )
  endif( NOM_BUILD_GUI_UNIT )

  # third-party, headers-only dependency TCLAP
  set( TCLAP_INCLUDE_DIR "${NOM_THIRD_PARTY_COMMON_DIR}/tclap" )

  if( NOT EXISTS ${TCLAP_INCLUDE_DIR} )
    message(  FATAL_ERROR
              "Could not find header file ${TCLAP_INCLUDE_DIR} for TCLAP; "
              "see nomlib's third-party/README.md for help obtaining this "
              "required dependency."
    )
  else( EXISTS ${TCLAP_INCLUDE_DIR} )
    message( STATUS "TCLAP headers found: ${TCLAP_INCLUDE_DIR}" )

    # Header file inclusion; third-party -- TCLAP
    include_directories("${NOM_THIRD_PARTY_COMMON_DIR}")
  endif( NOT EXISTS ${TCLAP_INCLUDE_DIR} )
endif()

if( PLATFORM_WINDOWS )
  # Build as console app -- shows cmd.exe window for logging -- only when
  # debugging is toggled on
  if ( NOT DEBUG )
    set ( EXECUTABLE_FLAGS "WIN32" ) # Implies WIN_MAIN
  else ( DEBUG )
    # MSVCPP should default to Console if we set EXECUTABLE_FLAGS empty
    set ( EXECUTABLE_FLAGS "" )
  endif ( NOT DEBUG )
endif ( PLATFORM_WINDOWS )

if( NOM_BUILD_APP_EXAMPLE )
  add_executable ( app "${EXECUTABLE_FLAGS}" "app/app.cpp" )

  set( APP_DEPS nomlib-gui nomlib-graphics nomlib-actions )

  if( PLATFORM_WINDOWS )
    # We need to link to SDL2main library on Windows
    list( APPEND APP_DEPS ${SDL2MAIN_LIBRARY} )
  endif( PLATFORM_WINDOWS )

  target_link_libraries( app ${APP_DEPS} )

  # Search paths file to use in finding the resources path
  install (
            FILES
            "${NOM_EXAMPLES_RESOURCES_DIR}/app.json"
            DESTINATION
            "${EXAMPLES_INSTALL_DIR}"
          )
endif( NOM_BUILD_APP_EXAMPLE )

if(NOM_BUILD_AUDIO_EXAMPLE)
  set(EX_SRC "audio/audio.cpp")
  add_executable(audio "${EXECUTABLE_FLAGS}" ${EX_SRC})

  set(AUDIO_DEPS nomlib-math nomlib-system nomlib-audio nomlib-actions
      nomlib-serializers)

  if(PLATFORM_WINDOWS)
    # We need to link to SDL2main library on Windows
    list(APPEND AUDIO_DEPS ${SDL2MAIN_LIBRARY})
  endif(PLATFORM_WINDOWS)

  target_link_libraries(audio ${AUDIO_DEPS})

  # Copy resource files for the example
  install_resource_file("${NOM_EXAMPLES_RESOURCES_DIR}/audio.json"
                        "${EXAMPLES_INSTALL_DIR}")

  endif(NOM_BUILD_AUDIO_EXAMPLE)

if(NOM_BUILD_EVENTS_EXAMPLE)
  set(APP_NAME events)
  set(EX_SRC "${APP_NAME}.cpp")
  add_executable(${APP_NAME} "${EXECUTABLE_FLAGS}" ${EX_SRC})

  set(APP_DEPS nomlib-system nomlib-graphics)

  if(PLATFORM_WINDOWS)
    # We need to link to SDL2main library on Windows
    list(APPEND APP_DEPS ${SDL2MAIN_LIBRARY})
  endif(PLATFORM_WINDOWS)

  target_link_libraries(${APP_NAME} ${APP_DEPS})
endif(NOM_BUILD_EVENTS_EXAMPLE)

if( NOM_BUILD_JOYSTICK_EXAMPLE )

  set( APP_NAME "joystick_events" )
  add_executable( ${APP_NAME} "${EXECUTABLE_FLAGS}" "${APP_NAME}.cpp" )

  set( APP_DEPS nomlib-system nomlib-graphics )

  if( PLATFORM_WINDOWS )
    # We need to link to SDL2main library on Windows
    list( APPEND APP_DEPS ${SDL2MAIN_LIBRARY} )
  endif( PLATFORM_WINDOWS )

  target_link_libraries( ${APP_NAME} ${APP_DEPS} )
endif( NOM_BUILD_JOYSTICK_EXAMPLE )

if( NOM_BUILD_GAME_CONTROLLER_EXAMPLE )

  set( APP_NAME "gamecontroller_events" )
  add_executable( ${APP_NAME} "${EXECUTABLE_FLAGS}" "${APP_NAME}.cpp" )

  set( APP_DEPS nomlib-system nomlib-graphics )

  if( PLATFORM_WINDOWS )
    # We need to link to SDL2main library on Windows
    list( APPEND APP_DEPS ${SDL2MAIN_LIBRARY} )
  endif( PLATFORM_WINDOWS )

  target_link_libraries( ${APP_NAME} ${APP_DEPS} )

  install(  FILES "${NOMLIB_SHARED_SUPPORT_DIR}/InputDevices.json"
            DESTINATION "${EXAMPLES_INSTALL_DIR}" )
endif( NOM_BUILD_GAME_CONTROLLER_EXAMPLE )

if( NOM_BUILD_DEVICE_INFO_EXAMPLE )

  add_executable ( device_info "${EXECUTABLE_FLAGS}" "device_info.cpp" )
  if ( NOM_BUILD_AUDIO_UNIT )
    set( DEVICE_INFO_DEPS nomlib-graphics nomlib-audio )
  elseif ( NOT NOM_BUILD_AUDIO_UNIT )
    set( DEVICE_INFO_DEPS nomlib-graphics )
  endif ( NOM_BUILD_AUDIO_UNIT )

  # Additional third-party dependency requirements
  find_package( SDL2_image REQUIRED )
  if( SDL2_IMAGE_FOUND )
    # Add development header files; SDL_image.h
    include_directories( ${SDL2_IMAGE_INCLUDE_DIR} )
    list( APPEND DEVICE_INFO_DEPS ${SDL2_IMAGE_LIBRARY} )
  endif( SDL2_IMAGE_FOUND )

  find_package( SDL2_ttf REQUIRED )
  if( SDL2_TTF_FOUND )
    # Add development header files; SDL_ttf.h
    include_directories( ${SDL2_TTF_INCLUDE_DIR} )
    list( APPEND DEVICE_INFO_DEPS ${SDL2_TTF_LIBRARY} )
  endif( SDL2_TTF_FOUND )

  find_package( OpenGL REQUIRED )
  if( OPENGL_FOUND )
    # Add development header files; gl.h
    include_directories( ${OPENGL_INCLUDE_DIR} )
    list( APPEND DEVICE_INFO_DEPS ${OPENGL_LIBRARY} )
  endif( OPENGL_FOUND )

  find_package( LibRocket REQUIRED )
  if( LIBROCKET_FOUND )
    # Add development header files
    include_directories( ${LIBROCKET_INCLUDE_DIRS} )
    list( APPEND DEVICE_INFO_DEPS ${LIBROCKET_LIBRARIES} )
  endif( LIBROCKET_FOUND )

  if( PLATFORM_WINDOWS )
    # We need to link to SDL2main library on Windows
    list( APPEND DEVICE_INFO_DEPS ${SDL2MAIN_LIBRARY} )
  endif( PLATFORM_WINDOWS )

  target_link_libraries( device_info ${DEVICE_INFO_DEPS} )

endif( NOM_BUILD_DEVICE_INFO_EXAMPLE )

if( NOM_BUILD_FONTS_EXAMPLE )
  add_executable ( renderfont "${EXECUTABLE_FLAGS}" "renderfont.cpp" )

  set( FONTS_DEPS nomlib-graphics )

  if( PLATFORM_WINDOWS )
    # We need to link to SDL2main library on Windows
    list( APPEND FONTS_DEPS ${SDL2MAIN_LIBRARY} )
  endif( PLATFORM_WINDOWS )

  target_link_libraries( renderfont ${FONTS_DEPS} )

endif( NOM_BUILD_FONTS_EXAMPLE )

if( NOM_BUILD_MOUSE_CURSORS_EXAMPLE )
  add_executable( cursors "${EXECUTABLE_FLAGS}" "cursors.cpp" )

  set( CURSORS_DEPS nomlib-graphics )

  if( PLATFORM_WINDOWS )
    # We need to link to SDL2main library on Windows
    list( APPEND CURSORS_DEPS ${SDL2MAIN_LIBRARY} )
  endif( PLATFORM_WINDOWS )

  target_link_libraries( cursors ${CURSORS_DEPS} )

  install(  FILES "${NOMLIB_RESOURCES_DIR}/icon.png"
            DESTINATION "${EXAMPLES_INSTALL_DIR}/Resources" )
endif( NOM_BUILD_MOUSE_CURSORS_EXAMPLE )

if( NOM_BUILD_MACROS_EXAMPLE )
  add_executable ( macros "${EXECUTABLE_FLAGS}" "macros.cpp" )

  set( MACROS_DEPS nomlib-core )

  if( PLATFORM_WINDOWS )
    # We need to link to SDL2main library on Windows
    list( APPEND MACROS_DEPS ${SDL2MAIN_LIBRARY} )
  endif( PLATFORM_WINDOWS )

  target_link_libraries( macros ${MACROS_DEPS} )
endif( NOM_BUILD_MACROS_EXAMPLE )

# Install library dependencies into binary output directory
if( PLATFORM_WINDOWS )
  install(  DIRECTORY
            ${SDL2_REDIST_DIR}
            ${SDL2_IMAGE_REDIST_DIR}
            ${SDL2_TTF_REDIST_DIR}
            ${OPENAL_REDIST_DIR}
            ${LIBSNDFILE_REDIST_DIR}
            ${MSVCPP_REDIST_DIR}
            ${GLEW_REDIST_DIR}
            ${LIBROCKET_REDIST_DIR}
            ${LIBROCKET_EXT_REDIST_DIR}
            DESTINATION
            ${EXAMPLES_INSTALL_DIR}
            OPTIONAL # In case end-user overrides third-party dependencies
            FILES_MATCHING PATTERN "*.dll"
  )

endif ( PLATFORM_WINDOWS )

if ( PLATFORM_LINUX )
  install_resource_dir( "${NOM_EXAMPLES_RESOURCES_DIR}"
                        "${EXAMPLES_INSTALL_DIR}/Resources")
  # FIXME(JEFF): The app example requires the tests resources path
  install_resource_dir( "${NOM_TESTS_RESOURCES_DIR}"
                        "${EXAMPLES_INSTALL_DIR}/Resources")
endif ( PLATFORM_LINUX )
