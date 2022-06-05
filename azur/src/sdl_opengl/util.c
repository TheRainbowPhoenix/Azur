#include <azur/sdl_opengl/gl.h>
#include <azur/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/* Read the full contents of a file into the heap. Returns an malloc'd pointer
   on success, NULL if an error occurs. */
static char *load_file(char const *path, size_t *out_size)
{
    char *contents = NULL;
    long size = 0;

    FILE *fp = fopen(path, "r");
    if(!fp) goto load_file_end;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    contents = malloc(size + 1);
    if(!contents) goto load_file_end;

    if(out_size) *out_size = size;
    fread(contents, size, 1, fp);
    contents[size] = 0;

load_file_end:
    if(fp) fclose(fp);
    return contents;
}

char const *azgl_error(GLenum ec)
{
    static char str[32];

    switch(ec) {
    case GL_NO_ERROR:
        return "Success";
    case GL_INVALID_ENUM:
        return "Invalid enum";
    case GL_INVALID_VALUE:
        return "Invalid value";
    case GL_INVALID_OPERATION:
        return "Invalid operation";
    case GL_OUT_OF_MEMORY:
        return "Out of memory";
    }

    snprintf(str, 32, "<GLenum %#x>", ec);
    return str;
}

/* Common code for the shader compiling functions. */
static GLuint azgl_compile_shader(GLenum type, char const *code, ssize_t size,
    char const *origin)
{
    /* Shader prelude; this gives shader version, some macro definitions and
       some "selectors" which altogether provide some degree of compatibility
       between GLSL and GLSL ES. */
    static char const *vs_prelude = NULL;
    static char const *fs_prelude = NULL;

    if(!vs_prelude) {
        #if defined AZUR_GRAPHICS_OPENGL_ES_2_0
        extern char const *azur_glsl__vs_prelude_gles2;
        vs_prelude = azur_glsl__vs_prelude_gles2;
        #elif defined AZUR_GRAPHICS_OPENGL_3_3
        extern char const *azur_glsl__vs_prelude_gl3;
        vs_prelude = azur_glsl__vs_prelude_gl3;
        #endif
    }
    if(!fs_prelude) {
        #if defined AZUR_GRAPHICS_OPENGL_ES_2_0
        extern char const *azur_glsl__fs_prelude_gles2;
        fs_prelude = azur_glsl__fs_prelude_gles2;
        #elif defined AZUR_GRAPHICS_OPENGL_3_3
        extern char const *azur_glsl__fs_prelude_gl3;
        fs_prelude = azur_glsl__fs_prelude_gl3;
        #endif
    }

    char const *prelude = NULL;
    if(type == GL_VERTEX_SHADER)
        prelude = vs_prelude;
    else if(type == GL_FRAGMENT_SHADER)
        prelude = fs_prelude;
    if(prelude == NULL)
        prelude = "";

    GLuint id = glCreateShader(type);
    if(id == 0) {
        azlog(ERROR, "glCreateShader failed\n");
        return 0;
    }

    GLint rc = GL_FALSE;
    GLsizei log_length = 0;

    if(size < 0)
        size = (int)strlen(code);

    char const *string_array[] = { prelude, code };
    GLint size_array[] = { strlen(prelude), size };

    azlog(INFO, "Compiling shader: %s\n", origin);
    glShaderSource(id, 2, string_array, size_array);
    glCompileShader(id);

    glGetShaderiv(id, GL_COMPILE_STATUS, &rc);
    if(rc == GL_FALSE)
        azlog(ERROR, "compilation failed!\n");

    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
    if(log_length > 0) {
        GLchar *log = malloc((log_length + 1) * sizeof *log);
        glGetShaderInfoLog(id, log_length, &log_length, log);
        if(log_length > 0) {
            azlogc(ERROR, "%s", log);
            if(log[log_length - 1] != '\n')
                azlogc(ERROR, "\n");
        }
        free(log);
    }

    return id;
}

GLuint azgl_compile_shader_file(GLenum type, char const *path)
{
    size_t size;
    char *source = load_file(path, &size);
    if(!source) {
        azlog(ERROR, "Cannot read '%s': %s\n", path, strerror(errno));
        return 0;
    }

    GLuint id = azgl_compile_shader(type, source, size, path);
    free(source);
    return id;
}

GLuint azgl_compile_shader_source(GLenum type, char const *code, ssize_t size)
{
    return azgl_compile_shader(type, code, size, "<inline>");
}

static GLuint azgl_link(GLuint *shaders, int count)
{
    GLuint prog = glCreateProgram();
    if(prog == 0) {
        azlog(ERROR, "glCreateProgram failed\n");
        return 0;
    }

    /* Attach all shaders */
    for(int i = 0; i < count; i++)
        glAttachShader(prog, shaders[i]);

    GLint rc = GL_FALSE;
    GLsizei log_length = 0;

    azlog(INFO, "Linking program\n");
    glLinkProgram(prog);

    glGetProgramiv(prog, GL_LINK_STATUS, &rc);
    if(rc == GL_FALSE)
        azlog(ERROR, "link failed!\n");

    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_length);
    if(log_length > 0) {
        GLchar *log = malloc((log_length + 1) * sizeof *log);
        glGetProgramInfoLog(prog, log_length, &log_length, log);
        if(log_length > 0) {
            azlogc(ERROR, "%s", log);
            if(log[log_length - 1] != '\n')
                azlogc(ERROR, "\n");
        }
        free(log);
    }

    /* Detach all shaders */
    for(int i = 0; i < count; i++)
        glDetachShader(prog, shaders[i]);

    return prog;
}

GLuint azgl_link_program(GLuint shader_1, ... /* 0-terminated */)
{
    va_list args;
    va_start(args, shader_1);

    GLuint shaders[32];
    int count = 0;

    do {
        shaders[count++] = shader_1;
        shader_1 = va_arg(args, GLuint);
    }
    while(count < 32 && shader_1 != 0);
    va_end(args);

    return azgl_link(shaders, count);
}

static GLuint azgl_load_program(bool is_file, GLenum type, char const *input,
    va_list *args)
{
    GLuint shaders[32];
    int count = 0;

    do {
        shaders[count++] = is_file
            ? azgl_compile_shader_file(type, input)
            : azgl_compile_shader_source(type, input, -1);
        type = va_arg(*args, GLenum);
        input = va_arg(*args, char const *);
    }
    while(count < 32 && type != 0);

    GLuint prog = azgl_link(shaders, count);

    for(int i = 0; i < count; i++)
        glDeleteShader(shaders[i]);

    return prog;
}

GLuint azgl_load_program_file(GLenum type, char const *path, ...)
{
    va_list args;
    va_start(args, path);
    GLuint prog = azgl_load_program(true, type, path, &args);
    va_end(args);
    return prog;
}

GLuint azgl_load_program_source(GLenum type, char const *code, ...)
{
    va_list args;
    va_start(args, code);
    GLuint prog = azgl_load_program(false, type, code, &args);
    va_end(args);
    return prog;
}
