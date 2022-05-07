#include <azur/gint/render.h>
#include <gint/defs/util.h>

uint8_t AZRP_SHADER_IMAGE_P8 = -1;

static void shader_p8(void *uniforms, void *command, void *fragment)
{
    struct gint_image_cmd *cmd = (void *)command;
    cmd->input = gint_image_p8_loop((int)uniforms, cmd);
    cmd->height -= cmd->lines;
    cmd->lines = min(cmd->height, azrp_frag_height);
    cmd->output = fragment + cmd->x * 2;
}

__attribute__((constructor))
static void register_shader(void)
{
    AZRP_SHADER_IMAGE_P8 = azrp_register_shader(shader_p8);
}

void azrp_shader_image_p8_configure(void)
{
    azrp_set_uniforms(AZRP_SHADER_IMAGE_P8, (void *)azrp_width);
}

void azrp_image_p8(int x, int y, image_t const *img, int eff)
{
    azrp_subimage_p8(x, y, img, 0, 0, img->width, img->height, eff);
}

void azrp_subimage_p8(int x, int y, image_t const *img,
    int left, int top, int w, int h, int eff)
{
    if(img->profile == IMAGE_P8_RGB565A)
        return azrp_subimage_p8_clearbg(x, y, img, left, top, w, h, eff,
            img->alpha);

    prof_enter(azrp_perf_cmdgen);
    struct gint_image_box box = { x, y, w, h, left, top };
    struct gint_image_cmd cmd;

    if(gint_image_mkcmd(&box, img, eff, false, true, &cmd, azrp_width,
            azrp_height)) {
        cmd.loop = azrp_image_shader_p8_normal;
        azrp_queue_image(&box, img, &cmd);
    }
    prof_leave(azrp_perf_cmdgen);
}

void azrp_image_p8_clearbg(int x, int y, image_t const *img, int eff, int bg)
{
    azrp_subimage_p8_clearbg(x, y, img, 0, 0, img->width, img->height, eff,
        bg);
}

void azrp_subimage_p8_clearbg(int x, int y, image_t const *img,
    int left, int top, int w, int h, int eff, int bg_color)
{
    prof_enter(azrp_perf_cmdgen);
    struct gint_image_box box = { x, y, w, h, left, top };
    struct gint_image_cmd cmd;

    if(gint_image_mkcmd(&box, img, eff, false, true, &cmd, azrp_width,
            azrp_height)) {
        cmd.effect += 4;
        cmd.color_1 = bg_color;
        cmd.loop = gint_image_p8_clearbg;
        azrp_queue_image(&box, img, &cmd);
    }
    prof_leave(azrp_perf_cmdgen);
}
