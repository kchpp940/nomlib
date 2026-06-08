# cmake/macros.cmake:jeff
#
# Helper macros for CMake build scripts
#

# Add runtime search path to our application bundle so that we can find its
# dependencies at launch.
macro ( add_rpath target rpath binary_path )

  foreach ( path ${rpath} )

    if ( CMAKE_VERBOSE_MAKEFILE )
      set ( COMMENT_TEXT "\nAdding runtime search path: \n\n\t${path}\n\nto ${binary_path}\n\n" )
    endif ( CMAKE_VERBOSE_MAKEFILE )

    add_custom_command  ( TARGET ${target}
                          COMMAND ${CMAKE_INSTALL_NAME_TOOL}
                          -add_rpath "${path}" "${binary_path}"
                          COMMENT ${COMMENT_TEXT}
                        )

  endforeach ( path ${rpath} )

endmacro ( add_rpath target rpath binary_path )

# Modify runtime search path for a library or application
macro ( change_rpath old_rpath new_rpath binary_path )

  if ( CMAKE_VERBOSE_MAKEFILE )
    set ( COMMENT_TEXT "\nModifying runtime search path for ${binary_path}: \n\n\t${old_rpath}\n\nto ${new_rpath}\n\n" )
  endif ( CMAKE_VERBOSE_MAKEFILE )

  add_custom_command  ( TARGET ${PROJECT_NAME}
                        COMMAND ${CMAKE_INSTALL_NAME_TOOL}
                        -change "${old_rpath}" "${new_rpath}" "${binary_path}"
                        COMMENT ${COMMENT_TEXT}
                      )

endmacro ( change_rpath rpath binary_path )

# Change the install name path of a library
macro ( install_name_rpath rpath binary_path )

  if ( CMAKE_VERBOSE_MAKEFILE )
    set ( COMMENT_TEXT "\nModifying install name path for ${binary_path}: \n\n\t${rpath}\n\n" )
  endif ( CMAKE_VERBOSE_MAKEFILE )

  add_custom_command  ( TARGET ${PROJECT_NAME}
                        COMMAND ${CMAKE_INSTALL_NAME_TOOL}
                        -id "${rpath}" "${binary_path}"
                        COMMENT ${COMMENT_TEXT}
                      )

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

