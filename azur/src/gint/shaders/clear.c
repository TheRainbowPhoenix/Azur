#include <azur/gint/render.h>

uint8_t AZRP_SHADER_CLEAR = -1;

static void configure(void)
{
    int longs_in_fragment = (azrp_width * azrp_frag_height / 2);
    azrp_set_uniforms(AZRP_SHADER_CLEAR, (void *)longs_in_fragment);
}

__attribute__((constructor))
static void register_shader(void)
{
    extern azrp_shader_t azrp_shader_clear;
    AZRP_SHADER_CLEAR = azrp_register_shader(azrp_shader_clear, configure);
    configure();
}

//---

struct command {
   uint8_t shader_id;
   uint8_t _;
   /* Color */
   uint16_t color;
};

void azrp_clear(uint16_t color)
{
    prof_enter(azrp_perf_cmdgen);

    struct command cmd;
    cmd.shader_id = AZRP_SHADER_CLEAR;
    cmd.color = color;

    azrp_queue_command(&cmd, sizeof cmd, 0, azrp_frag_count);
    prof_leave(azrp_perf_cmdgen);
}
