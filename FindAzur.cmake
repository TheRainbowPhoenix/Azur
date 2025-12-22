#-----------------------------------------------------------------------------#
#  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                   #
# |  _/__\_  |   Designed by Lephe' and the Planète Casio community.          #
#  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>           #
#-----------------------------------------------------------------------------#
# User-facing CMake find module to find Azur and set its options
# See also FindAzur3rdParty.cmake which is a sub-script of this.

set(AZUR_PATH "$ENV{AZUR_PATH_${AZUR_PLATFORM}}")

find_package(Python3 COMPONENTS Interpreter)

# Avoid double-finding.
if(DEFINED AZUR_FOUND)
  return()
endif()

if(AZUR_PLATFORM STREQUAL linux)
  find_package(PkgConfig REQUIRED)
endif()


#=== Find Azur's install path =================================================#

## With the fxSDK sysroot (when building for calculators)
if(NOT "${FXSDK_PLATFORM_LONG}" STREQUAL "")
  set(AZUR_PLATFORM gint)
  set(AZUR_LIB "${FXSDK_LIB}/azur")
  set(AZUR_LIB_AZUR "${AZUR_LIB}/libazur.a")
  set(AZUR_INCLUDE "${FXSDK_INCLUDE}")
  set(AZUR_DATA "${FXSDK_SYSROOT}/share/azur")
  message("(Azur) Using the fxSDK sysroot")

## With the user-provided environment variable AZUR_PATH
elseif(NOT "${AZUR_PATH}" STREQUAL "")
  set(AZUR_LIB "${AZUR_PATH}/lib/azur")
  set(AZUR_LIB_AZUR "${AZUR_LIB}/libazur.a")
  set(AZUR_INCLUDE "${AZUR_PATH}/include")
  set(AZUR_DATA "${AZUR_PATH}/share/azur")

  if(NOT EXISTS "${AZUR_LIB}")
    message(FATAL_ERROR
      "AZUR_PATH was set to ${AZUR_PATH}, but ${AZUR_LIB_AZUR} does not exist")
  endif()

  message("(Azur) Found AZUR_PATH environment variable: ${AZUR_PATH}")
endif()

message("(Azur) Will take libraries from: ${AZUR_LIB}")
message("(Azur) Will take includes from: ${AZUR_INCLUDE}")
message("(Azur) Additional assets are at: ${AZUR_DATA}")


#=== Check version and find third-party packages ==============================#

set(AZUR_CONFIG_H "${AZUR_INCLUDE}/azur/azur/config.h")

if(NOT EXISTS "${AZUR_CONFIG_H}")
  message(FATAL_ERROR
    "Could not find <azur/config.h> at ${AZUR_CONFIG_H}\n"
    "Is Azur properly installed?")
endif()

execute_process(
  COMMAND sed "s/#define AZUR_VERSION \"\\([^\"]\\{1,\\}\\)\"/\\1/p; d"
          "${AZUR_CONFIG_H}"
  OUTPUT_VARIABLE AZUR_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE)
message("(Azur) Library version found in header: ${AZUR_VERSION}")

find_package(Azur3rdParty REQUIRED)

# Final version check
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Azur
  REQUIRED_VARS AZUR_LIB AZUR_INCLUDE AZUR_DATA
  VERSION_VAR AZUR_VERSION)

if(NOT Azur_FOUND)
  return()
endif()

# Libraries that Azur links to by default
set(AZUR_DEFAULT_LIBS)

#=== libnum ===================================================================#

set(AZUR_LIB_NUM "${AZUR_LIB}/libnum.a")
set(AZUR_INCLUDE_NUM "${AZUR_INCLUDE}/azur/libnum")

