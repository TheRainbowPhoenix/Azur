//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// azur.gl.shaders: Abstraction of OpenGL 3.3 program objects
//
// This header provides the ShaderProgram class as an abstraction for OpenGL
// shaders. Each shader comes with its own uniform parameters, vertex
// attributes, textures, and potentially other settings; ie., it encapsulates
// all of the OpenGL state needed to properly run the program.
//
// ShaderProgram provides the generic mechanisms for using a shader (ie. it has
// all the downwards-facing interfaces), but it only exposes a low-level vertex
// buffer for describing draw commands. Hence, the intended way to use it is to
// subclass it for each shader, and add shader-specific high-level drawing
// functions to the derived class.
// TODO: Mention how drawing functions add new vertices to the VAO
// Access to low-level OpenGL components is provided in case advanced GL things
// are needed, but that shouldn't be the case for basic use.
//
// The initialization of a ShaderProgram goes as follows:
// 1. Create a ShaderProgram object (directly or through a derived class);
// 2. Load code with one or more calls to addSourceFile(), addSourceText();
// 3. Compile the program with compile().
//
// At this stage, if no error occurred the shader has source code and its
// static configuration can be set:
// 4. Specify the attribute format (see below).
// 5. Load relevant textures or state that are the same for all frames.
//
// Once that's done, the shader can be used in rendering frames:
// 6. Set up frame-specific uniforms, resources, etc.
// 7. Queue draw calls.
//
// TODO: Provide a ShaderPipeline for automatically switching between programs
// and checking whether the flow is correct.
//
// The attribute format describes per-vertex data in the form of a structure
// layout, and is basically a meta-level reflection of the structure type used
// for VertexAttr. It tells OpenGL how to read the raw vertex data buffer.
//---

#pragma once
#include <azur/gl/gl.h>
#include <glm/glm.hpp>
#include <string.h>
#include <string>
#include <vector>
#include <map>

namespace azur::gl {

template<typename VertexAttr>
class ShaderProgram
{
public:
    /* Create an empty shader program with no code and no settings. This is an
       inert call which does nothing, and especially no OpenGL calls. */
    ShaderProgram() = default;
    ~ShaderProgram();

    /* OpenGL objects cannot be copied, and we don't care about moving yet. */
    ShaderProgram(ShaderProgram const &) = delete;
    ShaderProgram(ShaderProgram &&) = delete;

    /*** Initialization ***/

    /* Initialize the shader by allocating some OpenGL objects. */
    void init();

    /* Add a piece of source code taken from a file. */
    bool addSourceFile(GLuint type, std::string const &file);
    /* Add a piece of source given directly as a string. */
    void addSourceText(GLuint type, std::string const &code)
    {
        m_code[type] += code;
    }
    void addSourceText(GLuint type, char const *code, ssize_t size = -1)
    {
        m_code[type] += std::string {code, size >= 0 ? size : strlen(code)};
    }

    /* Compile all provided code. */
    bool compile();

    /* Check whether the program has been compiled successfully. */
    bool isCompiled() const
    {
        return (m_prog != 0);
    }

    /*** Configuration ***/

    /* Meta-programming trick to get the offset of a field within a structure.
       Should only be used with POD VertexAttr types. */
    template<typename U>
    constexpr void *offsetOf(U VertexAttr::*x) {
        VertexAttr t {};
        return (void *)((char *)&(t.*x) - (char *)&t);
    }

    /* Bind a VertexAttr field of arbitrary type U to a vertex attribute using
       glVertexAttribPointer(). */
    template<typename U>
    void bindVertexAttributeFP(U VertexAttr::*x, GLint size, GLenum type,
        GLboolean normalized, char const *name) {
        glVertexAttribPointer(
            // TODO: Cache glGetAttribLocation()
            glGetAttribLocation(m_prog, name),
            size, type, normalized, sizeof(VertexAttr), offsetOf(x));
    }

    /* Same for integer attributes, using glVertexAttribIPointer(). */
    template<typename U>
    void bindVertexAttributeInt(
        U VertexAttr::*x, GLint size, GLenum type, char const *name) {
        glVertexAttribIPointer(
            // TODO: Cache glGetAttribLocation()
            glGetAttribLocation(m_prog, name),
            size, type, sizeof(VertexAttr), offsetOf(x));
    }

