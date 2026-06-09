
# cmake/preflight.cmake
#
# nomlib Dependency Preflight CMake Integration
#
# This module runs bin/preflight.sh AFTER all project options and third-party
# path variables have been declared. It parses the JSON output and makes
# results available as CMake variables, but does NOT stop the configuration
# with FATAL_ERROR -- the outer build scripts (bin/_build_core.sh) and the
# normal find_package() calls take responsibility for stopping the build on
# missing dependencies.
#
# Variables defined after inclusion:
#   PREFLIGHT_PASSED              - TRUE if all non-optional checks passed
#   PREFLIGHT_CHECKS              - List of check names that were run
#   PREFLIGHT_<NAME>_STATUS       - PASS / FAIL / WARN for each check
#   PREFLIGHT_<NAME>_CATEGORY     - Category tag (gui_missing, audio_missing, ...)
#   PREFLIGHT_<NAME>_DETAIL       - Human-readable detail
#   PREFLIGHT_<NAME>_FIX          - Fix suggestion
#   PREFLIGHT_FAILED_CATEGORIES   - List of failure category tags
#
# To skip preflight (e.g. in CI):
#   cmake -DNOM_SKIP_PREFLIGHT=ON ..
# or
#   NOM_SKIP_PREFLIGHT=1 ./bin/configure.sh ...

option(NOM_SKIP_PREFLIGHT "Skip the dependency preflight check" OFF)

if(NOM_SKIP_PREFLIGHT)
  message(STATUS "Preflight: skipped (NOM_SKIP_PREFLIGHT=ON)")
  set(PREFLIGHT_PASSED TRUE)
  return()
endif()

set(PREFLIGHT_SCRIPT "${CMAKE_SOURCE_DIR}/bin/preflight.sh")

if(NOT EXISTS "${PREFLIGHT_SCRIPT}")
  message(STATUS "Preflight: script not found at ${PREFLIGHT_SCRIPT}; skipping.")
  set(PREFLIGHT_PASSED TRUE)
  return()
endif()

message(STATUS "Running dependency preflight...")

set(PREFLIGHT_ARCH_ARG "")
if(CMAKE_OSX_ARCHITECTURES)
  list(LENGTH CMAKE_OSX_ARCHITECTURES _arch_count)
  if(_arch_count EQUAL 1)
    list(GET CMAKE_OSX_ARCHITECTURES 0 _single_arch)
    set(PREFLIGHT_ARCH_ARG "--arch" "${_single_arch}")
  endif()
endif()