if(EXISTS "${AZUR_LIB_NUM}" AND EXISTS "${AZUR_INCLUDE_NUM}")
  message("(Azur) Providing libnum")
  add_library(Azur::libnum STATIC IMPORTED)

  set_target_properties(Azur::libnum PROPERTIES
    IMPORTED_LOCATION "${AZUR_LIB_NUM}"
    INTERFACE_INCLUDE_DIRECTORIES "${AZUR_INCLUDE_NUM}"
    SYSTEM FALSE)

  list(APPEND AZUR_DEFAULT_LIBS Azur::libnum)
endif()


#=== Azur =====================================================================#

add_library(Azur::Azur STATIC IMPORTED)

set_target_properties(Azur::Azur PROPERTIES
  IMPORTED_LOCATION "${AZUR_LIB_AZUR}"
  INTERFACE_INCLUDE_DIRECTORIES "${AZUR_INCLUDE}/azur"
  INTERFACE_COMPILE_OPTIONS "-DAZUR_PLATFORM=${AZUR_PLATFORM}"
  INTERFACE_LINK_LIBRARIES Azur::libnum
  SYSTEM FALSE)

if(AZUR_PLATFORM STREQUAL linux)
  message("(Azur) Getting sdl2 from pkg-config (required)")
  message("(Azur) Getting opengl from pkg-config (required)")
  pkg_check_modules(sdl2 REQUIRED sdl2 IMPORTED_TARGET)
  pkg_check_modules(opengl REQUIRED opengl IMPORTED_TARGET)
  target_link_libraries(Azur::Azur INTERFACE
      PkgConfig::sdl2 PkgConfig::opengl -lm)
elseif(AZUR_PLATFORM STREQUAL emscripten)
  set(PORTS -sUSE_SDL=2)
  add_compile_options(${PORTS})
  add_link_options(${PORTS} -O3)
endif()

message("(Azur) Third-party libraries linked by default: ${AZUR_DEFAULT_3RDPARTY_LIBS}")
message("(Azur) Internal libraries linked by default: ${AZUR_DEFAULT_LIBS}")
target_link_libraries(Azur::Azur INTERFACE
  ${AZUR_DEFAULT_3RDPARTY_LIBS} ${AZUR_DEFAULT_LIBS})


#=== Utility functions ========================================================#

function(azur_generate_html_template _output _js)
  set(AZUR_APP_TEMPLATE_JS ${_js})
  configure_file("${AZUR_DATA}/assets/app_template.html" "${_output}" @ONLY)
endfunction()

# Converted assets via a Python script equivalent to xxd -i into a source file
# and a header. Having a single file is not super elegant but it simplifies the
# dependencies a lot (it's pulled by source files and the header comes with it,
# so it's always up-to-date with no manual dependency declarations).
function(azur_embed_assets)
  cmake_parse_arguments(EA "" "OUTPUT;NAME;RELATIVE_TO" "" ${ARGN})

  # Check arguments
  if(NOT DEFINED EA_OUTPUT)
    message(FATAL_ERROR "azur_embed_assets: OUTPUT is required!")
  elseif(NOT DEFINED EA_NAME)
    message(FATAL_ERROR "azur_embed_assets: NAME is required!")
  elseif(NOT DEFINED EA_RELATIVE_TO)
    message(FATAL_ERROR "azur_embed_assets: RELATIVE_TO is required!")
  endif()
  set(EA_ASSETS ${EA_UNPARSED_ARGUMENTS})

  # Find folder where we're generating the output
  get_filename_component(EA_FOLDER "${EA_OUTPUT}" DIRECTORY)

  add_custom_command(
    OUTPUT "${EA_OUTPUT}"
    COMMAND mkdir -p "${EA_FOLDER}"
    COMMAND "${Python3_EXECUTABLE}"
            "${AZUR_DATA}/tools/gen-assets.py"
            -o "${EA_OUTPUT}" -n "${EA_NAME}" --
            -d "${EA_RELATIVE_TO}" ${EA_ASSETS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Embedding assets"
    DEPENDS ${EA_ASSETS})
endfunction()
