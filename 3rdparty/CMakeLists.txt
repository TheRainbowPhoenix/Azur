#-----------------------------------------------------------------------------#
#  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                   #
# |  _/__\_  |   Designed by Lephe' and the Planète Casio community.          #
#  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>           #
#-----------------------------------------------------------------------------#
# Build system for third-party libraries bundled with Azur

cmake_minimum_required(VERSION 3.15)
project(Azur3rdParty VERSION 0.3 LANGUAGES C CXX)

find_package(Python3 COMPONENTS Interpreter)

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
if("${FXSDK_PLATFORM_LONG}" STREQUAL fxCP)
  set(AZUR_PLATFORM gint)
endif()

if(AZUR_PLATFORM STREQUAL linux)
  set(AZUR_PLATFORM_LINUX TRUE)
  set(AZUR_GRAPHICS_OPENGL_3_3 TRUE)
  find_package(PkgConfig REQUIRED)
endif()
if(AZUR_PLATFORM STREQUAL emscripten)
  set(AZUR_PLATFORM_EMSCRIPTEN TRUE)
  set(PORTS -sUSE_SDL=2)
  add_compile_options(${PORTS})
  add_link_options(${PORTS})
endif()

# Options for the libs we compile from source here. Some of the dependencies
# are fully independent, like ffmpeg (configure/make).
add_compile_options(
  $<IF:$<CONFIG:DEBUG>,-O0,-Os>
  $<$<CONFIG:DEBUG>:-g>
  -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/=)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Install paths for third-party libraries
set(LIBDIR "lib/azur")
set(INCDIR3RDPARTY "include/azur/3rdparty")
set(MODPATH "lib/cmake")

if(AZUR_PLATFORM STREQUAL gint)
  # Install in the fxSDK sysroot
  set(LIBDIR "${FXSDK_LIB}/azur")
  set(INCDIR3RDPARTY "${FXSDK_INCLUDE}/azur/3rdparty")
  set(MODPATH "${FXSDK_CMAKE_MODULE_PATH}")
endif()

install(FILES FindAzur3rdParty.cmake DESTINATION ${MODPATH})

#=== gl3w =====================================================================#

# Use GL3W to load the OpenGL 3.3 core profile
if(AZUR_GRAPHICS_OPENGL_3_3)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/gl3w/src/gl3w.c"
    COMMAND "${Python3_EXECUTABLE}" gl3w_gen.py
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/gl3w")

  add_library(gl3w gl3w/src/gl3w.c)
  target_include_directories(gl3w PUBLIC gl3w/include)

  # Link with GLX for GL3W to find the extensions
  if(AZUR_PLATFORM STREQUAL linux)
    pkg_check_modules(glx REQUIRED glx IMPORTED_TARGET)
    pkg_check_modules(opengl REQUIRED opengl IMPORTED_TARGET)
    target_link_libraries(gl3w PUBLIC PkgConfig::glx PkgConfig::opengl -ldl)
  endif()

  install(TARGETS gl3w DESTINATION ${LIBDIR})
  install(DIRECTORY gl3w/include/ DESTINATION ${INCDIR3RDPARTY}/gl3w)
endif()

#=== glm ======================================================================#

# TODO: Get rid of the dependency GLM by using libnum instead

# Headers
install(DIRECTORY glm/glm DESTINATION ${INCDIR3RDPARTY}/glm)

#=== Dear ImGui ===============================================================#
# Configured for SDL + OpenGL3/OpenGLES3.

set(SOURCES
  imgui/imgui.cpp
  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
  imgui/backends/imgui_impl_opengl3.cpp
  imgui/backends/imgui_impl_sdl2.cpp)

# Use Freetype if the library is available; on emscripten there is a port
if(AZUR_PLATFORM_EMSCRIPTEN)
  list(APPEND SOURCES imgui/misc/freetype/imgui_freetype.cpp)
elseif(AZUR_PLATFORM_LINUX)
  pkg_check_modules(freetype2 freetype2 IMPORTED_TARGET)
  if(freetype2_FOUND)
    list(APPEND SOURCES imgui/misc/freetype/imgui_freetype.cpp)
  endif()
endif()

if(NOT AZUR_PLATFORM STREQUAL gint)
  add_library(imgui ${SOURCES})

  # Try to reduce the size of the binary code by including individual functions
  # from the archive, instead of whole files. (The interface library described in
  # FindAzur.cmake sets -Wl,--gc-sections.)
  target_compile_options(imgui PRIVATE -ffunction-sections)

  # FreeType2 settings
  if(AZUR_PLATFORM_EMSCRIPTEN)
    target_compile_definitions(imgui PUBLIC -DIMGUI_ENABLE_FREETYPE)
    target_include_directories(imgui PRIVATE imgui/misc/freetype)
    target_compile_options(imgui PUBLIC -sUSE_FREETYPE=1)
    target_link_options(imgui INTERFACE -sUSE_FREETYPE=1)
  elseif(freetype2_FOUND)
    target_compile_definitions(imgui PUBLIC -DIMGUI_ENABLE_FREETYPE)
    target_include_directories(imgui PRIVATE imgui/misc/freetype)
    target_link_libraries(imgui PUBLIC PkgConfig::freetype2)
  endif()

  if(AZUR_PLATFORM_LINUX)
    pkg_check_modules(sdl2 REQUIRED sdl2 IMPORTED_TARGET)
    target_link_libraries(imgui PUBLIC PkgConfig::sdl2)
  endif()

  # Provide include directories
  target_include_directories(imgui PUBLIC imgui imgui/backends)

  # Library file: libazur_*_imgui.a
  install(TARGETS imgui DESTINATION ${LIBDIR})
  # Headers
  install(DIRECTORY imgui/ DESTINATION ${INCDIR3RDPARTY}/imgui
    FILES_MATCHING PATTERN "*.h"
    PATTERN examples EXCLUDE
    PATTERN docs EXCLUDE
    PATTERN .github EXCLUDE)
endif()

#=== FFmpeg ===================================================================#

if(AZUR_PLATFORM_LINUX)
  set(FFMPEG_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-prefix")
  set(FFMPEG_LIBS
    "${FFMPEG_PREFIX}/lib/libavcodec.a"
    "${FFMPEG_PREFIX}/lib/libavformat.a"
    "${FFMPEG_PREFIX}/lib/libavutil.a"
    "${FFMPEG_PREFIX}/lib/libswscale.a"
    "${FFMPEG_PREFIX}/lib/libx264.a")

  add_custom_command(
    OUTPUT ${FFMPEG_LIBS}
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/build_ffmpeg.sh"
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/build_ffmpeg.sh"
            "${FFMPEG_PREFIX}"
            "${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg"
            "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-build"
            "${CMAKE_CURRENT_SOURCE_DIR}/x264"
            "${CMAKE_CURRENT_BINARY_DIR}/x264-build"
    COMMENT "Compiling ffmpeg and its dependencies...")

  add_custom_target(ffmpeg ALL DEPENDS ${FFMPEG_LIBS})

  # Install library files
  install(FILES ${FFMPEG_LIBS} DESTINATION ${LIBDIR})
  # Install headers
  install(DIRECTORY ${FFMPEG_PREFIX}/include/
    DESTINATION ${INCDIR3RDPARTY}/ffmpeg)
endif()

#=== stb libraries ============================================================#

# Headers: azur/stb_*.h
install(FILES
  stb/stb_image.h
  stb/stb_image_write.h
  DESTINATION ${INCDIR3RDPARTY}/stb)
