//---
// azur.sdl_opengl.gl: General OpenGL utilities
//---

#pragma once
#include <azur/defs.h>
AZUR_BEGIN_DECLS

#ifdef AZUR_GRAPHICS_OPENGL_3_3
# include "GL/gl3w.h"
# include <SDL2/SDL_opengl.h>
#endif /* OpenGL 3.3 */

#ifdef AZUR_GRAPHICS_OPENGL_ES_2_0
# define GL_GLEXT_PROTOTYPES
# include <SDL2/SDL_opengles2.h>

# define glGenVertexArrays glGenVertexArraysOES
# define glBindVertexArray glBindVertexArrayOES
# define glDeleteVertexArrays glDeleteVertexArraysOES
#endif /* OpenGL ES 2.0 */

/* azgl_error(): String description of a GLenum error code.
   The returned pointer might be a static buffer modified by further calls. */
char const *azgl_error(GLenum error_code);

/* azgl_compile_shader_file(): Load and compile a shader from file.

   This function loads the file at [path], and compiles it into a shader of the
   specified type. If the file cannot be loaded, the shader cannot be created,
   or there are compilation errors, this function returns 0. Otherwise, it
   returns the shader ID. */
GLuint azgl_compile_shader_file(GLenum type, char const *path);

/* azgl_compile_shader_source(): Load and compile a shader from its code.

   Like azgl_load_shader(), but without the filesystem step. If the size is set
   to -1, [code] is assumed to be NUL-terminated. */
GLuint azgl_compile_shader_source(GLenum type, char const *code, ssize_t size);

/* azgl_link_program(): Link a program.

   This function attaches the 0-terminated list of shaders to a program, links
   it, and returns the program ID (0 if link fails). */
GLuint azgl_link_program(GLuint shader_1, ... /* 0-terminated */);

/* azgl_load_program(): Load a program from shader source files.

   This function runs both azgl_compile_shader_file() and azgl_link_program()
   for all specified files. Each argument should be a pair with a shader type
   and a file name, with a final 0.

   Returns the program ID, or 0 if any loading/compilation/link step fails. */
GLuint azgl_load_program(
   GLenum type_1, char const *path_1,
   ... /* Pairs repeat until 0-terminated */);

AZUR_END_DECLS
