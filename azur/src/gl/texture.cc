//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//

#include <azur/opengl.h>
#include <azur/log.h>

namespace azur::gl {

//=== Encoding of texture format information =================================//

// Reference (similar for all versions):
// <https://registry.khronos.org/OpenGL-Refpages/es3.1/html/glTexImage2D.xhtml>
static struct TextureFormat const formats[] = {
    /* Sized texture formats for images */
    { GL_R8,             GL_RED,          {GL_UNSIGNED_BYTE} },
    { GL_R8_SNORM,       GL_RED,          {GL_BYTE} },
    { GL_R16F,           GL_RED,          {GL_HALF_FLOAT, GL_FLOAT} },
    { GL_R32F,           GL_RED,          {GL_FLOAT} },
    { GL_R8UI,           GL_RED_INTEGER,  {GL_UNSIGNED_BYTE} },
    { GL_R8I,            GL_RED_INTEGER,  {GL_BYTE} },
    { GL_R16UI,          GL_RED_INTEGER,  {GL_UNSIGNED_SHORT} },
    { GL_R16I,           GL_RED_INTEGER,  {GL_SHORT} },
    { GL_R32UI,          GL_RED_INTEGER,  {GL_UNSIGNED_INT} },
    { GL_R32I,           GL_RED_INTEGER,  {GL_INT} },
    { GL_RG8,            GL_RG,           {GL_UNSIGNED_BYTE} },
    { GL_RG8_SNORM,      GL_RG,           {GL_BYTE} },
    { GL_RG16F,          GL_RG,           {GL_HALF_FLOAT, GL_FLOAT} },
    { GL_RG32F,          GL_RG,           {GL_FLOAT} },
    { GL_RG8UI,          GL_RG_INTEGER,   {GL_UNSIGNED_BYTE} },
    { GL_RG8I,           GL_RG_INTEGER,   {GL_BYTE} },
    { GL_RG16UI,         GL_RG_INTEGER,   {GL_UNSIGNED_SHORT} },
    { GL_RG16I,          GL_RG_INTEGER,   {GL_SHORT} },
    { GL_RG32UI,         GL_RG_INTEGER,   {GL_UNSIGNED_INT} },
    { GL_RG32I,          GL_RG_INTEGER,   {GL_INT} },
    { GL_RGB8,           GL_RGB,          {GL_UNSIGNED_BYTE} },
    { GL_SRGB8,          GL_RGB,          {GL_UNSIGNED_BYTE} },
    { GL_RGB565,         GL_RGB,          {GL_UNSIGNED_BYTE,
                                           GL_UNSIGNED_SHORT_5_6_5} },
    { GL_RGB8_SNORM,     GL_RGB,          {GL_BYTE} },
    { GL_R11F_G11F_B10F, GL_RGB,          {GL_UNSIGNED_INT_10F_11F_11F_REV,
                                           GL_HALF_FLOAT, GL_FLOAT} },
    { GL_RGB9_E5,        GL_RGB,          {GL_UNSIGNED_INT_5_9_9_9_REV,
                                           GL_HALF_FLOAT, GL_FLOAT} },
    { GL_RGB16F,         GL_RGB,          {GL_HALF_FLOAT, GL_FLOAT} },
    { GL_RGB32F,         GL_RGB,          {GL_FLOAT} },
    { GL_RGB8UI,         GL_RGB_INTEGER,  {GL_UNSIGNED_BYTE} },
    { GL_RGB8I,          GL_RGB_INTEGER,  {GL_BYTE} },
    { GL_RGB16UI,        GL_RGB_INTEGER,  {GL_UNSIGNED_SHORT} },
    { GL_RGB16I,         GL_RGB_INTEGER,  {GL_SHORT} },
    { GL_RGB32UI,        GL_RGB_INTEGER,  {GL_UNSIGNED_INT} },
    { GL_RGB32I,         GL_RGB_INTEGER,  {GL_INT} },
    { GL_RGBA8,          GL_RGBA,         {GL_UNSIGNED_BYTE} },
    { GL_SRGB8_ALPHA8,   GL_RGBA,         {GL_UNSIGNED_BYTE} },
    { GL_RGBA8_SNORM,    GL_RGBA,         {GL_BYTE} },
    { GL_RGB5_A1,        GL_RGBA,         {GL_UNSIGNED_BYTE,
                                           GL_UNSIGNED_SHORT_5_5_5_1,
                                           GL_UNSIGNED_INT_2_10_10_10_REV} },
    { GL_RGBA4,          GL_RGBA,         {GL_UNSIGNED_BYTE,
                                           GL_UNSIGNED_SHORT_4_4_4_4} },
    { GL_RGB10_A2,       GL_RGBA,         {GL_UNSIGNED_INT_2_10_10_10_REV} },
    { GL_RGBA16F,        GL_RGBA,         {GL_HALF_FLOAT, GL_FLOAT} },
    { GL_RGBA32F,        GL_RGBA,         {GL_FLOAT} },
    { GL_RGBA8UI,        GL_RGBA_INTEGER, {GL_UNSIGNED_BYTE} },
    { GL_RGBA8I,         GL_RGBA_INTEGER, {GL_BYTE} },
    { GL_RGB10_A2UI,     GL_RGBA_INTEGER, {GL_UNSIGNED_INT_2_10_10_10_REV} },
    { GL_RGBA16UI,       GL_RGBA_INTEGER, {GL_UNSIGNED_SHORT} },
    { GL_RGBA16I,        GL_RGBA_INTEGER, {GL_SHORT} },
    { GL_RGBA32I,        GL_RGBA_INTEGER, {GL_INT} },
    { GL_RGBA32UI,       GL_RGBA_INTEGER, {GL_UNSIGNED_INT } },

