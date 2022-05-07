#include <azur/gint/render.h>

void azrp_subimage_rgb16_effect(int x, int y, image_t const *img,
    int left, int top, int w, int h, int eff, ...)
{
    va_list args;
    va_start(args, eff);

    if(eff & IMAGE_CLEARBG) {
        int bg = va_arg(args, int);
        azrp_subimage_rgb16_clearbg(x, y, img, left, top, w, h, eff, bg);
    }
    else if(eff & IMAGE_SWAPCOLOR) {
        int c1 = va_arg(args, int);
        int c2 = va_arg(args, int);
        azrp_subimage_rgb16_swapcolor(x, y, img, left, top, w, h, eff, c1, c2);
    }
    else if(eff & IMAGE_ADDBG) {
        int bg = va_arg(args, int);
        azrp_subimage_rgb16_addbg(x, y, img, left, top, w, h, eff, bg);
    }
    else if(eff & IMAGE_DYE) {
        int dye = va_arg(args, int);
        azrp_subimage_rgb16_dye(x, y, img, left, top, w, h, eff, dye);
    }
    else {
        azrp_subimage_rgb16(x, y, img, left, top, w, h, eff);
    }

    va_end(args);
}
