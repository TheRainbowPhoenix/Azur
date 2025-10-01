//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// azur.opengl: Wrappers and direct access for OpenGL APIs
// Docs: @opengl
//---

#pragma once
#include <azur/defs.h>
#include <glm/glm.hpp>
#include <string.h>
#include <memory>
#include <string_view>

#if AZUR_GRAPHICS_OPENGL_3_3
# include <GL/gl3w.h>
# include <SDL2/SDL_opengl.h>
#endif /* OpenGL 3.3 */

#if AZUR_GRAPHICS_OPENGL_ES_3_0
# include <GLES3/gl3.h>
#endif /* OpenGL ES 3.0 */

namespace azur::gl {

//=== General utilities ======================================================//

/* An integer handle to an OpenGL resource that zeros itself when moved or
   deleted. This object doesn't make any OpenGL resource management calls; it's
   just here to disable copy and enable move on other classes. It has the same
   representation as the underlying type. */
template<typename T, T zero = 0>
struct Resource
{
    T value;

    Resource(T v): value {v} {}
    Resource(Resource<T> const &) = delete;
    Resource(Resource<T> &&other) { value = other.value; other.value = zero; }
    ~Resource() { value = zero; }

    Resource<T> &operator =(T v) { value = v; return *this; }
    operator T() const { return value; }

    T *get() { return &value; }
    T const *get() const { return &value; }
};


//=== Shaders ================================================================//

class ProgramSource;

/* A shading program (all pipeline stages) */
class Program
{
public:
    /* The default state is an empty program with no code and no settings, that
       holds no actual OpenGL state. */
    Program() = default;
    ~Program() { reset(); }

    /* Reset to the default state. This frees all OpenGL state. Subclass
       implementations should end by calling the base class implementation. */
    virtual void reset();

    //=== Compilation ========================================================//

    /* Set the program's name for logging. */
    void setName(string programName);

    /* Add the Azur prelude for this shader type. */
    bool addPrelude(GLuint type);
    /* Add a piece of source code taken from a file or resource. */
    bool addSourceFile(GLuint type, string const &path, bool linedir=true);
    /* Add a piece of source given directly as a string. */
    void addSourceText(GLuint type, std::string_view code, string origin);

    /* Compile all provided code. */
    bool compile();
    /* Link compiled code (after binding attribute locations). */
    bool link();
    /* Check whether the program has been successfully linked. */
    bool isLinked() const { return (m_prog != 0) && m_linked; }

    //=== Managing attribute locations =======================================//
    // TODO: Bind vertex attributes based on a reflected type.

    /* Get an attribute's location. This is cached. */
    GLuint getAttributeLocation(char const *name);

    /* Meta-programming trick to get the offset of a field within a structure.
       Should only be used with POD VertexAttr types. */
    template<typename Vertex, typename U>
    constexpr void *offsetOf(U Vertex::*x) {
        Vertex t {};
        return (void *)((char *)&(t.*x) - (char *)&t);
    }

    /* Bind a Vertex field of arbitrary type U to a vertex attribute using
       glVertexAttribPointer(). */
    template<typename Vertex, typename U>
    void bindVertexAttributeFP(U Vertex::*x, GLint size, GLenum type,
        GLboolean normalized, GLuint id) {
        glEnableVertexAttribArray(id);
        glVertexAttribPointer(id, size, type, normalized, sizeof(Vertex),
            offsetOf(x));
    }

    /* Same for integer attributes, using glVertexAttribIPointer(). */
    template<typename Vertex, typename U>
    void bindVertexAttributeInt(
        U Vertex::*x, GLint size, GLenum type, GLuint id) {
        glEnableVertexAttribArray(id);
        glVertexAttribIPointer(id, size, type, sizeof(Vertex), offsetOf(x));
    }

    /* Shortcuts for binding attributes of common types, which automatically
       provide the type details. */
    template<typename Vertex>
    void bindVertexAttribute(float Vertex::*x, GLuint id);
    template<typename Vertex>
    void bindVertexAttribute(glm::vec2 Vertex::*x, GLuint id);
    template<typename Vertex>
    void bindVertexAttribute(glm::vec3 Vertex::*x, GLuint id);
    template<typename Vertex>
    void bindVertexAttribute(glm::vec4 Vertex::*x, GLuint id);
    template<typename Vertex>
    void bindVertexAttribute(u8 Vertex::*x, GLuint id);
    template<typename Vertex>
    void bindVertexAttribute(int Vertex::*x, GLuint id);
    template<typename Vertex>
    void bindVertexAttribute(glm::ivec2 Vertex::*x, GLuint id);
    template<typename Vertex>
    void bindVertexAttribute(glm::ivec3 Vertex::*x, GLuint id);
    template<typename Vertex>
    void bindVertexAttribute(glm::ivec4 Vertex::*x, GLuint id);
    template<typename Vertex>
    void bindVertexAttribute(glm::uvec2 Vertex::*x, GLuint id);
    template<typename Vertex>
    void bindVertexAttribute(glm::uvec3 Vertex::*x, GLuint id);
    template<typename Vertex>
    void bindVertexAttribute(glm::uvec4 Vertex::*x, GLuint id);

