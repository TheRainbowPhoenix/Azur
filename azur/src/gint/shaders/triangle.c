#include <azur/gint/render.h>

uint8_t AZRP_SHADER_TRIANGLE = -1;

__attribute__((constructor))
static void register_shader(void)
{
    extern azrp_shader_t azrp_shader_triangle;
    AZRP_SHADER_TRIANGLE = azrp_register_shader(azrp_shader_triangle);
}

void azrp_shader_triangle_configure(void)
{
    azrp_set_uniforms(AZRP_SHADER_TRIANGLE, (void *)azrp_width);
}

static int min(int x, int y)
{
    return (x < y) ? x : y;
}
static int max(int x, int y)
{
    return (x > y) ? x : y;
}

//---

struct command {
   uint8_t shader_id;
   /* Local y coordinate of the first line in the fragment */
   uint8_t y;
   /* Number of lines to render on the current fragment */
   uint8_t height_frag;
   /* Numebr of lines to render total, includnig this fragment */
   uint8_t height_total;
   /* Rectangle along the x coordinates (x_max included) */
   uint16_t x_min, x_max;
   /* Color */
   uint16_t color;
   /* Initial barycentric coordinates */
   int u0, v0, w0;
   /* Variation of each coordinate for a movement in x/y */
   int du_x, dv_x, dw_x;
   int du_y, dv_y, dw_y;
};

//---

// TODO: Write in assembler
void azrp_shader_triangle(void *uniforms0, void *command0, void *fragment0)
{
    int width = (int)uniforms0;
    struct command *cmd = command0;
    uint16_t *frag = fragment0;

    frag += cmd->x_min + width * cmd->y;

    int u, v, w;

    for(int y = 0; y < cmd->height_frag; y++) {
        u = cmd->u0;
        v = cmd->v0;
        w = cmd->w0;

        for(int x = cmd->x_min; x <= cmd->x_max; x++) {
            if((u | v | w) > 0) {
                frag[x] = cmd->color;
            }

            u += cmd->du_x;
            v += cmd->dv_x;
            w += cmd->dw_x;
        }

        frag += width;
        cmd->u0 += cmd->du_y;
        cmd->v0 += cmd->dv_y;
        cmd->w0 += cmd->dw_y;
    }

    /* Prepare next fragment */
    cmd->y = 0;
    cmd->height_total -= cmd->height_frag;
    cmd->height_frag = min(azrp_frag_height, cmd->height_total);
}

//---

static int edge_start(int x1, int y1, int x2, int y2, int px, int py)
{
    return (y2 - y1) * (px - x1) - (x2 - x1) * (py - y1);
}
void azrp_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int color)
{
    prof_enter(azrp_perf_cmdgen);

    /* Find a rectangle containing the triangle */
    int min_x = max(0, min(x1, min(x2, x3)));
    int max_x = min(azrp_width-1, max(x1, max(x2, x3)));
    int min_y = max(0, min(y1, min(y2, y3)));
    int max_y = min(azrp_height-1, max(y1, max(y2, y3)));

    /* TODO: Have a proper way to do optimized-division by azrp_frag_height
       TODO: Also account for first-fragment offset */
    int frag_first = min_y >> 4;
    int frag_last  = max_y >> 4;
    int frag_count = frag_last - frag_first + 1;
    int first_offset = min_y & 15;

    struct command cmd;
    cmd.shader_id = AZRP_SHADER_TRIANGLE;
    cmd.y = first_offset;
    cmd.height_total = max_y - min_y + 1;
    cmd.height_frag = min(cmd.height_total, azrp_frag_height - cmd.y);
    cmd.x_min = min_x;
    cmd.x_max = max_x;
    cmd.color = color;

    /* Vector products for barycentric coordinates */
    cmd.u0 = edge_start(x2, y2, x3, y3, min_x, min_y);
    cmd.du_x = y3 - y2;
    cmd.du_y = x2 - x3;
    cmd.v0 = edge_start(x3, y3, x1, y1, min_x, min_y);
    cmd.dv_x = y1 - y3;
    cmd.dv_y = x3 - x1;
    cmd.w0 = edge_start(x1, y1, x2, y2, min_x, min_y);
    cmd.dw_x = y2 - y1;
    cmd.dw_y = x1 - x2;

    azrp_queue_command(&cmd, sizeof cmd, frag_first, frag_count);
    prof_leave(azrp_perf_cmdgen);
}
