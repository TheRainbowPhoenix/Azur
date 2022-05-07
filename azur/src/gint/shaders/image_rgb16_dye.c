#include <azur/gint/render.h>

void azrp_image_rgb16_dye(int x, int y, image_t const *img, int eff,
    int dye_color)
{
    azrp_subimage_rgb16_dye(x, y, img, 0, 0, img->width, img->height, eff,
        dye_color);
}

void azrp_subimage_rgb16_dye(int x, int y, image_t const *img,
    int left, int top, int w, int h, int eff, int dye_color)
{
    prof_enter(azrp_perf_cmdgen);
    struct gint_image_box box = { x, y, w, h, left, top };
    struct gint_image_cmd cmd;

    if(gint_image_mkcmd(&box, img, eff, false, true, &cmd, azrp_width,
            azrp_height)) {
        cmd.effect += 12;
        cmd.color_1 = img->alpha;
        cmd.color_2 = dye_color;
        cmd.loop = azrp_image_shader_rgb16_dye;
        azrp_queue_image(&box, img, &cmd);
    }
    prof_leave(azrp_perf_cmdgen);
}