    /* Shortcuts for binding attributes of common types, which automatically
       provide the type details. */
    void bindVertexAttribute(float     VertexAttr::*x, char const *name);
    void bindVertexAttribute(glm::vec2 VertexAttr::*x, char const *name);
    void bindVertexAttribute(glm::vec3 VertexAttr::*x, char const *name);
    void bindVertexAttribute(glm::vec4 VertexAttr::*x, char const *name);

    /* Set uniforms. */
    void setUniform(char const *name, float f);
    void setUniform(char const *name, float f1, float f2);
    void setUniform(char const *name, float f1, float f2, float f3);
    void setUniform(char const *name, glm::vec2 const &v2);
    void setUniform(char const *name, glm::vec3 const &v3);
    void setUniform(char const *name, glm::vec4 const &v4);
    void setUniform(char const *name, glm::mat2 const &m2);
    void setUniform(char const *name, glm::mat3 const &m3);
    void setUniform(char const *name, glm::mat4 const &m4);

    /*** Generating draw commands ***/

    /* Add a vertex to the list of vertices for this frame. */
    void addVertex(VertexAttr &&attr)
    {
        m_vertices.push_pack(std::move(attr));
    }

    /* Add three new vertices (makes a triangle in GL_TRIANGLES mode). */
    void addVertexTriangle(
        VertexAttr const &a1, VertexAttr const &a2, VertexAttr const &a3)
    {
        m_vertices.push_back(a1);
        m_vertices.push_back(a2);
        m_vertices.push_back(a3);
    }

    /* Add two side-sharing triangles (makes a quad in GL_TRIANGLES mode).
       VertexAttr must be copyable. */
    void addVertexQuad(VertexAttr const &a1, VertexAttr const &a2,
                       VertexAttr const &a3, VertexAttr const &a4)
    {
        m_vertices.push_back(a1);
        m_vertices.push_back(a2);
        m_vertices.push_back(a3);
        m_vertices.push_back(a2);
        m_vertices.push_back(a3);
        m_vertices.push_back(a4);
    }

    /* Direct access to vertex list, to facilitate faster methods of building a
       vertex list for any given frame. */
    std::vector<VertexAttr> &vertices()
    {
        return m_vertices;
    }

    /* Queue a draw call. Indices should be within the range of vertices pushed
       to the VAO at the time of the call. Info for this call is kept in memory
       until the rendering phase, so the VAO and the order should be valid
       until the end of the frame.

       With queueDrawArrays, the vertices at positions [first .. first+count)
       of the VBO are used to render. With queueDrawElements, the vertices at
       positions ind[0], .., ind[count-1] are used. Depending on mode, these
       are combined in pairs, triples, or other ways to render primitives. */
    void queueDrawArrays(GLenum mode, GLint first, GLsizei count);
    void queueDrawElements(GLenum mode, GLsizei count, uint8_t const *ind);
    void queueDrawElements(GLenum mode, GLsizei count, uint16_t const *ind);
    void queueDrawElements(GLenum mode, GLsizei count, uint32_t const *ind);

    /*** Executing commands ***/

    /* Load buffer data; this is usually executed once at the beginning of the
       frame. (This is normally called by ShaderPipeline::render().) */
    void loadBuffer();

