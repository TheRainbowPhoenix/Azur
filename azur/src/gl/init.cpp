#include <azur/azur.h>
#include <azur/log.h>
#include <azur/gl/gl.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

static SDL_Window *window = NULL;
static SDL_GLContext glcontext = NULL;

static void main_loop_quit(void);

#ifdef AZUR_PLATFORM_EMSCRIPTEN
#include <emscripten/html5.h>

static EM_BOOL fullscreen_callback(int ev, void const *_, void *window0)
{
    double w, h;
    emscripten_get_element_css_size("canvas", &w, &h);

    SDL_Window *window = window0;
    SDL_SetWindowSize(window, (int)w, (int)h);

    azlog(INFO, "Canvas size updated: now %dx%d", (int)w, (int)h);
    return EM_TRUE;
}

static void enter_fullscreen(SDL_Window *window)
{
    EmscriptenFullscreenStrategy strategy = {
        .scaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_STDDEF,
        .filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT,
        .canvasResizedCallback = fullscreen_callback,
        .canvasResizedCallbackUserData = window,
    };
    emscripten_enter_soft_fullscreen("canvas", &strategy);
    azlog(INFO, "Entered fullscreen! (info)");
    azlog(ERROR, "Entered fullscreen! (error)");
}
#endif /* AZUR_PLATFORM_EMSCRIPTEN */

