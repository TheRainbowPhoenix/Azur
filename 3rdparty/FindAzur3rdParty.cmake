#-----------------------------------------------------------------------------#
#  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                   #
# |  _/__\_  |   Designed by Lephe' and the Planète Casio community.          #
#  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>           #
#-----------------------------------------------------------------------------#
# User-facing CMake find module for third-party libraries installed with Azur.

# TODO: We look for 3rd-party libraries assuming non-fxSDK layout.
set(AZUR_PATH "$ENV{AZUR_PATH_${AZUR_PLATFORM}}")
set(AZUR_LIB "${AZUR_PATH}/lib/azur")
set(AZUR_INCLUDE "${AZUR_PATH}/include")

message("(Azur) Starting from AZUR_PATH environment variable: ${AZUR_PATH}")
message("(Azur) Will take libraries from: ${AZUR_LIB}")
message("(Azur) Will take includes from: ${AZUR_INCLUDE}")

# Where we look for includes.
set(AZUR_INCLUDE_3RDPARTY "${AZUR_INCLUDE}/azur/3rdparty")
# Output: libraries that Azur should link to (some are default when present).
set(AZUR_DEFAULT_3RDPARTY_LIBS)

if(AZUR_PLATFORM STREQUAL linux)
  find_package(PkgConfig REQUIRED)
endif()

#=== gl3w =====================================================================#

set(AZUR_INCLUDE_GL3W "${AZUR_INCLUDE_3RDPARTY}/gl3w")
set(AZUR_LIB_GL3W "${AZUR_LIB}/libgl3w.a")

if(EXISTS "${AZUR_INCLUDE_GL3W}" AND EXISTS "${AZUR_LIB_GL3W}")
  message("(Azur) Found gl3w at: ${AZUR_LIB_GL3W}")
  add_library(Azur::gl3w STATIC IMPORTED)

  set_target_properties(Azur::gl3w PROPERTIES
    IMPORTED_LOCATION "${AZUR_LIB_GL3W}"
    INTERFACE_INCLUDE_DIRECTORIES "${AZUR_INCLUDE_GL3W}"
    SYSTEM FALSE)

  if(AZUR_PLATFORM STREQUAL linux)
    message("(Azur/gl3w) Getting glx from pkg-config (required)")
    pkg_check_modules(glx REQUIRED glx IMPORTED_TARGET)
    target_link_libraries(Azur::gl3w INTERFACE PkgConfig::glx -ldl)
    list(APPEND AZUR_DEFAULT_3RDPARTY_LIBS Azur::gl3w)
  endif()
endif()

#=== Dear ImGui ===============================================================#

set(AZUR_INCLUDE_IMGUI "${AZUR_INCLUDE_3RDPARTY}/imgui")
set(AZUR_LIB_IMGUI "${AZUR_LIB}/libimgui.a")

if(EXISTS "${AZUR_INCLUDE_IMGUI}" AND EXISTS "${AZUR_LIB_IMGUI}")
  message("(Azur) Found Dear ImGui at: ${AZUR_LIB_IMGUI}")
  add_library(Azur::ImGui STATIC IMPORTED)

  # TODO: Why does Dear ImGui expose -Wl,--gc-sections?
  set_target_properties(Azur::ImGui PROPERTIES
    IMPORTED_LOCATION "${AZUR_LIB_IMGUI}"
    INTERFACE_INCLUDE_DIRECTORIES
      "${AZUR_INCLUDE_IMGUI};${AZUR_INCLUDE_IMGUI}/backends"
    INTERFACE_LINK_OPTIONS "-Wl,--gc-sections"
    SYSTEM FALSE)

  # Get FreeType from emscripten or from pkg-config.
  if(AZUR_PLATFORM STREQUAL emscripten)
    target_link_options(Azur::ImGui INTERFACE
      -sUSE_FREETYPE=1
      -sMIN_WEBGL_VERSION=2)
    target_compile_definitions(Azur::ImGui INTERFACE -DIMGUI_ENABLE_FREETYPE)
    target_include_directories(Azur::ImGui INTERFACE
      "${AZUR_INCLUDE_IMGUI}/misc/freetype")
  else()
    # TODO: Do we actually have an ImGui fallback if there's no FreeType?
    message("(Azur/imgui) Getting FreeType from pkg-config (optional)")
    pkg_check_modules(freetype2 freetype2 IMPORTED_TARGET)
    if(freetype2_FOUND)
      target_link_libraries(Azur::ImGui INTERFACE PkgConfig::freetype2)
      target_compile_definitions(Azur::ImGui INTERFACE -DIMGUI_ENABLE_FREETYPE)
    endif()
  endif()
endif()

#=== glm ======================================================================#

set(AZUR_INCLUDE_GLM "${AZUR_INCLUDE_3RDPARTY}/glm")

if(EXISTS "${AZUR_INCLUDE_GLM}")
  message("(Azur) Found GLM at: ${AZUR_INCLUDE_GLM}")
  add_library(Azur::GLM INTERFACE IMPORTED)

  set_target_properties(Azur::GLM PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${AZUR_INCLUDE_GLM}"
    SYSTEM FALSE)

  list(APPEND AZUR_DEFAULT_3RDPARTY_LIBS Azur::GLM)
