
# cmake/preflight.cmake
#
# nomlib Dependency Preflight CMake Integration
#
# This module runs the bin/preflight.sh dependency checker before the main
# CMake configuration proceeds. It parses the JSON output and makes the
# results available to both CMake and downstream scripts.
#
# The following variables are defined after inclusion:
#   PREFLIGHT_PASSED        - TRUE if all non-optional checks passed
#   PREFLIGHT_CHECKS        - List of check names that were run
#   PREFLIGHT_<NAME>_STATUS - PASS / FAIL / WARN for each check
#   PREFLIGHT_<NAME>_FIX    - Fix suggestion for failed/warn checks
#   PREFLIGHT_FAILED_CATEGORIES - List of failure category tags
#
# To skip preflight (e.g. in CI), pass -DNOM_SKIP_PREFLIGHT=ON to CMake.

option(NOM_SKIP_PREFLIGHT "Skip the dependency preflight check" OFF)

if(NOM_SKIP_PREFLIGHT)
  message(STATUS "Preflight: skipped (NOM_SKIP_PREFLIGHT=ON)")
  set(PREFLIGHT_PASSED TRUE)
  return()
endif()

set(PREFLIGHT_SCRIPT "${CMAKE_SOURCE_DIR}/bin/preflight.sh")

if(NOT EXISTS "${PREFLIGHT_SCRIPT}")
  message(WARNING "Preflight script not found at ${PREFLIGHT_SCRIPT}; skipping preflight checks.")
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
  message(WARNING "Preflight produced no output; continuing without dependency validation.")
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

message(STATUS "Preflight: ${PREFLIGHT_PASS_COUNT} passed, ${PREFLIGHT_FAIL_COUNT} failed, ${PREFLIGHT_WARN_COUNT} warnings (target=${PREFLIGHT_TARGET_ARCH})")

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
    set("PREFLIGHT_${_name_upper}_STATUS" "${_status}" CACHE INTERNAL "Preflight status for ${_name}")
    set("PREFLIGHT_${_name_upper}_CATEGORY" "${_category}" CACHE INTERNAL "Preflight category for ${_name}")
    set("PREFLIGHT_${_name_upper}_DETAIL" "${_detail}" CACHE INTERNAL "Preflight detail for ${_name}")
    set("PREFLIGHT_${_name_upper}_FIX" "${_fix}" CACHE INTERNAL "Preflight fix for ${_name}")
  endforeach()
endif()

if(NOT PREFLIGHT_PASSED)
  message("")
  message("======================================================================")
  message("  nomlib PREFLIGHT FAILED")
  message("======================================================================")
  message("")
  message("  The following checks did not pass:")
  message("")

  if(PREFLIGHT_CHECKS_LEN GREATER 0)
    math(EXPR PREFLIGHT_CHECKS_LAST "${PREFLIGHT_CHECKS_LEN} - 1")
    foreach(_idx RANGE 0 ${PREFLIGHT_CHECKS_LAST})
      string(JSON _check GET "${PREFLIGHT_CHECKS_ARRAY}" ${_idx})
      string(JSON _name GET "${_check}" "name")
      string(JSON _status GET "${_check}" "status")

      if(_status STREQUAL "FAIL")
        string(JSON _fix_type TYPE "${_check}" "fix")
        set(_fix_text "")
        if(NOT _fix_type STREQUAL "null")
          string(JSON _fix_text GET "${_check}" "fix")
        endif()
        string(JSON _detail_type TYPE "${_check}" "detail")
        set(_detail_text "")
        if(NOT _detail_type STREQUAL "null")
          string(JSON _detail_text GET "${_check}" "detail")
        endif()

        message("    [FAIL] ${_name}")
        if(_detail_text)
          string(REPLACE "\n" "\n           " _detail_indented "${_detail_text}")
          message("           ${_detail_indented}")
        endif()
        if(_fix_text)
          string(REPLACE "\n" "\n           " _fix_indented "${_fix_text}")
          message("           Fix: ${_fix_indented}")
        endif()
        message("")
      endif()
    endforeach()
  endif()

  message("  To re-run the preflight check manually:")
  message("    ${PREFLIGHT_SCRIPT}")
  message("")
  message("  To view only fixes:")
  message("    ${PREFLIGHT_SCRIPT} --fixes")
  message("")
  message("  To skip this check (not recommended):")
  message("    cmake -DNOM_SKIP_PREFLIGHT=ON ..")
  message("")
  message("======================================================================")

  set(_has_arch_mismatch FALSE)
  set(_has_gui_missing FALSE)
  set(_has_audio_missing FALSE)
  set(_has_env_issue FALSE)

  foreach(_cat ${PREFLIGHT_FAILED_CATEGORIES})
    if(_cat STREQUAL "arch_mismatch")
      set(_has_arch_mismatch TRUE)
    elseif(_cat STREQUAL "gui_missing")
      set(_has_gui_missing TRUE)
    elseif(_cat STREQUAL "audio_missing")
      set(_has_audio_missing TRUE)
    elseif(_cat STREQUAL "env")
      set(_has_env_issue TRUE)
    endif()
  endforeach()

  if(_has_arch_mismatch)
    message(FATAL_ERROR
      "Preflight: Architecture mismatch detected (arm64/x86_64). "
      "Please ensure all dependencies are built for the target architecture (${PREFLIGHT_TARGET_ARCH}). "
      "Run '${PREFLIGHT_SCRIPT} --fixes' for detailed instructions."
    )
  elseif(_has_gui_missing)
    message(FATAL_ERROR
      "Preflight: Missing GUI dependencies (SDL2 / SDL2_image / SDL2_ttf / LibRocket). "
      "Please install the required GUI libraries before building. "
      "Run '${PREFLIGHT_SCRIPT} --fixes' for detailed instructions."
    )
  elseif(_has_audio_missing AND NOM_BUILD_AUDIO_UNIT)
    message(FATAL_ERROR
      "Preflight: Missing audio dependencies (OpenAL / libsndfile) but NOM_BUILD_AUDIO_UNIT=ON. "
      "Either install the audio dependencies or disable the audio unit with -DNOM_BUILD_AUDIO_UNIT=OFF. "
      "Run '${PREFLIGHT_SCRIPT} --fixes' for detailed instructions."
    )
  elseif(_has_env_issue)
    message(FATAL_ERROR
      "Preflight: Environment issue detected (third-party directory empty or misconfigured). "
      "Please set up your build environment correctly. "
      "Run '${PREFLIGHT_SCRIPT} --fixes' for detailed instructions."
    )
  else()
    message(FATAL_ERROR
      "Preflight checks failed. Please fix the issues listed above and re-run CMake. "
      "Run '${PREFLIGHT_SCRIPT}' outside CMake for a more detailed report."
    )
  endif()
endif()

message(STATUS "Preflight: all required checks passed.")
