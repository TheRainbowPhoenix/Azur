#-----------------------------------------------------------------------------#
#  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                   #
# |  _/__\_  |   Designed by Lephe' and the Planète Casio community.          #
#  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>           #
#-----------------------------------------------------------------------------#
# Main library build system

configure_file(include/azur/config.h.in include/azur/config.h)
set(AUTOGEN "${CMAKE_CURRENT_BINARY_DIR}/autogen")

set(SOURCES
  src/log.cpp
  src/resources.cc)
set(ASSETS)

# SDL/OpenGL rendering
if(AZUR_TOOLKIT_SDL AND AZUR_GRAPHICS_OPENGL)
  list(APPEND SOURCES
    src/gl/init.cpp
    src/gl/imgui.cpp
    src/gl/program.cc
    src/gl/surface.cc
    src/gl/texture.cc
    src/gl/util.cpp
    src/stb/stb_image.c
    src/stb/stb_image_write.c
    "${CMAKE_CURRENT_BINARY_DIR}/src/glsl.c"
    "${AUTOGEN}/assets.cc")

  list(APPEND ASSETS
    glsl/vs_prelude_gles2.glsl
    glsl/fs_prelude_gles2.glsl
    glsl/vs_prelude_gles3.glsl
    glsl/fs_prelude_gles3.glsl
    glsl/vs_prelude_gl3.glsl
    glsl/fs_prelude_gl3.glsl
    glsl/vs_tex2d.glsl
    glsl/fs_tex2d.glsl)

  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/src/glsl.c"
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/src"
    COMMAND python3 gen_glsl.py "${CMAKE_CURRENT_BINARY_DIR}/src/glsl.c"
            ${ASSETS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating a C file with prebuilt shaders"
    DEPENDS gen_glsl.py ${ASSETS})

  # azur_embed_assets(), but manually, since we haven't installed yet
  add_custom_command(
    OUTPUT "${AUTOGEN}/assets.cc"
    COMMAND mkdir -p "${AUTOGEN}"
    COMMAND python3 "tools/gen-assets.py"
                    -o "${AUTOGEN}/assets.cc" -n "azur" --allow-reserved-names
                    -i "azur_registerResourceGroup_azur"
                    -- -d "." ${ASSETS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Embedding assets"
    DEPENDS ${ASSETS})

  # stb_image_write documents that it assumes no strict aliasing rule
  set_source_files_properties(src/stb/stb_image_write.c APPEND
    PROPERTY COMPILE_OPTIONS -fnostrict-aliasing)

  # TODO: Include GLM through an interface target
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/glm")
endif()

if(AZUR_TOOLKIT_GINT)
  list(APPEND SOURCES
    src/gint/init.cpp)
endif()

# gint rendering
if(AZUR_GRAPHICS_GINT_CG)
  list(APPEND SOURCES
    src/gint/render.c
    src/gint/r61524.s
    # Clear shader
    src/gint/shaders/clear.c
    src/gint/shaders/clear.S
    # Image shader
    src/gint/shaders/image.c
    src/gint/shaders/image_rgb16_normal.S
    src/gint/shaders/image_rgb16_clearbg.S
    src/gint/shaders/image_rgb16_swapcolor.S
    src/gint/shaders/image_rgb16_dye.S
    src/gint/shaders/image_p8_normal.S
    src/gint/shaders/image_p8_swapcolor.S
    src/gint/shaders/image_p4_normal.S
    # Image shader interface
    src/gint/shaders/image_rgb16.c
    src/gint/shaders/image_rgb16_effect.c
    src/gint/shaders/image_rgb16_swapcolor.c
    src/gint/shaders/image_rgb16_dye.c
    src/gint/shaders/image_p8.c
    src/gint/shaders/image_p8_effect.c
    src/gint/shaders/image_p8_swapcolor.c
    src/gint/shaders/image_p8_dye.c
    src/gint/shaders/image_p4.c
    src/gint/shaders/image_p4_effect.c
    src/gint/shaders/image_p4_swapcolor.c
    src/gint/shaders/image_p4_dye.c
    # Triangle shader
    src/gint/shaders/triangle.c
    src/gint/shaders/triangle.S
    # Rectangle shader
    src/gint/shaders/rect.c
    src/gint/shaders/rect.S
    # Text shader
    src/gint/shaders/text.c
    src/gint/shaders/text.S
    # Line shader
    src/gint/shaders/line.c)
endif()

add_library(azur STATIC ${SOURCES})
if(NOT AZUR_PLATFORM STREQUAL gint)
  target_compile_options(azur PRIVATE -pthread)
endif()
target_link_libraries(azur PUBLIC num)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(azur PUBLIC
    -Wno-mathematical-notation-identifier-extension)
endif()

# Link with GL3W to load the OpenGL 3.3 core profile
if(AZUR_GRAPHICS_OPENGL_3_3)
  target_link_libraries(azur PUBLIC gl3w)
endif()

if(NOT AZUR_PLATFORM STREQUAL gint)
  target_link_libraries(azur PUBLIC imgui)
  target_include_directories(azur PRIVATE "../3rdparty/stb/")
endif()

target_include_directories(azur PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_BINARY_DIR}/include")

#---
# Install
#---

# Library file: libazur.a
install(TARGETS azur DESTINATION ${LIBDIR})
# Headers: azur/*.h
install(DIRECTORY include/ DESTINATION ${INCDIR}
  FILES_MATCHING PATTERN "*.h")
# Generated header: azur/config.h
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/azur/config.h"
  DESTINATION ${INCDIR}/azur)
# Assets and tools
install(DIRECTORY assets DESTINATION ${DATADIR})
install(DIRECTORY tools DESTINATION ${DATADIR})