endif()

#=== FFmpeg ===================================================================#
# This reproduces the configuration in ffmpeg's pkg-config files.

set(AZUR_LIB_AVUTIL "${AZUR_LIB}/libavutil.a")
set(AZUR_LIB_AVFORMAT "${AZUR_LIB}/libavformat.a")
set(AZUR_LIB_AVCODEC "${AZUR_LIB}/libavcodec.a")
set(AZUR_LIB_SWSCALE "${AZUR_LIB}/libswscale.a")
set(AZUR_LIB_X264 "${AZUR_LIB}/libx264.a")
set(AZUR_INCLUDE_FFMPEG "${AZUR_INCLUDE_3RDPARTY}/ffmpeg")

if(EXISTS "${AZUR_INCLUDE_FFMPEG}")
  message("(Azur) Found ffmpeg (headers) at: ${AZUR_INCLUDE_FFMPEG}")
  add_library(Azur::ffmpeg INTERFACE IMPORTED)
  target_include_directories(Azur::ffmpeg INTERFACE "${AZUR_INCLUDE_FFMPEG}")

  # ffnvcodec is header-only so it's included automatically.

  if(EXISTS "${AZUR_LIB_AVUTIL}")
    message("(Azur/ffmpeg) Our ffmpeg build has libavutil")
    add_library(Azur::libavutil STATIC IMPORTED)
    set_target_properties(Azur::libavutil PROPERTIES
      IMPORTED_LOCATION "${AZUR_LIB_AVUTIL}"
      # INTERFACE_LINK_LIBRARIES "-lva-drm;-lva;-lva-x11;-lva;-lvdpau;-lX11;-lm;-ldrm;-lva;-latomic;-pthread;-lX11"
      SYSTEM FALSE)
    target_link_libraries(Azur::ffmpeg INTERFACE Azur::libavutil)
  endif()

  if(EXISTS "${AZUR_LIB_AVCODEC}")
    message("(Azur/ffmpeg) Our ffmpeg build has libavcodec")
    add_library(Azur::libavcodec STATIC IMPORTED)
    set_target_properties(Azur::libavcodec PROPERTIES
      IMPORTED_LOCATION "${AZUR_LIB_AVCODEC}"
      INTERFACE_LINK_LIBRARIES Azur::libavutil
      # INTERFACE_LINK_LIBRARIES "-lvpx;-lm;-pthread;-lm;-latomic;-lx264;-lx265"
      SYSTEM FALSE)
    target_link_libraries(Azur::ffmpeg INTERFACE Azur::libavcodec)
  endif()

  if(EXISTS "${AZUR_LIB_AVFORMAT}")
    message("(Azur/ffmpeg) Our ffmpeg build has libavformat")
    add_library(Azur::libavformat STATIC IMPORTED)
    set_target_properties(Azur::libavformat PROPERTIES
      IMPORTED_LOCATION "${AZUR_LIB_AVFORMAT}"
      INTERFACE_LINK_LIBRARIES Azur::libavcodec
      # INTERFACE_LINK_LIBRARIES "-lm;-lz;-latomic"
      SYSTEM FALSE)
    target_link_libraries(Azur::ffmpeg INTERFACE Azur::libavformat)
  endif()

  if(EXISTS "${AZUR_LIB_SWSCALE}")
    message("(Azur/ffmpeg) Our ffmpeg build has libswscale")
    add_library(Azur::libswscale STATIC IMPORTED)
    set_target_properties(Azur::libswscale PROPERTIES
      IMPORTED_LOCATION "${AZUR_LIB_SWSCALE}"
      INTERFACE_LINK_LIBRARIES Azur::libavutil
      # INTERFACE_LINK_LIBRARIES "-lm;-latomic"
      SYSTEM FALSE)
    target_link_libraries(Azur::ffmpeg INTERFACE Azur::libswscale)
  endif()

  if(EXISTS "${AZUR_LIB_X264}")
    message("(Azur/ffmpeg) Our ffmpeg build has libx264")
    add_library(Azur::libx264 STATIC IMPORTED)
    set_target_properties(Azur::libx264 PROPERTIES
      IMPORTED_LOCATION "${AZUR_LIB_X264}"
      INTERFACE_LINK_LIBRARIES "-lpthread;-lm;-ldl")
    target_link_libraries(Azur::ffmpeg INTERFACE Azur::libx264)
  endif()
endif()

#=== stb libraries ============================================================#

set(AZUR_INCLUDE_STB "${AZUR_INCLUDE_3RDPARTY}/stb")

if(EXISTS "${AZUR_INCLUDE_STB}")
  message("(Azur) Found STB libraries at: ${AZUR_INCLUDE_STB}")
  add_library(Azur::STB INTERFACE IMPORTED)

  set_target_properties(Azur::STB PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${AZUR_INCLUDE_STB}"
    SYSTEM FALSE)

  list(APPEND AZUR_DEFAULT_3RDPARTY_LIBS Azur::STB)
endif()
