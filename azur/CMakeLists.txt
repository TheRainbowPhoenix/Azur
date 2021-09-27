cmake_minimum_required(VERSION 3.15)
project(Azur VERSION 0.1 LANGUAGES CXX C ASM)

configure_file(include/azur/config.h.in include/azur/config.h)

set(SOURCES
  src/log.c)
set(ASSETS)

# All flavours of OpenGL
if(AZUR_GRAPHICS_OPENGL_3_3 OR AZUR_GRAPHICS_OPENGL_ES_2_0)
  set(AZUR_GRAPHICS_OPENGL TRUE)
endif()

# SDL/OpenGL rendering
if(AZUR_TOOLKIT_SDL AND AZUR_GRAPHICS_OPENGL)
  list(APPEND SOURCES
    src/sdl_opengl/init.c
    src/sdl_opengl/util.c)
  list(APPEND ASSETS
    glsl/vs_prelude_gles2.glsl
    glsl/fs_prelude_gles2.glsl
    glsl/vs_tex2d.glsl
    glsl/fs_tex2d.glsl)
endif()

# gint rendering
if(AZUR_GRAPHICS_GINT_CG)
  list(APPEND SOURCES
    src/gint/render.c
    src/gint/shaders/clear.c
    src/gint/shaders/clear.S
    src/gint/shaders/image.c
    src/gint/shaders/image.S)
endif()

add_library(azur STATIC ${SOURCES})

# File preloading on emscripten
if(AZUR_PLATFORM_EMSCRIPTEN)
  set_target_properties(azur PROPERTIES
    INTERFACE_LINK_OPTIONS "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/glsl@/azur/glsl"
    LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/glsl")

  # Add assets to link dependencies
  set(ASSETS_ABSOLUTE)
  foreach(ASSET IN LISTS ASSETS)
    list(APPEND ASSETS_ABSOLUTE "${CMAKE_CURRENT_SOURCE_DIR}/${ASSET}")
  endforeach()
  set_target_properties(azur PROPERTIES LINK_DEPENDS "${ASSETS_ABSOLUTE}")
endif()

# Use ImGui's integrated GL3W loader (which we build separately)
if(AZUR_GRAPHICS_OPENGL_3_3)
  target_link_libraries(azur PUBLIC gl3w)
endif()

target_include_directories(azur
  PUBLIC "${PROJECT_SOURCE_DIR}/include"
  PUBLIC "${PROJECT_BINARY_DIR}/include"
)
