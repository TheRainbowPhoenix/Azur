#include <azur/gint/render.h>

void azrp_image_rgb16_swapcolor(int x, int y, image_t const *img, int eff,
    int old_color, int new_color)
{
    azrp_subimage_rgb16_swapcolor(x, y, img, 0, 0, img->width, img->height,
        eff, old_color, new_color);
}

void azrp_subimage_rgb16_swapcolor(int x, int y, image_t const *img,
    int left, int top, int w, int h, int eff, int old_color, int new_color)
{
    prof_enter(azrp_perf_cmdgen);
    struct gint_image_box box = { x, y, w, h, left, top };
    struct gint_image_cmd cmd;

    if(gint_image_mkcmd(&box, img, eff, false, true, &cmd, azrp_width,
            azrp_height)) {
        cmd.effect += 8;
        cmd.color_1 = old_color;
        cmd.color_2 = new_color;
        cmd.loop = azrp_image_shader_rgb16_swapcolor;
        azrp_queue_image(&box, img, &cmd);
    }
    prof_leave(azrp_perf_cmdgen);
}

void azrp_image_rgb16_addbg(int x, int y, image_t const *img, int eff,
    int bg_color)
{
    azrp_subimage_rgb16_addbg(x, y, img, 0, 0, img->width, img->height,
        eff, bg_color);
}

void azrp_subimage_rgb16_addbg(int x, int y, image_t const *img,
    int left, int top, int w, int h, int eff, int bg_color)
{
    prof_enter(azrp_perf_cmdgen);
    struct gint_image_box box = { x, y, w, h, left, top };
    struct gint_image_cmd cmd;

    if(gint_image_mkcmd(&box, img, eff, false, true, &cmd, azrp_width,
            azrp_height)) {
        cmd.effect += 8;
        cmd.color_1 = image_alpha(img->format);
        cmd.color_2 = bg_color;
        cmd.loop = azrp_image_shader_rgb16_swapcolor;
        azrp_queue_image(&box, img, &cmd);
    }
    prof_leave(azrp_perf_cmdgen);
}
