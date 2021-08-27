#include <azur/gint/render.h>
#include <gint/defs/util.h>

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

/* Profile values from bopti */
#define PX_RGB565   0
#define PX_RGB565A  1
#define PX_P8       2
#define PX_P4       3

void azrp_image(int x, int y, bopti_image_t const *image)
{
    azrp_subimage(x, y, image, 0, 0, image->width, image->height, 0);
}

void azrp_subimage(int x, int y, bopti_image_t const *image,
    int left, int top, int width, int height, int flags)
{
    prof_enter(azrp_perf_cmdgen);

    if(!(flags & DIMAGE_NOCLIP)) {
        /* TODO: tex2d: clip function */
    }

    struct azrp_shader_tex2d_command cmd;
    cmd.shader_id = AZRP_SHADER_TEX2D;
    cmd.columns = width;
    cmd.image = image;

    int input_multiplier = 1;
    if(image->profile == PX_P8) input_multiplier = 0;
    if(image->profile == PX_P4) input_multiplier = -1;

    /* This divides by azrp_frag_height */
    cmd.fragment_id = (azrp_scale == 1) ? (y >> 3) : (y >> 4);

    while(height > 0) {
        cmd.lines = min(height, azrp_frag_height - (y & (azrp_frag_height-1)));

        int input_offset = (image->width * top + left) << input_multiplier;
        cmd.input = (void *)image->data + input_offset;

        cmd.output = 2 * (azrp_width * (y & (azrp_frag_height-1)) + x);

        y += cmd.lines;
        top += cmd.lines;
        height -= cmd.lines;

        azrp_queue_command(&cmd, sizeof cmd);
        cmd.fragment_id++;
    }

    prof_leave(azrp_perf_cmdgen);
}
