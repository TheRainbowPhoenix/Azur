#include <azur/azur.h>
#include <azur/log.h>
#include <azur/gl/gl.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <string>

static SDL_Window *window = NULL;
static SDL_GLContext glcontext = NULL;

#if AZUR_PLATFORM_EMSCRIPTEN
#include <emscripten/html5.h>

static EM_BOOL fullscreen_callback(int ev, void const *_, void *window0)
{
    double w, h;
    emscripten_get_element_css_size("canvas", &w, &h);

    SDL_Window *window = static_cast<SDL_Window *>(window0);
    SDL_SetWindowSize(window, (int)w, (int)h);

    azlog(INFO, "Canvas size updated: now %dx%d\n", (int)w, (int)h);
    return EM_TRUE;
}

static void enter_fullscreen(SDL_Window *window)
{
    EmscriptenFullscreenStrategy strategy = {
        .scaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_STDDEF,
        .filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_NEAREST,
        .canvasResizedCallback = fullscreen_callback,
        .canvasResizedCallbackUserData = window,
    };
    emscripten_enter_soft_fullscreen("canvas", &strategy);
    azlog(INFO, "Entered fullscreen!\n");
}
#endif /* AZUR_PLATFORM_EMSCRIPTEN */

#if AZUR_GRAPHICS_OPENGL_3_3
static void gl_debug_callback(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length, const GLchar *message, const GLvoid *)
{
    std::string source_str {"OtherSource"};
    std::string type_str {"other-type"};
    std::string severity_str {"?"};
    (void)id;
    (void)length;

    if(severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    if(source == GL_DEBUG_SOURCE_API)               source_str = "API";
    if(source == GL_DEBUG_SOURCE_WINDOW_SYSTEM)     source_str = "WM";
    if(source == GL_DEBUG_SOURCE_SHADER_COMPILER)   source_str = "Compiler";
    if(source == GL_DEBUG_SOURCE_THIRD_PARTY)       source_str = "ThirdParty";
    if(source == GL_DEBUG_SOURCE_APPLICATION)       source_str = "App";

    if(type == GL_DEBUG_TYPE_ERROR)                 type_str = "error";
    if(type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR)   type_str = "deprecation";
    if(type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)    type_str = "ub";
    if(type == GL_DEBUG_TYPE_PORTABILITY)           type_str = "non-portable";
    if(type == GL_DEBUG_TYPE_PERFORMANCE)           type_str = "performance";
    if(type == GL_DEBUG_TYPE_MARKER)                type_str = "marker";
    if(type == GL_DEBUG_TYPE_PUSH_GROUP)            type_str = "push-group";
    if(type == GL_DEBUG_TYPE_POP_GROUP)             type_str = "pop-group";

    if(severity == GL_DEBUG_SEVERITY_HIGH)          severity_str = "high";
    if(severity == GL_DEBUG_SEVERITY_MEDIUM)        severity_str = "medium";
    if(severity == GL_DEBUG_SEVERITY_LOW)           severity_str = "low";
    if(severity == GL_DEBUG_SEVERITY_NOTIFICATION)  severity_str = "note";

    fprintf(stderr, "[OpenGL/%s/%s/%s] %s\n",
        source_str.c_str(), type_str.c_str(), severity_str.c_str(), message);
}
#endif /* AZUR_GRAPHICS_OPENGL_3_3 */

extern void azur_registerResourceGroup_azur();
int azur_init(char const *title, int window_width, int window_height, bool dbg)
{
    azur_registerResourceGroup_azur();

    /* Select the OpenGL profile. This has to come before SDL_Init. */

    #if AZUR_GRAPHICS_OPENGL_3_3
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    if(dbg)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    #elif AZUR_GRAPHICS_OPENGL_ES_2_0
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    #elif AZUR_GRAPHICS_OPENGL_ES_3_0
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    #endif

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

#if AZUR_GRAPHICS_OPENGL_3_3
    rc = gl3wInit();
    if(rc != 0) {
        azlog(FATAL, "gl3wInit: %d\n", rc);
        return 1;
    }
#endif

#if AZUR_GRAPHICS_OPENGL_3_3
    if(dbg) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(gl_debug_callback, nullptr);
    }
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#if AZUR_PLATFORM_EMSCRIPTEN
    enter_fullscreen(window);
#endif

    return 0;
}

namespace azur_internal {
void azur_quit_imgui(void);
}

void azur_quit(void)
{
    // TODO: Replace this with a non-direct call so we don't dl
    azur_internal::azur_quit_imgui();

    if(window) SDL_DestroyWindow(window);
    if(glcontext) SDL_GL_DeleteContext(glcontext);
    IMG_Quit();
    SDL_Quit();

    window = NULL;
    glcontext = NULL;
}

#if AZUR_TOOLKIT_SDL

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
#if AZUR_PLATFORM_EMSCRIPTEN

#include <emscripten.h>
#include <emscripten/html5.h>

struct params {
    std::function<void(void)> const &render;
    std::function<int(void)> const &update;
    bool tied, started;
};

static void nop(void)
{
}

static std::function<int(void)> const *update_int = nullptr;
static void update_void(void)
{
    (*update_int)();
}

static EM_BOOL frame(double time, void *data)
{
    struct params *params = static_cast<struct params *>(data);

    if(params->tied && params->started)
        params->update();

    params->render();
    params->started = true;

    return EM_TRUE;
}

int azur_main_loop(
    std::function<void(void)> const &render, int render_fps,
    std::function<int(void)> const &update, int update_ups,
    int flags)
{
    static struct params p = {
        .render = render,
        .update = update,
        .tied = (flags & AZUR_MAIN_LOOP_TIED) != 0,
        .started = false,
    };

    (void)render_fps;
    emscripten_request_animation_frame_loop(frame, &p);

    if(!(flags & AZUR_MAIN_LOOP_TIED)) {
        update_int = &update;
        /* Setting the infinite loop to true throws an exception which prevents
           this function from returning after starting the loop. */
        emscripten_set_main_loop(update_void, update_ups, true);
    }
    else {
        /* Block this function from returning and leave the RAF loop alone. */
        emscripten_throw_number(0.0);
    }

    return 0;
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
    SDL_zero(e);
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
    std::function<void(void)> const &render, int render_fps,
    std::function<int(void)> const &update, int update_ups,
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
            if(update()) break;
        }

        /* When vsync is enabled, render() is indirectly blocking so we just
           call it in a straight loop. */
        if(render_tick || (flags & AZUR_MAIN_LOOP_VSYNC)) {
            render_tick = 0;

            /* Tied renders and updates */
            if(started && (flags & AZUR_MAIN_LOOP_TIED)) {
                if(update()) break;
            }
            render();
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

    if(ml_timer_render > 0) {
        SDL_RemoveTimer(ml_timer_render);
        ml_timer_render = 0;
    }
    if(ml_timer_update > 0) {
        SDL_RemoveTimer(ml_timer_update);
        ml_timer_update = 0;
    }

    return 0;
}

#endif /* emscripten SDL vs. standard SDL */
