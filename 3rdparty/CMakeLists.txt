#-----------------------------------------------------------------------------#
#  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                   #
# |  _/__\_  |   Designed by Lephe' and the Planète Casio community.          #
#  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>           #
#-----------------------------------------------------------------------------#
# Build system for third-party libraries bundled with Azur

#=== gl3w =====================================================================#

# Use GL3W to load the OpenGL 3.3 core profile
if(AZUR_GRAPHICS_OPENGL_3_3)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/gl3w/src/gl3w.c"
    COMMAND "${Python3_EXECUTABLE}" gl3w_gen.py
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/gl3w")

  add_library(gl3w gl3w/src/gl3w.c)
  target_include_directories(gl3w PUBLIC gl3w/include)

  # Link with GLX for GL3W to find the extensions, and -ldl
  target_link_libraries(gl3w INTERFACE PkgConfig::glx -ldl)

  # Library file: libazur_*_gl3w.a
  install(TARGETS gl3w DESTINATION ${LIBDIR})
  # Headers
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
else()
  pkg_check_modules(freetype2 freetype2 IMPORTED_TARGET)
  if(freetype2_FOUND)
    list(APPEND SOURCES imgui/misc/freetype/imgui_freetype.cpp)
  endif()
endif()

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

#=== FFmpeg ===================================================================#

if(AZUR_PLATFORM_LINUX)
  set(FFMPEG_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-prefix")
  set(FFMPEG_LIBS
    "${FFMPEG_PREFIX}/lib/libavcodec.a"
    "${FFMPEG_PREFIX}/lib/libavformat.a"
    "${FFMPEG_PREFIX}/lib/libavutil.a"
    "${FFMPEG_PREFIX}/lib/libswscale.a"
    "${FFMPEG_PREFIX}/lib/libx264.a")
    # TODO: libx264.a
  set(FFMPEG_LIBS ${FFMPEG_LIBS} PARENT_SCOPE)

  # First install nv-codec-headers (NVFF) which is needed for the Nvidia codecs.
  # We might not build them, but at least we can. Then do ffmpeg.
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

  add_custom_target(ffmpeg DEPENDS ${FFMPEG_LIBS})

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
