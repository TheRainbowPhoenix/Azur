cmake_minimum_required(VERSION 3.19)
project(Facets VERSION 0.1)

# TODO: Set AZUR_DEBUG based on CMake Debug/Release setting
set(AZUR_DEBUG ON)

# When using [fxsdk build-cg], assume gint
if("${FXSDK_PLATFORM_LONG}" STREQUAL fxCG50)
  set(FACETS_PLATFORM gint)
endif()

# General options
add_compile_options(-Wall -Wextra -O2
  -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/=)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# gint

if(FACETS_PLATFORM STREQUAL gint)
  set(AZUR_PLATFORM_GENERIC TRUE)
  set(AZUR_TOOLKIT_GINT TRUE)
  set(AZUR_GRAPHICS_GINT_CG TRUE)
  set(AZUR_TERMINAL_NONE TRUE)

  find_package(Gint 2.4 REQUIRED)
  find_package(LibProf 2.4 REQUIRED)

  link_libraries(LibProf::LibProf Gint::Gint)
endif()

# linux

if(FACETS_PLATFORM STREQUAL linux)
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

# emscripten

if(FACETS_PLATFORM STREQUAL emscripten)
  set(AZUR_PLATFORM_EMSCRIPTEN TRUE)
  set(AZUR_TOOLKIT_SDL TRUE)
  set(AZUR_GRAPHICS_OPENGL_ES_2_0 TRUE)
  set(AZUR_TERMINAL_PLAIN TRUE)

  set(PORTS -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS=["png"])
  add_compile_options(${PORTS})
  add_link_options(${PORTS} -O3)
endif()

#---

if(NOT FACETS_PLATFORM STREQUAL gint)
  # This provides GL3W on desktop
  add_subdirectory(gl3w-builds)
  # This provides Dear ImGui
  add_subdirectory(imgui-builds)
  # This provide GLM
  add_subdirectory(glm-builds)
endif()

add_subdirectory(azur)
