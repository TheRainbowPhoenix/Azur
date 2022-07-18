//---
// azur.azur: Main functions
//---

#pragma once
#include <azur/defs.h>
AZUR_BEGIN_DECLS

/* azur_init(): Initialize the engine's subsystems.

   Initializes either the SDL with OpenGL/OpenGLES, or gint. Returns 0 on
   success, non-zero on failure. Resources allocated by azur_init() are
   automatically destroyed by a destructor.

   On GINT_CG, the window size is fixed to 396x224 and ignored at this stage.
   The rendering system can still be configured for super-resolution with
   azrp_config_scale() from <azur/gint/render.h>. */
int azur_init(char const *title, int window_width, int window_height);

/* azur_main_loop(): Run the update/render loop.

   This function runs the main loop, which regularly calls two different
   functions for frame renders (usually redraws and commits the screen) and
   application updates (usually reads SDL/gint events and runs simulations).

   Renders and updates can either be triggered independently by timers with
   different frequencies, or be triggered together. Additionally, frame renders
   might be synchronized with display refresh.

   -> When using standard SDL, both can be set independently, and vsync is
      supported (if it succeeds at runtime, otherwise a fallback is used).
   -> When using emscripten's ported SDL, update frequency can be set with a
      timer, but frame renders are always synchronized with the browser's
      refresh because it is the only sensible and supported option.
   -> When using gint, both can be set independently; there is no vertical
      synchronization but full-speed can be enabled (as it makes sense there).

   The render framerate is determined first. If AZUR_MAIN_LOOP_FULLSPEED is
   set, renders and updates alternate at full speed (gint only). Otherwise, if
   AZUR_MAIN_LOOP_VSYNC is set (forced on emscripten), vsync is used. If vsync
   fails, or neither flag was specified, the target FPS is used. Thus the
   render FPS should always be specified.

   The update framerate is determined second. If AZUR_MAIN_LOOP_TIED is set,
   updates are set to run before renders (except before the very first render),
   and update_ups is ignored. Otherwise, the target UPS is used.

   The main loop stops whenever update() returns non-zero. */
int azur_main_loop(
   void (*render)(void), int render_fps,
   int (*update)(void), int update_ups,
   int flags);

/* Render loop is synchronized with display refresh. */
#define AZUR_MAIN_LOOP_VSYNC       0x01
/* Both loops run at full speed with no delay (relevant on gint). */
#define AZUR_MAIN_LOOP_UNLIMITED   0x02
/* Update loop is tied to the render loop. */
#define AZUR_MAIN_LOOP_TIED        0x04

//---
// Global information
//---

#ifdef AZUR_TOOLKIT_SDL
#include <SDL2/SDL.h>

/* azur_sdl_window(): Get the current SDL window. */
SDL_Window *azur_sdl_window(void);

#endif /* AZUR_TOOLKIT_SDL */

AZUR_END_DECLS