execute_process(
  COMMAND "${PREFLIGHT_SCRIPT}" --json ${PREFLIGHT_ARCH_ARG}
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE PREFLIGHT_JSON
  ERROR_VARIABLE PREFLIGHT_STDERR
  RESULT_VARIABLE PREFLIGHT_EXIT_CODE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(PREFLIGHT_STDERR)
  message(STATUS "Preflight stderr: ${PREFLIGHT_STDERR}")
endif()

set(PREFLIGHT_PASSED FALSE)
if(PREFLIGHT_EXIT_CODE EQUAL 0)
  set(PREFLIGHT_PASSED TRUE)
endif()

if(NOT PREFLIGHT_JSON)
  message(STATUS "Preflight: no JSON output; continuing without validation.")
  return()
endif()

string(JSON PREFLIGHT_SUMMARY GET "${PREFLIGHT_JSON}" "summary")
string(JSON PREFLIGHT_TOTAL GET "${PREFLIGHT_SUMMARY}" "total")
string(JSON PREFLIGHT_PASS_COUNT GET "${PREFLIGHT_SUMMARY}" "passed")
string(JSON PREFLIGHT_FAIL_COUNT GET "${PREFLIGHT_SUMMARY}" "failed")
string(JSON PREFLIGHT_WARN_COUNT GET "${PREFLIGHT_SUMMARY}" "warnings")

string(JSON PREFLIGHT_TARGET_ARCH GET "${PREFLIGHT_JSON}" "target_arch")
string(JSON PREFLIGHT_HOST_ARCH GET "${PREFLIGHT_JSON}" "host_arch")
string(JSON PREFLIGHT_PLATFORM GET "${PREFLIGHT_JSON}" "platform")

message(STATUS
  "Preflight: ${PREFLIGHT_PASS_COUNT} passed, ${PREFLIGHT_FAIL_COUNT} failed, "
  "${PREFLIGHT_WARN_COUNT} warnings (target=${PREFLIGHT_TARGET_ARCH}, host=${PREFLIGHT_HOST_ARCH})"
)

string(JSON PREFLIGHT_CAT_ARRAY GET "${PREFLIGHT_JSON}" "failed_categories")
string(JSON PREFLIGHT_CAT_LEN LENGTH "${PREFLIGHT_CAT_ARRAY}")
set(PREFLIGHT_FAILED_CATEGORIES "")
if(PREFLIGHT_CAT_LEN GREATER 0)
  math(EXPR PREFLIGHT_CAT_LAST "${PREFLIGHT_CAT_LEN} - 1")
  foreach(_cat_idx RANGE 0 ${PREFLIGHT_CAT_LAST})
    string(JSON _cat GET "${PREFLIGHT_CAT_ARRAY}" ${_cat_idx})
    list(APPEND PREFLIGHT_FAILED_CATEGORIES "${_cat}")
  endforeach()
endif()

string(JSON PREFLIGHT_CHECKS_ARRAY GET "${PREFLIGHT_JSON}" "checks")
string(JSON PREFLIGHT_CHECKS_LEN LENGTH "${PREFLIGHT_CHECKS_ARRAY}")
set(PREFLIGHT_CHECKS "")

if(PREFLIGHT_CHECKS_LEN GREATER 0)
  math(EXPR PREFLIGHT_CHECKS_LAST "${PREFLIGHT_CHECKS_LEN} - 1")
  foreach(_idx RANGE 0 ${PREFLIGHT_CHECKS_LAST})
    string(JSON _check GET "${PREFLIGHT_CHECKS_ARRAY}" ${_idx})
    string(JSON _name GET "${_check}" "name")
    string(JSON _status GET "${_check}" "status")
    string(JSON _category GET "${_check}" "category")

    string(JSON _detail_type TYPE "${_check}" "detail")
    set(_detail "")
    if(NOT _detail_type STREQUAL "null")
      string(JSON _detail GET "${_check}" "detail")
    endif()

    string(JSON _fix_type TYPE "${_check}" "fix")
    set(_fix "")
    if(NOT _fix_type STREQUAL "null")
      string(JSON _fix GET "${_check}" "fix")
    endif()

    string(MAKE_C_IDENTIFIER "${_name}" _name_id)
    string(TOUPPER "${_name_id}" _name_upper)

    list(APPEND PREFLIGHT_CHECKS "${_name}")
    set("PREFLIGHT_${_name_upper}_STATUS"   "${_status}"   CACHE INTERNAL "Preflight status for ${_name}")
    set("PREFLIGHT_${_name_upper}_CATEGORY" "${_category}" CACHE INTERNAL "Preflight category for ${_name}")
    set("PREFLIGHT_${_name_upper}_DETAIL"   "${_detail}"   CACHE INTERNAL "Preflight detail for ${_name}")
    set("PREFLIGHT_${_name_upper}_FIX"      "${_fix}"      CACHE INTERNAL "Preflight fix for ${_name}")
  endforeach()
endif()

# ---------------------------------------------------------------------------
# Report failures (WARNING only, no FATAL_ERROR -- shell and find_package handle it)
# ---------------------------------------------------------------------------

if(NOT PREFLIGHT_PASSED)
  set(_fail_summary "")
  if(PREFLIGHT_CHECKS_LEN GREATER 0)
    math(EXPR PREFLIGHT_CHECKS_LAST "${PREFLIGHT_CHECKS_LEN} - 1")
    foreach(_idx RANGE 0 ${PREFLIGHT_CHECKS_LAST})
      string(JSON _check GET "${PREFLIGHT_CHECKS_ARRAY}" ${_idx})
      string(JSON _name GET "${_check}" "name")
      string(JSON _status GET "${_check}" "status")
      if(_status STREQUAL "FAIL")
        string(JSON _detail_type TYPE "${_check}" "detail")
        set(_detail_text "")
        if(NOT _detail_type STREQUAL "null")
          string(JSON _detail_text GET "${_check}" "detail")
        endif()
        string(JSON _category GET "${_check}" "category")

        # Skip reporting failures for components that are disabled via options
        set(_skip FALSE)
        if(_category STREQUAL "audio_missing" AND DEFINED NOM_BUILD_AUDIO_UNIT AND NOT NOM_BUILD_AUDIO_UNIT)
          set(_skip TRUE)
        endif()
        if(_category STREQUAL "gui_missing" AND DEFINED NOM_BUILD_GUI_UNIT AND NOT NOM_BUILD_GUI_UNIT)
          set(_skip TRUE)
        endif()
        if(_category STREQUAL "test_missing" AND DEFINED NOM_BUILD_TESTS AND NOT NOM_BUILD_TESTS)
          set(_skip TRUE)
        endif()

        if(NOT _skip)
          string(APPEND _fail_summary "    [FAIL] ${_name} (${_category})\n")
          if(_detail_text)
            string(REPLACE "\n" "\n           " _detail_indented "${_detail_text}")
            string(APPEND _fail_summary "           ${_detail_indented}\n")
          endif()
        endif()
      endif()
    endforeach()
  endif()

  if(_fail_summary)
    message(WARNING
      "\n"
      "======================================================================\n"
      "  nomlib PREFLIGHT -- ISSUES DETECTED\n"
      "======================================================================\n"
      "\n"
      "${_fail_summary}"
      "  Manual preflight:\n"
      "    ${PREFLIGHT_SCRIPT}\n"
      "  View only fixes:\n"
      "    ${PREFLIGHT_SCRIPT} --fixes\n"
      "  To skip this check (not recommended):\n"
      "    cmake -DNOM_SKIP_PREFLIGHT=ON ..\n"
      "======================================================================\n"
    )
  else()
    message(STATUS "Preflight: some optional checks failed, but the corresponding build options are disabled; continuing.")
  endif()
else()
  message(STATUS "Preflight: all required checks passed.")
endif()