    /* Sized texture formats for depth and stencil buffers */
    { GL_DEPTH_COMPONENT16,    GL_DEPTH_COMPONENT,  {GL_UNSIGNED_SHORT,
                                                     GL_UNSIGNED_INT} },
    { GL_DEPTH_COMPONENT24,    GL_DEPTH_COMPONENT,  {GL_UNSIGNED_INT} },
    { GL_DEPTH_COMPONENT32F,   GL_DEPTH_COMPONENT,  {GL_FLOAT} },
    { GL_DEPTH24_STENCIL8,     GL_DEPTH_STENCIL,    {GL_UNSIGNED_INT_24_8} },
    { GL_DEPTH32F_STENCIL8,    GL_DEPTH_STENCIL,
        {GL_FLOAT_32_UNSIGNED_INT_24_8_REV} },

    /* Unsized texture formats--not ideal! Leaves the GPU to do whatever */
    { GL_RGB,            GL_RGB,          {GL_UNSIGNED_BYTE,
                                           GL_UNSIGNED_SHORT_5_6_5} },
    { GL_RGBA,           GL_RGBA,         {GL_UNSIGNED_BYTE,
                                           GL_UNSIGNED_SHORT_4_4_4_4,
                                           GL_UNSIGNED_SHORT_5_5_5_1} },
    { GL_ALPHA,          GL_ALPHA,        {GL_UNSIGNED_BYTE} },
};

static std::map<GLuint, TextureFormat const *> formatsIndex;

TextureFormat const*getTextureFormatInfo(GLuint internalFormat)
{
    /* Lazily construct the index */
    if(!formatsIndex.size()) {
        uint n = sizeof formats / sizeof formats[0];
        for(uint i = 0; i < n; i++)
            formatsIndex[formats[i].internalFormat] = &formats[i];
    }

    auto it = formatsIndex.find(internalFormat);
    return (it != formatsIndex.end()) ? it->second : nullptr;
}

uint TextureFormat::channelCount() const
{
    return channelsInImageFormat(this->allowedFormat);
}

uint TextureFormat::allowedTypeCount() const
{
    GLuint const *T = this->allowedTypes;
    return (T[0] != 0) + (T[1] != 0) + (T[2] != 0);
}

uint channelsInImageFormat(GLuint format)
{
    switch(format) {
    case GL_RED:
    case GL_RED_INTEGER:
    case GL_ALPHA:
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_STENCIL:
        return 1;

    case GL_RG:
    case GL_RG_INTEGER:
        return 2;

    case GL_RGB:
    case GL_RGB_INTEGER:
        return 3;

    case GL_RGBA:
    case GL_RGBA_INTEGER:
        return 4;
    }
    return 0;
}

uint bytesInImageType(GLuint type)
{
    switch(type) {
    case GL_UNSIGNED_BYTE:
    case GL_BYTE:
        return 1;
    case GL_UNSIGNED_SHORT:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_SHORT:
    case GL_HALF_FLOAT:
        return 2;
    case GL_UNSIGNED_INT:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
    case GL_UNSIGNED_INT_5_9_9_9_REV:
    case GL_UNSIGNED_INT_24_8:
    case GL_INT:
    case GL_FLOAT:
    case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
        return 4;
    }
    return 0;
}

//=== Base structure for textures ============================================//

void Texture::reset()
{
    if(m_name)
        glDeleteTextures(1, m_name.get());

    m_name = 0;
}

void Texture::generateName()
{
    glGenTextures(1, m_name.get());
}

void Texture::bind() const
{
    glBindTexture(m_target, m_name);
}

uint Texture::nextPowerOfTwo(uint n)
{
    if(!n)
        return 0;
    uint m = 1;
    while(m < n)
        m <<= 1;
    return m;
}

glm::ivec3 Texture::roundSize(
    glm::ivec3 size, bool roundX, bool roundY, bool roundZ)
{
    if(roundX)
        size.x = nextPowerOfTwo(size.x);
    if(roundY)
        size.y = nextPowerOfTwo(size.y);
    if(roundZ)
        size.z = nextPowerOfTwo(size.z);
    return size;
}

void Texture::setSwizzleMask(GLint r, GLint g, GLint b, GLint a)
{
#if AZUR_GRAPHICS_OPENGL_ES_3_0
    (void)r, (void)g, (void)b, (void)a;
    azlog(ERROR, "texture swizzles are not available in WebGL!\n");
#else
    GLint mask[4] = { r, g, b, a };
    glTexParameteriv(m_target, GL_TEXTURE_SWIZZLE_RGBA, mask);
#endif
}

bool Texture::isCompatibleWith(GLuint format, GLuint type, bool error)
{
    TextureFormat const *TF = getTextureFormatInfo(m_internalFormat);
    if(!TF) {
        azlog(ERROR, "invalid OpenGL texture internalFormat %u\n",
            m_internalFormat);
        return false;
    }

    if(format != TF->allowedFormat) {
        if(error)
            azlog(ERROR, "internalFormat %u does not allow format %u\n",
                m_internalFormat, format);
        return false;
    }

    bool ok = false;
    for(uint i = 0; i < TF->allowedTypeCount(); i++)
        ok |= (TF->allowedTypes[i] == type);
    if(!ok && error) {
        azlog(ERROR, "internalFormat %u does not allow type %u\n",
            m_internalFormat, type);
    }
    return ok;
}

GLuint Texture::naturalInputTypeFor(GLuint internalFormat)
{
    TextureFormat const *TF = getTextureFormatInfo(internalFormat);
    return TF ? TF->allowedTypes[0] : 0;
}

GLuint Texture::inputFormatFor(GLuint internalFormat)
{
    TextureFormat const *TF = getTextureFormatInfo(internalFormat);
    return TF ? TF->allowedFormat : 0;
}

//=== 2D textures ============================================================//

void Texture2D::setFormat(
    GLuint internalFormat, uint width, uint height, bool round)
{
    glm::ivec3 size(width, height, 0);
    glm::ivec3 allocSize = roundSize(size, round, round, false);

    if(internalFormat != m_internalFormat || allocSize != m_allocSize) {
        /* We don't have any data so just pass a plausible format. It won't be
           used anyway and we don't need to bother caller with this. */
        GLuint format = inputFormatFor(internalFormat);
        GLuint type = naturalInputTypeFor(internalFormat);
        glTexImage2D(m_target, 0, internalFormat, allocSize.x, allocSize.y, 0,
                     format, type, NULL);

        m_internalFormat = internalFormat;
        m_allocSize = allocSize;
    }

    m_size = size;
}

bool Texture2D::loadData(
    void const *data, GLuint format, GLuint type, int stride,
    int level, rect<int> r)
{
    if(!isCompatibleWith(format, type, true))
        return false;

    if(r.x() == 0 && r.y() == 0 && r.width() == 0 && r.height() == 0)
        r = rect<int>(0, 0, width(), height());

    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
    glTexSubImage2D(m_target, level, r.x(), r.y(), r.width(), r.height(),
                    format, type, data);
    return true;
}

bool Texture2D::loadData(Surface const &S, int level, rect<int> r)
{
    return loadData(S.data(), S.format(), S.type(), S.stride(), level, r);
}

//=== 2D texture arrays ======================================================//

void Texture2DArray::reset()
{
    Texture::reset();
    // TODO[Texture2DArray]: Free remembered texture data
}

void Texture2DArray::setFormat(
    GLuint internalFormat, uint width, uint height, uint layerCount,
    bool round)
{
    glm::ivec3 size(width, height, layerCount);
    glm::ivec3 allocSize = roundSize(size, round, round, false);

    if(internalFormat != m_internalFormat || allocSize != m_allocSize) {
        GLuint format = inputFormatFor(internalFormat);
        GLuint type = naturalInputTypeFor(internalFormat);
        glTexImage3D(m_target, 0, internalFormat, allocSize.x, allocSize.y,
                     allocSize.z, 0, format, type, NULL);

        m_internalFormat = internalFormat;
        m_allocSize = allocSize;
    }

    m_size = size;
}

bool Texture2DArray::loadLayerData(
    void const *data, GLuint format, GLuint type, int stride,
    uint layer, int level, rect<int> r)
{
    if(!isCompatibleWith(format, type, true))
        return false;

    if(r.x() == 0 && r.y() == 0 && r.width() == 0 && r.height() == 0)
        r = rect<int>(0, 0, width(), height());

    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
    glTexSubImage3D(m_target, level, r.x(), r.y(), layer, r.width(),
                    r.height(), 1, format, type, data);
    return true;
}

bool Texture2DArray::loadLayerData(
    Surface const &S, uint layer, int level, rect<int> r)
{
    return loadLayerData(
        S.data(), S.format(), S.type(), S.stride(), layer, level, r);
}

} /* namespace azur::gl */
