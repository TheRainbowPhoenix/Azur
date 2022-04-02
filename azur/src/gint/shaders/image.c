#include <azur/gint/render.h>
#include <gint/defs/util.h>

uint8_t AZRP_SHADER_IMAGE = -1;

__attribute__((constructor))
static void register_shader(void)
{
    extern azrp_shader_t azrp_shader_image;
    AZRP_SHADER_IMAGE = azrp_register_shader(azrp_shader_image);
}

void azrp_shader_image_configure(void)
{
    azrp_set_uniforms(AZRP_SHADER_IMAGE, (void *)(2 * azrp_width));
}

//---

/* Profile IDs */
#define RGB565      0
#define RGB565A     1
#define P4_RGB565A  3
#define P8_RGB565   4
#define P8_RGB565A  5
#define P4_RGB565   6

void azrp_image(int x, int y, bopti_image_t const *image)
{
    azrp_subimage(x, y, image, 0, 0, image->width, image->height, 0);
}

void azrp_subimage(int x, int y, bopti_image_t const *image,
    int left, int top, int width, int height, int flags)
{
    prof_enter(azrp_perf_cmdgen);

    if(!(flags & DIMAGE_NOCLIP)) {
        /* TODO: image: clip function */
    }

    struct azrp_shader_image_command cmd;
    cmd.shader_id = AZRP_SHADER_IMAGE;
    cmd.columns = width;
    cmd.image = image;

    int row_stride;

    if(image->profile == P8_RGB565 || image->profile == P8_RGB565A) {
        row_stride = image->width;
        cmd.input = (void *)image->data + (image->data[0] * 2) + 2 +
            top * row_stride + left;
    }
    else if(image->profile == P4_RGB565 || image->profile == P4_RGB565A) {
        row_stride = (image->width + 1) >> 1;
        cmd.input = (void *)image->data + 32 + top * row_stride + (left >> 1);

        int odd_left  = left & 1;
        int odd_right = (left + width) & 1;

        cmd.edge1 = -1 + odd_left;
        cmd.edge2 = width + odd_left;
        cmd.columns += odd_left + odd_right;
        x -= odd_left;
    }
    else {
        row_stride = image->width << 1;
        cmd.input = (void *)image->data + top * row_stride + (left << 1);
    }

    /* This divides by azrp_frag_height */
    int fragment_id = (azrp_scale == 1) ? (y >> 3) : (y >> 4);

    /* These settings only apply to the first fragment */
    int first_y = (y + azrp_frag_offset) & (azrp_frag_height - 1);
    cmd.lines = azrp_frag_height - first_y;
    cmd.output = 2 * (azrp_width * first_y + x);

    /* Settings for further updates */
    cmd.height = height;
    cmd.row_stride = row_stride;
    cmd.x = x;

    int n = 1 + (height - cmd.lines + azrp_frag_height - 1) / azrp_frag_height;
    azrp_queue_command(&cmd, sizeof cmd, fragment_id, n);

    prof_leave(azrp_perf_cmdgen);
}
