#include <azur/gint/render.h>
#include <gint/defs/util.h>

extern uint8_t AZRP_SHADER_IMAGE_RGB16;
extern uint8_t AZRP_SHADER_IMAGE_P8;
extern uint8_t AZRP_SHADER_IMAGE_P4;

void azrp_queue_image(struct gint_image_box *box, image_t const *img,
    struct gint_image_cmd *cmd)
{
    /* TODO: Ironically, this loads all 3 entry points */
    int f = img->format;
    if(IMAGE_IS_RGB16(f))
        cmd->shader_id = AZRP_SHADER_IMAGE_RGB16;
    else if(IMAGE_IS_P8(f))
        cmd->shader_id = AZRP_SHADER_IMAGE_P8;
    else
        cmd->shader_id = AZRP_SHADER_IMAGE_P4;

    /* This divides by azrp_frag_height */
    /* TODO: Have a proper way to do optimized-division by azrp_frag_height */
    int fragment_id = (azrp_scale == 1) ? (box->y >> 4) : (box->y >> 4);

    /* These settings only apply to the first fragment */
    int first_y = (box->y + azrp_frag_offset) & (azrp_frag_height - 1);
    cmd->lines = min(box->h, azrp_frag_height - first_y);
    cmd->output = (void *)azrp_frag + (azrp_width * first_y + cmd->x) * 2;

    int n = 1 + (box->h - cmd->lines + azrp_frag_height-1) / azrp_frag_height;
    azrp_queue_command(cmd, sizeof *cmd, fragment_id, n);
}

void azrp_subimage(int x, int y, image_t const *img,
    int left, int top, int width, int height, int flags)
{
    int f = img->format;

    if(IMAGE_IS_RGB16(f))
        return azrp_subimage_rgb16(x, y, img, left, top, width, height, flags);
    if(IMAGE_IS_P8(f))
        return azrp_subimage_p8(x, y, img, left, top, width, height, flags);
    if(IMAGE_IS_P4(f))
        return azrp_subimage_p4(x, y, img, left, top, width, height, flags);
}

void azrp_image(int x, int y, image_t const *img)
{
    azrp_subimage(x, y, img, 0, 0, img->width, img->height, 0);
}
