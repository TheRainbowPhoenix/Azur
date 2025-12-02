# 1. Find the library's install path and data

set(AZUR_PATH "$ENV{AZUR_PATH_${AZUR_PLATFORM}}")

if(NOT "${FXSDK_PLATFORM_LONG}" STREQUAL "")
  set(AZUR_PLATFORM gint)
  set(AZUR_LIB "${FXSDK_LIB}/azur")
  set(AZUR_LIB_AZUR "${AZUR_LIB}/libazur.a")
  set(AZUR_INCLUDE "${FXSDK_INCLUDE}")
  set(AZUR_DATA "${FXSDK_SYSROOT}/share/azur")

  message("(Azur) Using the fxSDK compiler path: ${AZUR_LIB}")
  message("(Azur) Will take includes from: ${AZUR_INCLUDE}")
  message("(Azur) Additional assets are at: ${AZUR_DATA}")
elseif(NOT "${AZUR_PATH}" STREQUAL "")
  set(AZUR_LIB "${AZUR_PATH}/lib/azur")
  set(AZUR_LIB_AZUR "${AZUR_LIB}/libazur.a")
  set(AZUR_INCLUDE "${AZUR_PATH}/include")
  set(AZUR_DATA "${AZUR_PATH}/share/azur")

  if(NOT EXISTS "${AZUR_LIB}")
    message(SEND_ERROR
      "AZUR_PATH was set to ${AZUR_PATH}, but ${AZUR_LIB_AZUR} does not exist")
  endif()

  message("(Azur) Found AZUR_PATH_${AZUR_PLATFORM}: ${AZUR_PATH}")
  message("(Azur) Will take libraries from: ${AZUR_LIB}")
  message("(Azur) Will take includes from: ${AZUR_INCLUDE}")
  message("(Azur) Additional assets are at: ${AZUR_DATA}")
else()
  unset(AZUR_PATH)
  find_library(AZUR_PATH "azur")
  if("${AZUR_PATH}" STREQUAL "AZUR_PATH-NOTFOUND")
    message(SEND_ERROR
      "Could not find libazur.a!\n"
      "You can specify the installation path with the environment variable "
      "AZUR_PATH, such as AZUR_PATH=/path/to/lib\n")
  else()
    set(AZUR_LIB "${AZUR_PATH}") # <prefix>/lib/azur
    get_filename_component(AZUR_INCLUDE "${AZUR_PATH}/../../include" ABSOLUTE)
    get_filename_component(AZUR_DATA "${AZUR_PATH}/../../share/azur" ABSOLUTE)

    message("(Azur) Found libazur.a at: ${AZUR_PATH}")
    message("(Azur) Will take libraries from: ${AZUR_LIB}")
    message("(Azur) Will take includes from: ${AZUR_INCLUDE}")
    message("(Azur) Additional assets are at: ${AZUR_DATA}")
  endif()
endif()

# 2. Find the library version and configuration

if(NOT EXISTS "${AZUR_INCLUDE}/azur/config.h")
  message(SEND_ERROR
    "Could not find <azur/config.h> at ${AZUR_INCLUDE}/azur/config.h\n"
    "Is libazur.a installed alongside the headers?")
endif()

execute_process(
  COMMAND sed "s/#define AZUR_VERSION \"\\([^\"]\\{1,\\}\\)\"/\\1/p; d"
          "${AZUR_INCLUDE}/azur/config.h"
  OUTPUT_VARIABLE AZUR_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE)
message("(Azur) Library version found in header: ${AZUR_VERSION}")

# TODO: ${AZUR_PATH}/lib will not work with the fxSDK sysroot
set(AZUR_3RDPARTY "")
set(AZUR_LIB_GL3W "${AZUR_LIB}/libgl3w.a")
set(AZUR_LIB_IMGUI "${AZUR_LIB}/libimgui.a")

if(EXISTS "${AZUR_INCLUDE}/azur/3rdparty/gl3w" AND EXISTS "${AZUR_LIB_GL3W}")
  list(APPEND AZUR_3RDPARTY "gl3w")
  message("(Azur) Found gl3w at: ${AZUR_LIB_GL3W}")
endif()

if(EXISTS "${AZUR_INCLUDE}/azur/3rdparty/imgui" AND EXISTS "${AZUR_LIB_IMGUI}")
  list(APPEND AZUR_3RDPARTY "imgui")
  message("(Azur) Found Dear ImGui at: ${AZUR_LIB_IMGUI}")
endif()

if(EXISTS "${AZUR_INCLUDE}/azur/3rdparty/glm")
  list(APPEND AZUR_3RDPARTY "glm")
  message("(Azur) Found GLM at: ${AZUR_INCLUDE}/azur/3rdparty/glm")
endif()

message("(Azur) Summary of external libraries found: ${AZUR_3RDPARTY}")

