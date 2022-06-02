cmake_minimum_required(VERSION 3.19)
project(AzurEngine VERSION 0.1)

#---
# Configuration
#
# -DCMAKE_BUILD_TYPE
#    Debug       Debug build with -O0 -g
#    Release     Release build with -Os [default]
#
#  -DAZUR_PLATFORM
#    gint        Build for gint for fx-CG
#    linux       Build for Linux with SDL and OpenGL 3.3
#    emscripten  Build for Emscripten with SDL and OpenGL ES 2.0
#
#  -DCMAKE_INSTALL_PREFIX
#---

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set("${CMAKE_BUILD_TYPE}" Release)
endif()

# Debug build
if("${CMAKE_BUILD_TYPE}" STREQUAL Debug)
  set(AZUR_DEBUG TRUE)
else()
  set(AZUR_DEBUG FALSE)
endif()

# When using [fxsdk build-cg], assume gint
if("${FXSDK_PLATFORM_LONG}" STREQUAL fxCG50)
  set(AZUR_PLATFORM gint)
endif()

# Default install paths
set(LIBDIR "lib")
set(INCDIR "include")

# Target configuration: gint
if("${AZUR_PLATFORM}" STREQUAL "gint")
  set(AZUR_PLATFORM_GENERIC TRUE)
  set(AZUR_TOOLKIT_GINT TRUE)
  set(AZUR_GRAPHICS_GINT_CG TRUE)
  set(AZUR_TERMINAL_NONE TRUE)

  find_package(Gint 2.4 REQUIRED)
  find_package(LibProf 2.4 REQUIRED)

  link_libraries(LibProf::LibProf Gint::Gint)

  # Default to the fxSDK install path
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${FXSDK_COMPILER_INSTALL}" CACHE PATH "..." FORCE)
  endif()

  # The fxSDK install path has a slightly different structure
  if(CMAKE_INSTALL_PREFIX STREQUAL "${FXSDK_COMPILER_INSTALL}")
    set(LIBDIR ".")
    set(INCDIR "include")
  endif()
endif()

# Target configuration: linux
if(AZUR_PLATFORM STREQUAL linux)
  set(AZUR_PLATFORM_GENERIC TRUE)
  set(AZUR_TOOLKIT_SDL TRUE)
  set(AZUR_GRAPHICS_OPENGL_3_3 TRUE)
  set(AZUR_TERMINAL_ANSI TRUE)

  find_package(PkgConfig REQUIRED)
  pkg_check_modules(sdl2 REQUIRED sdl2 IMPORTED_TARGET)
  pkg_check_modules(sdl2_image REQUIRED SDL2_image IMPORTED_TARGET)
  pkg_check_modules(opengl REQUIRED opengl IMPORTED_TARGET)
  pkg_check_modules(glx REQUIRED glx IMPORTED_TARGET)

  link_libraries(PkgConfig::sdl2_image PkgConfig::sdl2 PkgConfig::opengl -lm)
endif()

# Platform configuration: emscripten
if(AZUR_PLATFORM STREQUAL emscripten)
  set(AZUR_PLATFORM_EMSCRIPTEN TRUE)
  set(AZUR_TOOLKIT_SDL TRUE)
  set(AZUR_GRAPHICS_OPENGL_ES_2_0 TRUE)
  set(AZUR_TERMINAL_PLAIN TRUE)

  set(PORTS -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS=["png"])
  add_compile_options(${PORTS})
  add_link_options(${PORTS} -O3)
endif()

if(AZUR_GRAPHICS_OPENGL_3_3 OR AZUR_GRAPHICS_OPENGL_ES_2_0)
  set(AZUR_GRAPHICS_OPENGL TRUE)
endif()

#---
# Compile options
#---

add_compile_options(
  -Wall -Wextra
  $<IF:$<CONFIG:DEBUG>,-O0,-Os>
  $<$<CONFIG:DEBUG>:-g>
  -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/=)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

#---
# Subdirectory selection
#---

if(NOT AZUR_PLATFORM STREQUAL gint)
  # This provides GL3W on desktop
  add_subdirectory(gl3w-builds)
  # This provides Dear ImGui
  add_subdirectory(imgui-builds)
  # This provide GLM
  add_subdirectory(glm-builds)
endif()

add_subdirectory(libnum)
add_subdirectory(azur)
