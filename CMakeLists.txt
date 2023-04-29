#-----------------------------------------------------------------------------#
#  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                   #
# |  _/__\_  |   Designed by Lephe' and the Planète Casio community.          #
#  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>           #
#-----------------------------------------------------------------------------#
# Main build system
#
# The build system consists of this CMakeLists.txt file, the CMakeLists.txt
# files of each component folder, and importantly the FindAzur.cmake file
# which loads the library for users.
#
# We build and install third-party dependencies along the engine to alleviate
# some packaging/distribution issues. Currently, these are:
#
# o  GL3W, which we rely on to load the OpenGL core profile on PC
# o  Dear ImGui, for introspection and debugging tools
#
# The following variables can be set on the command-line at cmake time:
#
# -DCMAKE_BUILD_TYPE:
#   Debug       Debug build with -O0 -g
#   Release     Release build with -Os [default]
#
# -DAZUR_PLATFORM:
#   gint        Build for gint for fx-CG
#   linux       Build for Linux with SDL and OpenGL 3.3
#   emscripten  Build for Emscripten with SDL and OpenGL ES 2.0
#
# -DCMAKE_INSTALL_PREFIX:
#   The usual install prefix. It must be different for each platform. For gint,
#   it defaults to the compiler's data folder as usual. The environment
#   variable AZUR_PATH_<platform> is recognized by the find module.
#---

cmake_minimum_required(VERSION 3.15)
project(Azur VERSION 0.1 LANGUAGES C CXX ASM)
include(CTest)

set(CMAKE_INSTALL_MESSAGE LAZY)

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set("${CMAKE_BUILD_TYPE}" Release)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL Debug)
  set(AZUR_DEBUG TRUE)
else()
  set(AZUR_DEBUG FALSE)
endif()

# Detect when the fxSDK is used, and assume gint. This allows one to simply run
# `fxsdk build-cg` to build the engine.
if("${FXSDK_PLATFORM_LONG}" STREQUAL fxCG50)
  set(AZUR_PLATFORM gint)
endif()

# Default install paths
set(LIBDIR "lib")
set(INCDIR "include")
set(MODPATH "lib/cmake")

#---
# Target-specific configuration
#---

if(AZUR_PLATFORM STREQUAL gint)
  set(AZUR_PLATFORM_GENERIC TRUE)
  set(AZUR_TOOLKIT_GINT TRUE)
  set(AZUR_GRAPHICS_GINT_CG TRUE)
  set(AZUR_ARCH_SH4ALDSP TRUE)
  set(AZUR_TERMINAL_NONE TRUE)

  # Link with gint and libprof (we only do this to get include directories)
  find_package(Gint 2.8 REQUIRED)
  find_package(LibProf 2.4 REQUIRED)
  link_libraries(LibProf::LibProf Gint::Gint)

  # Install in the fxSDK sysroot
  set(LIBDIR "${FXSDK_LIB}")
  set(INCDIR "${FXSDK_INCLUDE}")
  set(MODPATH "${FXSDK_CMAKE_MODULE_PATH}")
endif()

if(AZUR_PLATFORM STREQUAL linux)
  set(AZUR_PLATFORM_GENERIC TRUE)
  set(AZUR_TOOLKIT_SDL TRUE)
  set(AZUR_GRAPHICS_OPENGL_3_3 TRUE)
  set(AZUR_ARCH_GENERIC TRUE)
  set(AZUR_TERMINAL_ANSI TRUE)

  # Once again, we link only for the include paths
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(sdl2 REQUIRED sdl2 IMPORTED_TARGET)
  pkg_check_modules(sdl2_image REQUIRED SDL2_image IMPORTED_TARGET)
  pkg_check_modules(opengl REQUIRED opengl IMPORTED_TARGET)
  pkg_check_modules(glx REQUIRED glx IMPORTED_TARGET)
  link_libraries(PkgConfig::sdl2_image PkgConfig::sdl2 PkgConfig::opengl -lm)
endif()

if(AZUR_PLATFORM STREQUAL emscripten)
  set(AZUR_PLATFORM_EMSCRIPTEN TRUE)
  set(AZUR_TOOLKIT_SDL TRUE)
  set(AZUR_GRAPHICS_OPENGL_ES_2_0 TRUE)
  set(AZUR_ARCH_GENERIC TRUE)
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

# Exclud third-party software from the build on gint; it wouldn't work
if(NOT AZUR_PLATFORM STREQUAL gint)
  add_subdirectory(3rdparty)
endif()

add_subdirectory(libnum)
add_subdirectory(azur)

#---
# Install
#---

# CMake module to find the library
install(FILES FindAzur.cmake DESTINATION ${MODPATH})

#---
# Testing
#---

# Force --verbose on CTest
add_custom_target(tests COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
