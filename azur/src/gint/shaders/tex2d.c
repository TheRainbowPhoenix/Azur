#include <azur/gint/render.h>

uint8_t AZRP_SHADER_TEX2D = -1;

__attribute__((constructor))
static void register_shader(void)
{
    extern azrp_shader_t azrp_shader_tex2d;
    AZRP_SHADER_TEX2D = azrp_register_shader(azrp_shader_tex2d);
}

void azrp_shader_tex2d_configure(void)
{
    azrp_set_uniforms(AZRP_SHADER_TEX2D, (void *)(2 * azrp_width));
}

//---
