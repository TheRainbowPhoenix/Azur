#include <azur/gint/render.h>

uint8_t AZRP_SHADER_RECT = -1;

static void configure(void)
{
    azrp_set_uniforms(AZRP_SHADER_RECT, (void *)azrp_width);
}

__attribute__((constructor))
static void register_shader(void)
{
    extern azrp_shader_t azrp_shader_rect;
    AZRP_SHADER_RECT = azrp_register_shader(azrp_shader_rect, configure);
    configure();
}

//---

struct command {
    uint8_t shader_id;
    /* Local y coordinate of the first line in the fragment */
    uint8_t y;
    /* Number of lines to render total, including this fragment */
    uint8_t height_total;
    /* Number of lines to render on the current fragment */
    uint8_t height_frag;
    /* Rectangle along the x coordinates (in longwords) */
    uint16_t xl, wl;
    /* Offset of left edge */
    int16_t edge_1;
    /* Offset of right edge */
    int16_t edge_2;
    /* Core loop (this is an internal label of the renderer) */
    void const *loop;
    /* Color, when applicable */
    uint16_t color;
};

/* Core loops */
extern void azrp_shader_rect_loop_flat(void);
extern void azrp_shader_rect_loop_invert(void);
extern void azrp_shader_rect_loop_darken(void);
extern void azrp_shader_rect_loop_whiten(void);

static void (*loops[])(void) = {
    azrp_shader_rect_loop_flat,
    azrp_shader_rect_loop_invert,
    azrp_shader_rect_loop_darken,
    azrp_shader_rect_loop_whiten,
};

void azrp_rect(int x1, int y1, int width0, int height0, int color_or_effect)
{
    /* Clipping (x2 and y2 excluded) */
    int x2 = x1 + width0;
    int y2 = y1 + height0;
    if(x1 < 0)
        x1 = 0;
    if(y1 < 0)
        y1 = 0;
    if(x2 > azrp_width)
        x2 = azrp_width;
    if(y2 > azrp_height)
        y2 = azrp_height;
    if(x2 <= x1 || y2 <= y1)
        return;

    prof_enter(azrp_perf_cmdgen);

    int frag_first, first_offset, frag_count;
    azrp_config_get_lines(y1, y2 - y1,
        &frag_first, &first_offset, &frag_count);

    struct command cmd;
    cmd.shader_id = AZRP_SHADER_RECT;
    cmd.y = first_offset;
    cmd.height_total = y2 - y1;
    cmd.height_frag = azrp_frag_height - first_offset;
    if(cmd.height_total < cmd.height_frag)
        cmd.height_frag = cmd.height_total;
    cmd.xl = (x1 >> 1);
    cmd.wl = ((x2 - 1) >> 1) - cmd.xl + 1;
    cmd.edge_1 = (x1 & 1) ? 0 : -2;
    cmd.edge_2 = 4 * cmd.wl + ((x2 & 1) ? -2 : 0);
    cmd.loop = loops[color_or_effect >= 0 ? 0 : -color_or_effect];
    cmd.color = color_or_effect;

    azrp_queue_command(&cmd, sizeof cmd, frag_first, frag_count);
    prof_leave(azrp_perf_cmdgen);
}
