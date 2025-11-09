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


//=== Textures (GPU-side) and surfaces (CPU-side) ============================//

/* OpenGL texture format. This defines the way texture data is stored in the
   GPU memory. The set of formats is enumerated, and this structure tracks
   information about each of them.

   Of particular interest is how can image data can be loaded into the texture.
   Only data with the right number of channels is allowed, and there might be
   multiple allowed pixel value representations, though often just one. */
struct TextureFormat
{
    /* Internal storage format for the texture. */
    GLuint internalFormat;
    /* Number of channels */
    uint channelCount() const;

    /* Single allowed input image format for loading into the texture. */
    GLuint allowedFormat;
    /* Valid pixel value types. */
    uint allowedTypeCount() const;
    GLuint allowedTypes[3];
};

TextureFormat const*getTextureFormatInfo(GLuint internalFormat);

uint channelsInImageFormat(GLuint format);
uint bytesInImageType(GLuint type);

class Surface
{
public:
    /* The default state is no format, no type, no data and 0/0 size. */
    Surface() = default;
    ~Surface() { reset(); }
    /* Reset to the default state and free the data if owned. */
    // TODO: Surface: data should have a proper destructor: not always free()!
    void reset();

    //=== Constructing surfaces ==============================================//

    static Surface *loadFromImageData(void const *buf, size_t size);
    static Surface *loadFromImageFile(char const *path);
    static Surface *loadFromResource(char const *resource);

    //=== Access to surface metadata and data ================================//

    /* Access format and dimensions */
    GLuint format() const { return m_format; }
    GLuint type() const { return m_type; }
    uint width() const { return m_width; }
    uint height() const { return m_height; }
    /* Distance between two rows in storage, expressed in pixels */
    uint stride() const { return m_stride; }
    /* Number of channels (stored interleaved for each pixel) */
    uint channelCount() const;
    /* Number of bytes per channel value */
    int bytesPerValue() const;
    /* Access to raw data */
    void *data() { return m_data; }
    void const *data() const { return m_data; }

    // TODO: Surface: sub-surface referencing

    //=== CPU rendering on surfaces ==========================================//
    // TODO: Basic Surface drawing API for experimenting and fallbacks

private:
    static Surface *loadFromSTBi(
        u8 *data, int width, int height, int n, bool integer=false);

     GLuint m_format = 0;
     GLuint m_type = 0;

     uint m_width = 0;
     uint m_height = 0;
     uint m_stride = 0;

     void *m_data = nullptr;
     bool m_ownsData = false;

};

class Texture
{
public:
    /* The default state is no texture and an empty name. */
    Texture(GLuint target) { m_target = target; }
    ~Texture() { reset(); }

    /* Reset to the default state. This frees all OpenGL state and all
       remembered texture data. */
    virtual void reset();

    /* Generate a new fresh name for this texture
       TODO: Find a way to benefit from glGenTextures(). Ideally this should
             be paired with a glDeleteTextures() in destructor (trickier). */
    void generateName();
    /* Whether this holds a valid, initialized texture. */
    bool isValid() const { return m_name != 0; }

    /* Bind the texture to the associated target in the *current* active
       texture unit. The caller is responsible for switching texture units! */
    void bind() const;

    /* Get the internal storage format */
    GLuint format() const { return m_internalFormat; }
    /* Get the texture target (type of texture) */
    GLuint target() const { return m_target; }

    /* Dimensions */
    uint width() const { return m_size.x; }
    uint height() const { return m_size.y; }
    uint depth() const { return m_size.z; }
    glm::ivec3 size() const { return m_size; }
    /* Size of the internal GPU storage */
    glm::ivec3 storageSize() const { return m_allocSize; }

    /* [When bound]
       Set the texture's swizzle mask. */
    void setSwizzleMask(GLint r, GLint g, GLint b, GLint a);

    /* Check whether a given format/type input image can be loaded into this
       texture. The format describes color components (`GL_RED`, `GL_RGB`,
       `GL_ALPHA`, etc), and there'll only ever be one that's compatible with
       the texture's internal storage format. The type is usually the type of
       a single channel value (`GL_UNSIGNED_BYTE`, `GL_FLOAT`) except for non-
       conventional arrangements setups were the value describes all channels
       at once (`GL_UNSIGNED_SHORT_5_6_5`). If `error` is true, an error
       message will be printed in case of mismatch. */
    bool isCompatibleWith(GLuint format, GLuint type, bool error=false);

protected:
    GLuint m_target;
    Resource<GLuint> m_name = 0;

    /* Internal storage format for texture pixels */
    GLuint m_internalFormat = 0;
    /* Dimensions of the data */
    glm::ivec3 m_size {0, 0, 0};
    /* Dimensions of the allocation (might be rounded up to a power of 2) */
    glm::ivec3 m_allocSize {0, 0, 0};

    /* Next power of two (going up). 0 maps to 0. */
    static uint nextPowerOfTwo(uint n);
    /* Conditionally round to powers of two on all three axes. */
    static glm::ivec3 roundSize(
        glm::ivec3 size, bool roundX, bool roundY, bool roundZ);
    /* Returns the "natural" (non-converting) input type for a given internal
       format. This is useful in calls to texImage* with NULL data, as the API
       for some reason requires a valid type even though there's no input. */
    static GLuint naturalInputTypeFor(GLuint internalFormat);
    /* Returns the only valid input format for a given internal format. */
    static GLuint inputFormatFor(GLuint internalFormat);
};

struct Texture2D: Texture
{
    Texture2D(): Texture(GL_TEXTURE_2D) {}

    /* [When bound]
       Set the texture's format and size, reallocating GPU memory as needed.
       `internalFormat` specifies how pixels are represented in the GPU memory.
       This function is idempotent. If `round` is set (enabled by default), the
       size is rounded up to a power of 2, which is usually faster and can help
       reducing the number of allocations. */
    void setFormat(
        GLuint internalFormat, uint width, uint height, bool round=true);

    // TODO: Texture load functions: account for stride

    /* [When bound]
       Load data into the texture. The input is specified by 4 parameters:
       * `data` is the pointer to raw pixel data;
       * `format` specifies the set of channels for each pixel;
       * `type` the value type for each channel for each pixel (or in the case
         of unusual formats the type of a pixel value containing all channels);
       * `stride` is the distance between two lines, in pixels.

       Where in the texture we load is given by the mipmap level and a
       sub-rectangle of the image.

       Fails and returns false if the format/type of the input isn't compatible
       with the texture format (with error log).
       TODO: Doesn't perform clipping yet! */
    bool loadData(
        void const *data, GLuint format, GLuint type, int stride,
        int level, rect<int> dstRect={});
    /* [When bound] Load data from a surface. */
    bool loadData(Surface const &S, int level, rect<int> dstRect={});
};

struct Texture2DArray: Texture
{
    Texture2DArray(): Texture(GL_TEXTURE_2D_ARRAY) {}
    void reset() override;

    /* [When bound]
       Set the texture's format and size. If powerOfTwo is set, the width and
       height are rounded up to a power of 2.*/
    void setFormat(
        GLuint internalFormat, uint width, uint height, uint layerCount,
        bool round=true);

    /* [When bound] Load data into a layer. */
    bool loadLayerData(
        void const *data, GLuint format, GLuint type, int stride,
        uint layer, int level, rect<int> dstRect={});
    /* [When bound] Load data into a layer from a surface. */
    bool loadLayerData(
        Surface const &S, uint layer, int level, rect<int> dstRect={});
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
