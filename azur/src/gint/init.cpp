#include <azur/azur.h>
#include <azur/log.h>
#include <gint/timer.h>
#include <gint/cpu.h>

int azur_init(char const *title, int window_width, int window_height)
{
    (void)title;
    (void)window_width;
    (void)window_height;
    return 0;
}

__attribute__((destructor))
void azur_quit(void)
{
}

//---
// Main loop setup
//---

/* Time spent in the main loop (seconds)
   TODO: Handle ml_time (also in the SDL backend) */
static double ml_time = 0.0;
/* Timers for render and updates */
static int ml_timer_render = -1;
static int ml_timer_update = -1;

static int set_flag(volatile int *flag)
{
    *flag = 1;
    return TIMER_CONTINUE;
}

int azur_main_loop(
    void (*render)(void), int render_fps,
    int (*update)(void), int update_ups,
    int flags)
{
    volatile int render_tick = 1;
    volatile int update_tick = 0;
    bool started = false;

    ml_timer_render = timer_configure(TIMER_ANY, 1000000 / render_fps,
        GINT_CALL(set_flag, &render_tick));
    if(ml_timer_render < 0) {
        azlog(ERROR, "failed to create render timer\n");
        return 1;
    }
    else {
        timer_start(ml_timer_render);
    }

    if(!(flags & AZUR_MAIN_LOOP_TIED)) {
        ml_timer_update = timer_configure(TIMER_ANY, 1000000 / update_ups,
            GINT_CALL(set_flag, &update_tick));
        if(ml_timer_update < 0) {
            timer_stop(ml_timer_render);
            azlog(ERROR, "failed to create render timer\n");
            return 1;
        }
        else {
            timer_start(ml_timer_update);
        }
    }

    while(1) {
        if(update_tick && !(flags & AZUR_MAIN_LOOP_TIED)) {
            update_tick = 0;
            if(update && update()) break;
        }

        if(render_tick) {
            render_tick = 0;

            /* Tied renders and updates */
            if(started && (flags & AZUR_MAIN_LOOP_TIED)) {
                if(update && update()) break;
            }
            if(render) render();
            started = true;
        }

        sleep();
    }

    if(ml_timer_render >= 0) {
        timer_stop(ml_timer_render);
        ml_timer_render = 0;
    }
    if(ml_timer_update >= 0) {
        timer_stop(ml_timer_update);
        ml_timer_update = 0;
    }

    return 0;
}
