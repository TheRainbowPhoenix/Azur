# The Dear ImGui library for SDL + OpenGL3/OpenGLES2
set(SOURCES
  ../imgui/imgui.cpp
  ../imgui/imgui_demo.cpp
  ../imgui/imgui_draw.cpp
  ../imgui/imgui_tables.cpp
  ../imgui/imgui_widgets.cpp
  ../imgui/backends/imgui_impl_opengl3.cpp
  ../imgui/backends/imgui_impl_sdl.cpp)

# Use Freetype if the library is installed, or the emscripten port
if(AZUR_PLATFORM_EMSCRIPTEN)
  list(APPEND SOURCES ../imgui/misc/freetype/imgui_freetype.cpp)
else()
  pkg_check_modules(freetype2 freetype2 IMPORTED_TARGET)
  if(freetype2_FOUND)
    list(APPEND SOURCES ../imgui/misc/freetype/imgui_freetype.cpp)
  endif()
endif()

add_library(imgui ${SOURCES})

# Try to reduce the size of the binary code by including individual functions
# from the archive, instead of whole files
target_compile_options(imgui PRIVATE -ffunction-sections)
target_compile_options(imgui INTERFACE -Wl,--gc-sections)

# Use the included ImGui loader on desktop
if(AZUR_GRAPHICS_OPENGL_3_3)
  # Nothing do, loader is built-in
# Use the autonomous GLES 2.0 loader otherwise
elseif(AZUR_GRAPHICS_OPENGL_ES_2_0)
  target_compile_definitions(imgui PUBLIC
    -DIMGUI_IMPL_OPENGL_ES2)
endif()

# FreeType2 settings
if(freetype2_FOUND)
  target_compile_definitions(imgui PUBLIC -DIMGUI_ENABLE_FREETYPE)
  target_include_directories(imgui PRIVATE ../imgui/misc/freetype)
  target_link_libraries(imgui PUBLIC PkgConfig::freetype2)
elseif(AZUR_PLATFORM_EMSCRIPTEN)
  target_compile_definitions(imgui PUBLIC -DIMGUI_ENABLE_FREETYPE)
  target_include_directories(imgui PRIVATE ../imgui/misc/freetype)
  target_compile_options(imgui PUBLIC -sUSE_FREETYPE=2)
  target_link_options(imgui INTERFACE -sUSE_FREETYPE=2)
endif()

# Provide include directories
target_include_directories(imgui PUBLIC ../imgui ../imgui/backends)
