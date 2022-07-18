//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// azur.gl.gl: OpenGL 3.3 / OpenGL ES 2.0 compatibility and utilities
//
// This header is specific to OpenGL targets. It provides helpers for some
// degree of compatibility between OpenGL ES 2.0 (with some extensions) and
// OpenGL 3.3. We always use OpenGL 3.3 as the reference and try to expose
// everything as if we are running OpenGL 3.3 on every platform.
//
// This header also provides utility functions to load and compile shaders.
// Azur provides a header for all shader files, which includes the `#version`
// string, modern keywords on OpenGL ES 2.0 (in/out/layout), and two macros
// _GL() and _GLES() that expand to their argument on the corresponding
// platform and nothing otherwise, for when differences in GLSL aren't easily
// fixable by preprocessor. See glsl/prelude*.glsl for details.
//---

#pragma once
#include <azur/defs.h>

#ifdef AZUR_GRAPHICS_OPENGL_3_3
# include <GL/gl3w.h>
# include <SDL2/SDL_opengl.h>
#endif /* OpenGL 3.3 */

#ifdef AZUR_GRAPHICS_OPENGL_ES_2_0
# define GL_GLEXT_PROTOTYPES
# include <SDL2/SDL_opengles2.h>

/* We expose vertex array objects through OES_vertex_array_object. */
# define glGenVertexArrays glGenVertexArraysOES
# define glBindVertexArray glBindVertexArrayOES
# define glDeleteVertexArrays glDeleteVertexArraysOES
# define glIsVertexArray glIsVertexArrayOES
#endif /* OpenGL ES 2.0 */

namespace azur::gl {

/* Returns a string description of an OpenGL error code. The return value if
   the GLenum is not recognized is the address of a static buffer, which can
   be modified by further calls to the function. */
char const *errorString(GLenum errorCode);

/* Loads the file at `path` and compiles it as a shader of the specified type.
   Returns the new shader's ID, or 0 in case of error. Errors are logged. */
GLuint compileShaderFile(GLenum type, char const *path);

/* Compiles the provided code string as a shader of the specified type. If size
   is -1, `code` is assumed to be NUL-terminated and strlen(code) is used.
   Returns the shader ID, 0 in case of error. Errors are logged. */
GLuint compileShaderSource(GLenum type, char const *code, ssize_t size);

/* Link a program. This function takes a 0-terminated list of shaders IDs and
   links them into a new program. Returns the new program's ID, or 0 in case
   of error. Errors are logged. */
GLuint linkProgram(GLuint shader_1, ... /* 0-terminated */);

/* Loads a program from shader source files. The arguments come in pairs of
   type and file path until a 0-type terminates the list. Each file is loaded
   and compiled with compileShaderFile(), then the whole program is linked
   with linkProgram(). Returns the program ID, or 0 in case of error. */
GLuint loadProgramFiles(
   GLenum type_1, char const *path_1,
   ... /* Pairs repeat until 0-terminated */);

/* Analoguous to loadProgramFiles(), but takes string inputs and compiles them
   with loadShaderSource() with size=-1. */
GLuint loadProgramSources(
  GLenum type_1, char const *code_1,
  ... /* Pairs repeat until 0-terminated */);

} /* namespace azur::gl */
