#include <azur/gint/render.h>
#include <gint/defs/util.h>

uint8_t AZRP_SHADER_LINE = -1;

static void configure(void)
{
    azrp_set_uniforms(AZRP_SHADER_LINE, (void *)azrp_width);
}

__attribute__((constructor))
static void register_shader(void)
{
    extern azrp_shader_t azrp_shader_line;
    AZRP_SHADER_LINE = azrp_register_shader(azrp_shader_line, configure);
    configure();
}

//---

struct command {
   uint8_t shader_id;
   /* Line color (solid color, no anti-aliasing) */
   uint16_t color;
   /* Current x/y location within fragment */
   uint16_t curr_y;
   uint16_t curr_x;
   /* Δx, Δy, and their signs */
   int16_t dx, dy, sx, sy;
   /* Current error in the Bresenham algorithm */
   int16_t cumul;
   /* Number of pixels drawn so far */
   int16_t i;
};

static int SGN(int x)
{
    return (x > 0) ? 1 : (x < 0) ? -1 : 0;
}

void azrp_line(int xA, int yA, int xB, int yB, int color)
{
    prof_enter(azrp_perf_cmdgen);

    //clipping algorithm as per "Another Simple but Faster Method for 2D Line Clipping"
    //from Dimitrios Matthes and Vasileios Drakopoulos
    //International Journal of Computer Graphics & Animation (IJCGA) Vol.9, No.1/2/3, July 2019

    int xmin = 0;
    int xmax = azrp_width-1;
    int ymin = 0;
    int ymax = azrp_height-1;

    //step 1 line are fully out of the screen
    if((xA<xmin && xB<xmin) || (xA>xmax && xB>xmax) || (yA<ymin && yB<ymin) || (yA>ymax && yB>ymax)) {
        prof_leave(azrp_perf_cmdgen);
        return;
    }

    int x1, x2, y1, y2;

    // step 1.5 - specific to Azur fragment approach
    // we swap to always start with the point on top as the fragment are updated from top to bottom
    // (x1,y1) is the most top point and (x2,y2) is the most bottom point (rankig as per y values only
    if(yA <= yB) {
        x1 = xA; y1 = yA;
        x2 = xB; y2 = yB;
    }
    else {
        x1 = xB; y1 = yB;
        x2 = xA; y2 = yA;
    }

    //step 2 line clipping within the box (xmin,ymin) --> (xmax,ymax)
    int x[2];
    int y[2];

    x[0] = x1; x[1] = x2;
    y[0] = y1; y[1] = y2;


    for(int i=0; i<2; i++) {
        if(x[i] < xmin) {
            x[i] = xmin;    y[i] = ((y2-y1) * (xmin-x1)) / (x2-x1) + y1;
        }
        else if(x[i] > xmax) {
            x[i] = xmax;    y[i] = ((y2-y1) * (xmax-x1)) / (x2-x1) + y1;
        }

        if(y[i] < ymin) {
            x[i] = ((x2-x1) * (ymin-y1)) / (y2-y1) + x1;    y[i] = ymin;
        }
        else if(y[i] > ymax) {
            x[i] = ((x2-x1) * (ymax-y1)) / (y2-y1) + x1;    y[i] = ymax;
        }
    }

    if((x[0] < xmin && x[1] < xmin) || (x[0] > xmax && x[1] > xmax)) {
        prof_leave(azrp_perf_cmdgen);
        return;
    }

    x1 = x[0];
    y1 = y[0];
    x2 = x[1];
    y2 = y[1];

    int frag_first = y1 / azrp_frag_height;
    int frag_last  = y2 / azrp_frag_height;
    int frag_count = frag_last - frag_first + 1;

    struct command *cmd =
        azrp_cmdq_command(sizeof *cmd, frag_first, frag_count);
    if(!cmd) {
        prof_leave(azrp_perf_cmdgen);
        return;
    }

    cmd->shader_id = AZRP_SHADER_LINE;
    cmd->color = color;
    cmd->curr_x = x1;
    cmd->curr_y = y1 & 15;
    cmd->dx = abs(x2-x1);
    cmd->sx = SGN(x2-x1);
    cmd->dy = abs(y2-y1);
    cmd->sy = SGN(y2-y1);
    cmd->i = 0;
    cmd->cumul = max(cmd->dx, cmd->dy) >> 1;

    prof_leave(azrp_perf_cmdgen);
}

void azrp_shader_line(void *uniforms, void *cmd0, void *frag0)
{
    struct command *cmd = cmd0;
    uint16_t *frag = (uint16_t *)frag0;
    int width = (int)uniforms;

    int y = cmd->curr_y, x = cmd->curr_x;
    int cumul = cmd->cumul;
    frag[width * y + x] = cmd->color;

    int i;

    if(cmd->dx >= cmd->dy) {
        for( i = cmd->i; i < cmd->dx; i++ ) {
            x += cmd->sx;
            cumul += cmd->dy;
            if(cumul > cmd->dx)
            {
                cumul -= cmd->dx;
                y += cmd->sy;
                // if y=16, this means we are changing to next fragment
                if(y == azrp_frag_height ) break;
            }

            frag[width * y + x] = cmd->color;
        }
    }
    else {
        for( i = cmd->i; i < cmd->dy; i++ ) {
            y += cmd->sy;
            cumul += cmd->dx;
            if(cumul > cmd->dy)
            {
                cumul -= cmd->dy;
                x += cmd->sx;
            }

            // if y=16, this means we are changing to next fragment
            if(y == azrp_frag_height) break;

            frag[width * y + x] = cmd->color;
        }
    }

    cmd->cumul = cumul;
    cmd->curr_y = y & (azrp_frag_height - 1);
    cmd->curr_x = x;
    cmd->i = i+1;
}