    /* Switch to this shader program. This should be called before executing a
       draw command if the previous shader to run was different. (This is
       normally called by ShaderPipeline::render().) */
    void useProgram();


protected:
    /* Program ID */
    GLuint m_prog = 0;
    /* Vertex Array Object and Vertex Buffer Object with parameters */
    GLuint m_vao = 0, m_vbo = 0;
    /* Size of the VBO on the GPU */
    size_t m_vboSize = 0;

private:
    /* Map from program type (vertex/etc/fragment shader) to code during the
       construction phase. */
    std::map<GLuint, std::string> m_code;
    /* List of vertices during rendering. */
    std::vector<VertexAttr> m_vertices;
};

template<typename T>
void ShaderProgram<T>::init()
{
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
}

template<typename T>
ShaderProgram<T>::~ShaderProgram()
{
    glDeleteProgram(m_prog);
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

template<typename T>
bool ShaderProgram<T>::addSourceFile(
    GLuint type, std::string const &file)
{
    extern char *load_file(char const *, size_t *);
    char *data = load_file(file.c_str(), nullptr);
    if(!data)
        return false;

    m_code[type] += data;
    delete[] data;
    return true;
}

template<typename T>
bool ShaderProgram<T>::compile()
{
    /* List of shader descriptors obtained from OpenGL. */
    std::vector<GLuint> m_shaders;
    bool success = true;

    for(auto const &[type, code]: m_code) {
        GLuint id = compileShaderSource(type, code.c_str(), -1);
        if(id != 0)
            m_shaders.push_back(id);
        else
            success = false;
    }

    // TODO: No link error detection?
    m_prog = success ? link(m_shaders.data(), m_shaders.size()) : 0;

    for(auto id: m_shaders)
        glDeleteShader(id);

    return success;
}

/* TODO: Complete state associated with each program:
   1. Uniforms (glUniform*)
   2. Vertex array (glBindVertexArray)
   3. Textures (glActiveTexture, glBindTexture)
   4. Other state (glEnable, glDisable, glBlendFunc)
   Render with glDrawArrays or glDrawElements.

   TODO: Can we minimize switching (eg. texture switching)? */

template<typename T>
void ShaderProgram<T>::bindVertexAttribute(float T::*x, char const *name)
{
    bindVertexAttributeFP(x, 1, GL_FLOAT, GL_FALSE, name);
}

template<typename T>
void ShaderProgram<T>::bindVertexAttribute(glm::vec2 T::*x, char const *name)
{
    bindVertexAttributeFP(x, 2, GL_FLOAT, GL_FALSE, name);
}

template<typename T>
void ShaderProgram<T>::bindVertexAttribute(glm::vec3 T::*x, char const *name)
{
    bindVertexAttributeFP(x, 3, GL_FLOAT, GL_FALSE, name);
}

template<typename T>
void ShaderProgram<T>::bindVertexAttribute(glm::vec4 T::*x, char const *name)
{
    bindVertexAttributeFP(x, 4, GL_FLOAT, GL_FALSE, name);
}

template<typename T>
void ShaderProgram<T>::setUniform(char const *name, float f)
{
    glUniform1f(glGetUniformLocation(m_prog, name), f);
}

template<typename T>
void ShaderProgram<T>::setUniform(char const *name, float f1, float f2)
{
    glUniform2f(glGetUniformLocation(m_prog, name), f1, f2);
}

template<typename T>
void ShaderProgram<T>::setUniform(
    char const *name, float f1, float f2, float f3)
{
    glUniform3f(glGetUniformLocation(m_prog, name), f1, f2, f3);
}

template<typename T>
void ShaderProgram<T>::setUniform(char const *name, glm::vec2 const &v2)
{
    glUniform2fv(glGetUniformLocation(m_prog, name), 1, &v2[0]);
}

template<typename T>
void ShaderProgram<T>::setUniform(char const *name, glm::vec3 const &v3)
{
    glUniform3fv(glGetUniformLocation(m_prog, name), 1, &v3[0]);
}

template<typename T>
void ShaderProgram<T>::setUniform(char const *name, glm::vec4 const &v4)
{
    glUniform4fv(glGetUniformLocation(m_prog, name), 1, &v4[0]);
}

template<typename T>
void ShaderProgram<T>::setUniform(char const *name, glm::mat2 const &m2)
{
    glUniformMatrix2fv(
        glGetUniformLocation(m_prog, name), 1, GL_FALSE, &m2[0][0]);
}

template<typename T>
void ShaderProgram<T>::setUniform(char const *name, glm::mat3 const &m3)
{
    glUniformMatrix3fv(
        glGetUniformLocation(m_prog, name), 1, GL_FALSE, &m3[0][0]);
}

template<typename T>
void ShaderProgram<T>::setUniform(char const *name, glm::mat4 const &m4)
{
    glUniformMatrix4fv(
        glGetUniformLocation(m_prog, name), 1, GL_FALSE, &m4[0][0]);
}

template<typename T>
void ShaderProgram<T>::loadBuffer()
{
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    /* If the size of the VBO is too small or much larger than needed, resize
       it; otherwise, simply swap the data without reallocating */
    if(m_vboSize < m_vertices.size() || m_vboSize > m_vertices.size() * 4) {
        glBufferData(GL_ARRAY_BUFFER, sizeof(T) * m_vertices.size(),
            m_vertices.data(), GL_DYNAMIC_DRAW);
        m_vboSize = m_vertices.size();
    }
    else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(T) * m_vertices.size(),
            m_vertices.data());
    }
}

template<typename T>
void ShaderProgram<T>::useProgram()
{
    glUseProgram(m_prog);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
}

} /* namespace azur::gl */