    //=== Initialization =====================================================//

    /* Initialize the shader. This must be called after linking but before
       using any other shader functions. Subclasses can override this to create
       buffers, load external data, etc. that the shader might need. */
    virtual bool init() { return true; }

    //=== Binding and using the program ======================================//

    /* Switch to this program with glUseProgram(). */
    virtual void useProgram();

    /* Set uniforms based on uniform location. This doesn't set the current
       program object! */
    void setUniform(GLuint location, float f);
    void setUniform(GLuint location, float f1, float f2);
    void setUniform(GLuint location, float f1, float f2, float f3);
    void setUniform(GLuint location, glm::vec2 const &v2);
    void setUniform(GLuint location, glm::vec3 const &v3);
    void setUniform(GLuint location, glm::vec4 const &v4);
    void setUniform(GLuint location, glm::mat2 const &m2);
    void setUniform(GLuint location, glm::mat3 const &m3);
    void setUniform(GLuint location, glm::mat4 const &m4);
    void setUniform(GLuint location, int i);

    /* Same based on the uniform value's name. */
    template<typename T>
    void setUniform(char const *name, T &&uniform) {
        setUniform(glGetUniformLocation(m_prog, name),
                   std::forward<T>(uniform));
    }

protected:
    /* Program ID */
    Resource<GLuint> m_prog = 0;

private:
    /* Program name (for logging/pretty printing purposes) */
    string m_programName = "<unnamed>";

    /* Map from program type (vertex/etc/fragment shader) to code during the
       construction phase. Cleared after compiling. ProgramSource is hidden in
       the implementation but we must still provide a deleter for the Program
       destructor (which uses map's, thus unique_ptr's) to be defined. */
    struct ProgramSourceDeleter { void operator()(ProgramSource *) const; };
    map<GLuint, std::unique_ptr<ProgramSource, ProgramSourceDeleter>> m_sources;
    /* Get the program source object for a given type. */
    ProgramSource &getProgramSource(GLuint type);

    /* List of shader descriptors between compiling and linking. */
    vector<GLuint> m_shaders;

    /* Whether the program has been successfully linked. */
    bool m_linked = false;
    /* Map of attribute names to shader locations. */
    std::map<std::string, GLuint> m_attributes;
};

template<typename Vertex>
void Program::bindVertexAttribute(float Vertex::*x, GLuint id) {
    bindVertexAttributeFP(x, 1, GL_FLOAT, GL_FALSE, id);
}

template<typename Vertex>
void Program::bindVertexAttribute(glm::vec2 Vertex::*x, GLuint id) {
    bindVertexAttributeFP(x, 2, GL_FLOAT, GL_FALSE, id);
}

template<typename Vertex>
void Program::bindVertexAttribute(glm::vec3 Vertex::*x, GLuint id) {
    bindVertexAttributeFP(x, 3, GL_FLOAT, GL_FALSE, id);
}

template<typename Vertex>
void Program::bindVertexAttribute(glm::vec4 Vertex::*x, GLuint id) {
    bindVertexAttributeFP(x, 4, GL_FLOAT, GL_FALSE, id);
}

template<typename Vertex>
void Program::bindVertexAttribute(u8 Vertex::*x, GLuint id) {
    bindVertexAttributeFP(x, 1, GL_UNSIGNED_BYTE, GL_FALSE, id);
}

template<typename Vertex>
void Program::bindVertexAttribute(int Vertex::*x, GLuint id) {
    bindVertexAttributeInt(x, 1, GL_INT, id);
}

template<typename Vertex>
void Program::bindVertexAttribute(glm::ivec2 Vertex::*x, GLuint id) {
    bindVertexAttributeInt(x, 2, GL_INT, id);
}

template<typename Vertex>
void Program::bindVertexAttribute(glm::ivec3 Vertex::*x, GLuint id) {
    bindVertexAttributeInt(x, 3, GL_INT, id);
}

template<typename Vertex>
void Program::bindVertexAttribute(glm::ivec4 Vertex::*x, GLuint id) {
    bindVertexAttributeInt(x, 4, GL_INT, id);
}

template<typename Vertex>
void Program::bindVertexAttribute(glm::uvec2 Vertex::*x, GLuint id) {
    bindVertexAttributeInt(x, 2, GL_UNSIGNED_INT, id);
}

template<typename Vertex>
void Program::bindVertexAttribute(glm::uvec3 Vertex::*x, GLuint id) {
    bindVertexAttributeInt(x, 3, GL_UNSIGNED_INT, id);
}

template<typename Vertex>
void Program::bindVertexAttribute(glm::uvec4 Vertex::*x, GLuint id) {
    bindVertexAttributeInt(x, 4, GL_UNSIGNED_INT, id);
}

} /* namespace azur::gl */