int azur_init(char const *title, int window_width, int window_height)
{
    int rc = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    if(rc < 0) {
        azlog(FATAL, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    rc = IMG_Init(IMG_INIT_PNG);
    if(rc != IMG_INIT_PNG) {
        azlog(FATAL, "IMG_Init: %s\n", IMG_GetError());
        return 1;
    }

    /* Select OpenGL 3.3 core */
    #ifdef AZUR_GRAPHICS_OPENGL_3_3
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    #endif

    /* ... or select Open GL ES 2.0 */
    #ifdef AZUR_GRAPHICS_OPENGL_ES_2_0
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    #endif

    window = SDL_CreateWindow(title,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        window_width, window_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if(!window) {
        azlog(FATAL, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }

    glcontext = SDL_GL_CreateContext(window);
    if(!glcontext) {
        azlog(FATAL, "SDL_GL_CreateContext: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_MakeCurrent(window, glcontext);

    rc = SDL_GL_SetSwapInterval(0);
    if(rc < 0)
        azlog(ERROR, "SDL_GL_SetSwapInterval: %s\n", SDL_GetError());

    #ifdef AZUR_GRAPHICS_OPENGL_3_3
    rc = gl3wInit();
    if(rc != 0) {
        azlog(FATAL, "gl3wInit: %d\n", rc);
        return 1;
    }
    #endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef AZUR_PLATFORM_EMSCRIPTEN
    enter_fullscreen(window);
#endif

    return 0;
}

__attribute__((destructor))
void azur_quit(void)
{
    main_loop_quit();

    if(window) SDL_DestroyWindow(window);
    if(glcontext) SDL_GL_DeleteContext(glcontext);
    SDL_Quit();

    window = NULL;
    glcontext = NULL;
}

#ifdef AZUR_TOOLKIT_SDL

SDL_Window *azur_sdl_window(void)
{
    return window;
}

#endif /* AZUR_TOOLKIT_SDL */

//---
// Main loop setup
//---

/* Time spent in the main loop (seconds)
   TODO: Handle ml_time (also in the gint backend) */
static double ml_time = 0.0;

/* In emscripten, callbacks are void/void, vsync is always ON, and the
   mechanism used to generate updates is not a timer. */
#ifdef AZUR_PLATFORM_EMSCRIPTEN

#include <emscripten.h>
#include <emscripten/html5.h>

struct params {
    void (*render)(void);
    int (*update)(void);
    bool tied, started;
};

static void nop(void)
{
}

static EM_BOOL frame(double time, void *data)
{
    struct params *params = data;

    if(params->tied && params->started)
        if(params->update) params->update();

    if(params->render) params->render();
    params->started = true;

    return EM_TRUE;
}

int azur_main_loop(
    void (*render)(void), int render_fps,
    int (*update)(void), int update_ups,
    int flags)
{
    static struct params p;
    p.render = render;
    p.update = update;
    p.tied = flags & AZUR_MAIN_LOOP_TIED;
    p.started = false;

    (void)render_fps;
    emscripten_request_animation_frame_loop(frame, &p);

    if(!(flags & AZUR_MAIN_LOOP_TIED)) {
        void (*update_v)(void) = (void *)update;
        emscripten_set_main_loop(update_v, update_ups, true);
    }
    else {
        /* Even if no update is requested, we want emscripten to simulate an
           infinite loop, as it is needed for Dear ImGui to work due to
           threading considerations */
        emscripten_set_main_loop(nop, 1, true);
    }

    return 0;
}

static void main_loop_quit(void)
{
}

/* In standard SDL, vsync is configurable and timers are used to get
   callbacks. Events are queued to the main thread since rendering cannot be
   done from another thread. */
#else

/* Event code for events queued to break SDL_WaitEvent() from timer handlers */
static int ml_event = -1;
/* Timers for render and updates */
static SDL_TimerID ml_timer_render = 0;
static SDL_TimerID ml_timer_update = 0;

static Uint32 handler(Uint32 interval, void *param)
{
    *(int *)param = 1;

    SDL_Event e;
    e.user.type = SDL_USEREVENT;
    e.user.code = ml_event;
    SDL_PushEvent(&e);
    return interval;
}

static int filter_event(void *userdata, SDL_Event *event)
{
    (void)userdata;
    if(event->type == SDL_USEREVENT && event->user.code == ml_event)
        return 0;
    return 1;
}

int azur_main_loop(
    void (*render)(void), int render_fps,
    int (*update)(void), int update_ups,
    int flags)
{
    /* Register an event to wake up SDL_WaitEvent() on timer interrupts */
    if(ml_event < 0) ml_event = SDL_RegisterEvents(1);

    int mode = (flags & AZUR_MAIN_LOOP_VSYNC) ? 1 : 0;
    int rc = SDL_GL_SetSwapInterval(mode);
    if(rc < 0) {
        azlog(ERROR, "SDL_GL_SetSwapInterval(%d): %s\n", mode, SDL_GetError());
        azlog(ERROR, "Defaulting to non-vsync\n");
    }

    volatile int render_tick = 1;
    volatile int update_tick = 0;
    bool started = false;

    if(mode == 0 || rc < 0) {
        ml_timer_render = SDL_AddTimer(1000/render_fps, handler,
            (void *)&render_tick);
    }
    if(!(flags & AZUR_MAIN_LOOP_TIED)) {
        ml_timer_update = SDL_AddTimer(1000/update_ups, handler,
            (void *)&update_tick);
    }

    while(1) {
        if(update_tick && !(flags & AZUR_MAIN_LOOP_TIED)) {
            update_tick = 0;
            if(update && update()) break;
        }

        /* When vsync is enabled, render() is indirectly blocking so we just
           call it in a straight loop. */
        if(render_tick || (flags & AZUR_MAIN_LOOP_VSYNC)) {
            render_tick = 0;

            /* Tied renders and updates */
            if(started && (flags & AZUR_MAIN_LOOP_TIED)) {
                if(update && update()) break;
            }
            if(render) render();
            started = true;
        }

        /* We wait for an event here. The render_tick and update_tick flags
           control when we call the user functions. In addition to these flags,
           user events with type ml_event are pushed to the event queue to
           "break" SDL_WaitEvent() without us needing to read from the queue
           (which would take events away from the user). */
        SDL_WaitEvent(NULL);
        /* Remove that unneeded event */
        SDL_FilterEvents(filter_event, NULL);
    }

    return 0;
}

static void main_loop_quit(void)
{
    if(ml_timer_render > 0) {
        SDL_RemoveTimer(ml_timer_render);
        ml_timer_render = 0;
    }
    if(ml_timer_update > 0) {
        SDL_RemoveTimer(ml_timer_update);
        ml_timer_update = 0;
    }
}

#endif /* emscripten SDL vs. standard SDL */