# ============================================================================
# nom_validate_build_options()
#
# Centralized validation of CMake build option dependencies.
#
# Prerequisites:
#   - nom_probe_third_party() must have been called first so that the
#     NOM_HAVE_OPENAL / NOM_HAVE_LIBSNDFILE / NOM_HAVE_LIBROCKET
#     normalized booleans are defined.
#
# Design rules:
#   * Only STRONG (hard) dependencies produce FATAL_ERROR.
#   * Optional / conditional dependencies are NOT enforced — the source
#     trees already guard them with if(NOM_BUILD_*_UNIT) blocks, so turning
#     off graphics/audio while actions is ON is a valid "minimal build"
#     configuration (only the base actions are compiled).
#   * External-library availability is checked via the NOM_HAVE_*
#     project-owned variables, never directly via *_FOUND from Find
#     modules (whose naming is inconsistent across modules).
# ============================================================================
macro(nom_validate_build_options)

  set(_NOM_ERRORS "")

  # -------------------------------------------------------------------------
  # 1. NOM_BUILD_ACTIONS_UNIT — strong deps only (CORE / MATH / SYSTEM)
  #    GRAPHICS and AUDIO are optional: their action subsets are guarded by
  #    if(NOM_BUILD_GRAPHICS_UNIT) / if(NOM_BUILD_AUDIO_UNIT) in
  #    src/actions/CMakeLists.txt and may be legitimately trimmed.
  # -------------------------------------------------------------------------
  if(NOM_BUILD_ACTIONS_UNIT)
    foreach(_req CORE MATH SYSTEM)
      if(NOT NOM_BUILD_${_req}_UNIT)
        string(APPEND _NOM_ERRORS
          "  [ERROR] NOM_BUILD_ACTIONS_UNIT requires NOM_BUILD_${_req}_UNIT=ON.\n")
      endif()
    endforeach()
  endif()

  # -------------------------------------------------------------------------
  # 2. NOM_BUILD_GRAPHICS_UNIT — strong deps
  # -------------------------------------------------------------------------
  if(NOM_BUILD_GRAPHICS_UNIT)
    foreach(_req CORE MATH FILE SERIALIZERS SYSTEM)
      if(NOT NOM_BUILD_${_req}_UNIT)
        string(APPEND _NOM_ERRORS
          "  [ERROR] NOM_BUILD_GRAPHICS_UNIT requires NOM_BUILD_${_req}_UNIT=ON.\n")
      endif()
    endforeach()
  endif()

  # -------------------------------------------------------------------------
  # 3. NOM_BUILD_AUDIO_UNIT — strong module deps + external-library deps
  #    External-library check uses the normalized NOM_HAVE_* booleans set
  #    by nom_probe_third_party().
  # -------------------------------------------------------------------------
  if(NOM_BUILD_AUDIO_UNIT)

    foreach(_req CORE MATH SYSTEM)
      if(NOT NOM_BUILD_${_req}_UNIT)
        string(APPEND _NOM_ERRORS
          "  [ERROR] NOM_BUILD_AUDIO_UNIT requires NOM_BUILD_${_req}_UNIT=ON.\n")
      endif()
    endforeach()

    if(NOT NOM_HAVE_OPENAL)
      string(APPEND _NOM_ERRORS
        "  [ERROR] NOM_BUILD_AUDIO_UNIT requires OpenAL, but it was NOT found.\n"
        "          Install OpenAL/OpenAL-Soft, set OPENALDIR/OPENAL_ROOT,\n"
        "          or disable NOM_BUILD_AUDIO_UNIT.\n")
    endif()

    if(NOT NOM_HAVE_LIBSNDFILE)
      string(APPEND _NOM_ERRORS
        "  [ERROR] NOM_BUILD_AUDIO_UNIT requires libsndfile, but it was NOT found.\n"
        "          Install libsndfile, set LIBSNDFILEDIR/LIBSNDFILE_ROOT,\n"
        "          or disable NOM_BUILD_AUDIO_UNIT.\n")
    endif()

  endif()

  # -------------------------------------------------------------------------
  # 4. NOM_BUILD_GUI_UNIT — strong module dep + external-library dep
  # -------------------------------------------------------------------------
  if(NOM_BUILD_GUI_UNIT)

    if(NOT NOM_BUILD_GRAPHICS_UNIT)
      string(APPEND _NOM_ERRORS
        "  [ERROR] NOM_BUILD_GUI_UNIT requires NOM_BUILD_GRAPHICS_UNIT=ON.\n"
        "          Enable NOM_BUILD_GRAPHICS_UNIT or disable NOM_BUILD_GUI_UNIT.\n")
    endif()

    if(NOT NOM_HAVE_LIBROCKET)
      string(APPEND _NOM_ERRORS
        "  [ERROR] NOM_BUILD_GUI_UNIT requires libRocket, but it was NOT found.\n"
        "          Install libRocket, set LIBROCKETDIR/LIBROCKET_ROOT,\n"
        "          or disable NOM_BUILD_GUI_UNIT.\n")
    endif()

  endif()

  # -------------------------------------------------------------------------
  # 5. EXAMPLES — strong deps only
  #    Examples that depend on optional modules (app→gui/actions,
  #    audio→audio/actions, etc.) are individually guarded by
  #    if(NOM_BUILD_*_EXAMPLE) in examples/CMakeLists.txt and are
  #    automatically skipped when their deps are off. Only the modules
  #    required by EVERY example are enforced here.
  # -------------------------------------------------------------------------
  if(EXAMPLES)
    foreach(_req CORE SYSTEM GRAPHICS)
      if(NOT NOM_BUILD_${_req}_UNIT)
        string(APPEND _NOM_ERRORS
          "  [ERROR] EXAMPLES requires NOM_BUILD_${_req}_UNIT=ON.\n")
      endif()
    endforeach()
  endif()

  # -------------------------------------------------------------------------
  # 6. NOM_BUILD_TESTS — strong deps
  # -------------------------------------------------------------------------
  if(NOM_BUILD_TESTS)

    if(NOT NOM_BUILD_CORE_UNIT)
      string(APPEND _NOM_ERRORS
        "  [ERROR] NOM_BUILD_TESTS requires NOM_BUILD_CORE_UNIT=ON.\n")
    endif()

    if(NOM_BUILD_AUDIO_TESTS AND NOT NOM_BUILD_AUDIO_UNIT)
      string(APPEND _NOM_ERRORS
        "  [ERROR] NOM_BUILD_AUDIO_TESTS requires NOM_BUILD_AUDIO_UNIT=ON.\n")
    endif()

    if(NOM_BUILD_GRAPHICS_TESTS AND NOT NOM_BUILD_GRAPHICS_UNIT)
      string(APPEND _NOM_ERRORS
        "  [ERROR] NOM_BUILD_GRAPHICS_TESTS requires NOM_BUILD_GRAPHICS_UNIT=ON.\n")
    endif()

    if(NOM_BUILD_ACTIONS_TESTS AND NOT NOM_BUILD_ACTIONS_UNIT)
      string(APPEND _NOM_ERRORS
        "  [ERROR] NOM_BUILD_ACTIONS_TESTS requires NOM_BUILD_ACTIONS_UNIT=ON.\n")
    endif()

    if(NOM_BUILD_GUI_TESTS AND NOT NOM_BUILD_GUI_UNIT)
      string(APPEND _NOM_ERRORS
        "  [ERROR] NOM_BUILD_GUI_TESTS requires NOM_BUILD_GUI_UNIT=ON.\n")
    endif()

  endif()

  # -------------------------------------------------------------------------
  # Emit collected errors
  # -------------------------------------------------------------------------
  if(_NOM_ERRORS)
    message(FATAL_ERROR
      "\n==========================================================================\n"
      "Build Option Validation FAILED — fix the following before compiling:\n"
      "\n"
      "${_NOM_ERRORS}"
      "==========================================================================\n")
  endif()

  unset(_NOM_ERRORS)
  unset(_req)

endmacro(nom_validate_build_options)
