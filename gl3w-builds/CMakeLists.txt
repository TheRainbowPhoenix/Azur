# Use GL3W as an OpenGL loader on desktop
if(AZUR_GRAPHICS_OPENGL_3_3)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/../gl3w/src/gl3w.c"
    COMMAND ./gl3w_gen.py
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../gl3w")
  add_library(gl3w
    ../gl3w/src/gl3w.c)
  target_include_directories(gl3w PUBLIC
    ../gl3w/include)
  # Link with GLX for GL3W to find the extensions, and -ldl
  target_link_libraries(gl3w INTERFACE PkgConfig::glx -ldl)
endif()
