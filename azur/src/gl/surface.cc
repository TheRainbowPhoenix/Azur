//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//

#include <azur/opengl.h>
#include <azur/log.h>
#include <azur/resources.h>
#include <stb_image.h>

namespace azur::gl {

//=== Surface loading ========================================================//

void Surface::reset(void)
{
    if(m_ownsData)
        free(m_data);

    m_format = m_type = 0;
    m_width = m_height = m_stride = 0;
    m_data = nullptr;
    m_ownsData = false;
}

uint Surface::channelCount() const
{
    uint n = channelsInImageFormat(m_format);
    if(n == 0)
        azlog(ERROR, "invalid OpenGL source image format %u\n", m_format);
    return n;
}

int Surface::bytesPerValue() const
{
    uint n = bytesInImageType(m_type);
    if(n == 0)
        azlog(ERROR, "invalid OpenGL source image data type %u\n", m_type);
    return n;
}

Surface *Surface::loadFromImageData(void const *buf, size_t size)
{
    int width, height, n;
    u8 *data =
        stbi_load_from_memory((u8 const *)buf, size, &width, &height, &n, 0);
    return loadFromSTBi(data, width, height, n);
}

Surface *Surface::loadFromImageFile(char const *path)
{
    int width, height, n;
    u8 *data = stbi_load(path, &width, &height, &n, 0);
    return loadFromSTBi(data, width, height, n);
}

Surface *Surface::loadFromResource(char const *resource)
{
    size_t size;
    void const *buf = azur::getResource(resource, &size);
    return buf ? loadFromImageData(buf, size) : nullptr;
}

/* TODO: Coverage for OpenGL formats when loading from STB
   GL_DEPTH_COMPONENT:  Normally generated at runtime
   GL_DEPTH_STENCIL:    Normally generated at runtime
   GL_LUMINANCE_ALPHA:  GL_RG + swizzle mask
   GL_LUMINANCE:        GL_RED
   GL_ALPHA:            GL_RED + swizzle mask */
Surface *Surface::loadFromSTBi(
    u8 *data, int width, int height, int n, bool integer)
{
    if(!data) {
        azlog(ERROR, "cannot load surface from memory: stbi: %s\n",
            stbi_failure_reason());
        return nullptr;
    }

    GLuint format;
    if(n == 1)
        format = integer ? GL_RED_INTEGER : GL_RED;
    else if(n == 2)
        format = integer ? GL_RG_INTEGER : GL_RG;
    else if(n == 3)
        format = integer ? GL_RGB_INTEGER : GL_RGB;
    else if(n == 4)
        format = integer ? GL_RGBA_INTEGER : GL_RGBA;
    else {
        azlog(ERROR, "invalid stb_image channel count: %d\n", n);
        return nullptr;
    }

    Surface *s = new Surface();
    s->m_format = format;
    s->m_type = GL_UNSIGNED_BYTE;
    s->m_width = width;
    s->m_height = height;
    s->m_stride = width;
    s->m_data = data;
    s->m_ownsData = true;
    return s;
}

} /* namespace azur::gl */