# 3. Handle usual find_package() arguments and perform version check

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Azur
  REQUIRED_VARS AZUR_LIB AZUR_INCLUDE
  VERSION_VAR AZUR_VERSION)

# 4. Find dependencies

find_package(Azur3rdParty REQUIRED)

if(AZUR_PLATFORM STREQUAL linux)
  find_package(PkgConfig REQUIRED)
endif()

# 5. Generate targets

if(Azur_FOUND)
  if(NOT TARGET Azur::Azur)
    add_library(Azur::Azur UNKNOWN IMPORTED)
  endif()

  # TODO: Don't force -L because that brings 3rdparty libs in -l namespace
  set_target_properties(Azur::Azur PROPERTIES
    IMPORTED_LOCATION "${AZUR_LIB_AZUR}"
    INTERFACE_INCLUDE_DIRECTORIES "${AZUR_INCLUDE}"
    INTERFACE_COMPILE_OPTIONS "-DAZUR_PLATFORM=${AZUR_PLATFORM}"
    INTERFACE_LINK_OPTIONS "-L${AZUR_LIB}"
    INTERFACE_LINK_LIBRARIES -lnum)

  if(AZUR_PLATFORM STREQUAL linux)
    pkg_check_modules(sdl2 REQUIRED sdl2 IMPORTED_TARGET)
    pkg_check_modules(sdl2_image REQUIRED SDL2_image IMPORTED_TARGET)
    pkg_check_modules(opengl REQUIRED opengl IMPORTED_TARGET)
    target_link_libraries(Azur::Azur INTERFACE
      PkgConfig::sdl2_image PkgConfig::sdl2 PkgConfig::opengl -lm)
  elseif(AZUR_PLATFORM STREQUAL emscripten)
    set(PORTS -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS=["png"])
    add_compile_options(${PORTS})
    add_link_options(${PORTS} -O3)
  endif()
endif()

if("gl3w" IN_LIST AZUR_3RDPARTY)
  if(NOT TARGET Azur::gl3w)
    add_library(Azur::gl3w UNKNOWN IMPORTED)
  endif()

  set_target_properties(Azur::gl3w PROPERTIES
    IMPORTED_LOCATION "${AZUR_LIB_GL3W}"
    INTERFACE_INCLUDE_DIRECTORIES "${AZUR_INCLUDE}/azur/3rdparty/gl3w")

  if(AZUR_PLATFORM STREQUAL linux)
    pkg_check_modules(glx REQUIRED glx IMPORTED_TARGET)
    target_link_libraries(Azur::gl3w INTERFACE PkgConfig::glx -ldl)
    target_link_libraries(Azur::Azur INTERFACE Azur::gl3w)
  endif()
endif()

if("glm" IN_LIST AZUR_3RDPARTY)
  if(NOT TARGET Azur::GLM)
    add_library(Azur::GLM INTERFACE IMPORTED)
  endif()

  set_target_properties(Azur::GLM PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${AZUR_INCLUDE}/azur/3rdparty")
endif()

if("imgui" IN_LIST AZUR_3RDPARTY)
  if(NOT TARGET Azur::ImGui)
    add_library(Azur::ImGui UNKNOWN IMPORTED)
  endif()

  set_target_properties(Azur::ImGui PROPERTIES
    IMPORTED_LOCATION "${AZUR_LIB_IMGUI}"
    INTERFACE_INCLUDE_DIRECTORIES
      "${AZUR_INCLUDE}/azur/3rdparty/imgui;${AZUR_INCLUDE}/azur/3rdparty/imgui/backends"
    INTERFACE_LINK_OPTIONS "-Wl,--gc-sections")

  if(AZUR_PLATFORM STREQUAL emscripten)
    target_link_options(Azur::ImGui INTERFACE
      -sUSE_FREETYPE=1
      -sMIN_WEBGL_VERSION=2)
    target_compile_definitions(Azur::ImGui INTERFACE -DIMGUI_ENABLE_FREETYPE)
    target_include_directories(Azur::ImGui INTERFACE
      "${AZUR_INCLUDE}/azur/3rdparty/imgui/misc/freetype")
  else()
    pkg_check_modules(freetype2 freetype2 IMPORTED_TARGET)
    if(freetype2_FOUND)
      target_link_libraries(Azur::ImGui INTERFACE PkgConfig::freetype2)
      target_compile_definitions(Azur::ImGui INTERFACE -DIMGUI_ENABLE_FREETYPE)
    endif()
  endif()
endif()

# 6. Provide utility functions

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
    COMMAND python "${AZUR_DATA}/tools/gen-assets.py"
                   -o "${EA_OUTPUT}" -n "${EA_NAME}" --
                   -d "${EA_RELATIVE_TO}" ${EA_ASSETS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Embedding assets"
    DEPENDS ${EA_ASSETS})
endfunction()
