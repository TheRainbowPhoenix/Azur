#include <azur/gint/render.h>

uint8_t AZRP_SHADER_CLEAR = -1;

__attribute__((constructor))
static void register_shader(void)
{
    extern azrp_shader_t azrp_shader_clear;
    AZRP_SHADER_CLEAR = azrp_register_shader(azrp_shader_clear);
}

void azrp_shader_clear_configure(void)
{
    int longs_in_fragment = 0;

    if(azrp_scale == 1)
        longs_in_fragment = (396 * 2) * 8 / 4;
    else if(azrp_scale == 2)
        longs_in_fragment = (198 * 2) * 16 / 4;
    else if(azrp_scale == 3)
        longs_in_fragment = (132 * 2) * 16 / 4;

    azrp_set_uniforms(AZRP_SHADER_CLEAR, (void *)longs_in_fragment);
}

//---

struct command {
   /* Shader ID and fragment number */
   uint8_t shader_id;
   uint8_t fragment_id;
   /* Color */
   uint16_t color;
};

void azrp_clear(uint16_t color)
{
    struct command cmd;
    cmd.shader_id = AZRP_SHADER_CLEAR;
    cmd.color = color;

    for(int i = 0; i < azrp_frag_count; i++) {
        cmd.fragment_id = i;
        azrp_queue_command(&cmd, sizeof cmd);
    }
}
