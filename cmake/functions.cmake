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
# public_include_dirs is an optional list of include directories that should
# be exposed as PUBLIC (transitive) include dependencies to consumers of this
# library, e.g. SDL2_INCLUDE_DIR.
#
# TODO: Future expansion of this macro should strongly consider refactoring with
# the use of the CMakeParseArguments module.
# http://www.cmake.org/cmake/help/v3.0/module/CMakeParseArguments.html
function(nom_add_library target lib_type source headers external_deps public_include_dirs)
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
  add_library( nomlib::${target} ALIAS ${target} )

  set_target_properties( ${target} PROPERTIES
    VERSION ${LIB_VERSION}
    SOVERSION ${LIB_SOVERSION}
    DEBUG_POSTFIX "-d"
    EXPORT_NAME ${target}
  )

  target_link_libraries( ${target} ${external_deps} )

  # Native CMake RPATH configuration -- replaces manual install_name_tool usage
  if( BUILD_SHARED_LIBS )
    set_target_properties( ${target} PROPERTIES
      MACOSX_RPATH TRUE
      SKIP_BUILD_RPATH FALSE
      BUILD_WITH_INSTALL_RPATH FALSE
      INSTALL_RPATH_USE_LINK_PATH TRUE
    )

    if( PLATFORM_OSX )
      if( FRAMEWORK )
        set_target_properties( ${target} PROPERTIES
          INSTALL_RPATH "@loader_path/../Frameworks;@loader_path/../../.."
          BUILD_RPATH "@loader_path/../Frameworks;@loader_path/../../.."
        )
      else()
        set_target_properties( ${target} PROPERTIES
          INSTALL_RPATH "@loader_path/../lib;${CMAKE_INSTALL_PREFIX}/lib"
          BUILD_RPATH "${CMAKE_BINARY_DIR}/lib"
        )
      endif()
    elseif( PLATFORM_LINUX )
      set_target_properties( ${target} PROPERTIES
        INSTALL_RPATH "\$ORIGIN/../lib;${CMAKE_INSTALL_PREFIX}/lib"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/lib"
      )
    endif()
  endif()

  # Expose include directories to consumers of this target
  target_include_directories( ${target}
    PUBLIC
      $<BUILD_INTERFACE:${INC_ROOT_DIR}>
      $<INSTALL_INTERFACE:include>
      ${public_include_dirs}
  )

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
    )
  endif( PLATFORM_OSX AND FRAMEWORK )

  # Install target and export to nomlib-targets
  if( PLATFORM_OSX AND FRAMEWORK )
    install(  TARGETS ${target}
              EXPORT nomlib-targets
              LIBRARY DESTINATION lib COMPONENT Runtime
              ARCHIVE DESTINATION lib COMPONENT Development
              FRAMEWORK DESTINATION ${CMAKE_INSTALL_PREFIX} COMPONENT Runtime
              PUBLIC_HEADER DESTINATION include COMPONENT Development
    )
  else()
    install(  TARGETS ${target}
              EXPORT nomlib-targets
              LIBRARY DESTINATION lib COMPONENT Runtime
              ARCHIVE DESTINATION lib COMPONENT Development
              RUNTIME DESTINATION bin COMPONENT Runtime
              INCLUDES DESTINATION include
    )
  endif()

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
