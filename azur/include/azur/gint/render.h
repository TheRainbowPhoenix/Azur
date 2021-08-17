//---
// azur.render: Specialized rendering pipeline for fx-CG gint
//
// On-chip ILRAM and DSP memory bring out the full power of the SH4AL-DSP.
// Therefore, optimal performance in a game's renderer will rely on using on-
// chip memory instead of standard RAM for graphics data.
//
// The obvious limitation of on-chip memory is its size (20 kiB total), which
// is much smaller than a full-resolution image (~177 kiB). This prompts for a
// technique known as "frame streaming", where fragments (strips of VRAM) of
// each frame are rendered and transferred to the display in sequence.
// Reasonable efficiency and suitable display driver settings can prevent
// tearing even though data is not being sent continuously.
//
// The main components of this rendering pipeline are the command queue and
// fragment shaders.
//
// The command queue stores all rendering commands, split into fragments. Each
// fragment needs to read through a number of commands, and the order does not
// match the order of API calls because each API call typically impacts several
// fragments. Therefore commands need to be stored.
//
// Fragment shaders are the programs that render commands into graphics data
// for each fragments. They are pretty similar to OpenGL shaders, in that they
// receive vectors of objects (command parameters) and produce graphics data to
// fragments (even though each fragment is a strip of VRAM, not a pixel), hence
// the name.
//
// The prefix for this module is [azrp] for "azur rendering pipeline".
//---

#pragma once
#include <azur/defs.h>
AZUR_BEGIN_DECLS

#include <gint/defs/types.h>
#include <gint/display.h>

#include <libprof.h>

/* arzp_shader_t: Type of shader functions
   * [uniforms] is a pointer to any data the shader might use as uniform.
   * [command] is a structure of the shader's command type. */
typedef void azrp_shader_t(void *uniforms, void *command);

/* Video memory fragment used as rendering target (in XRAM). */
extern uint16_t azrp_frag[];

/* Maximum number of commands that can be queued. (This is only one of two
   limits, the other being the size of the command data.) */
#define AZRP_MAX_COMMANDS 512

/* Maximum number of shaders that can be defined. (This is a loose limit). */
#define AZRP_MAX_SHADERS 32

//---
// High and low-level pipeline functions
//
// The process of rendering a frame with azrp has four steps:
// 1. Clear the command queue
// 2. Queue commands with command generation functions
// 3. Sort the command queue
// 4. Render fragments in on-chip memory and send them to the display driver
//
// The command queue is empty when the program starts. The azrp_update()
// performs steps 3 and 4, rendering a frame; then clears the command queue
// again. Therefore, azrp_update() can be used more or less like dupdate().
//
// Functions for command generation are listed in the shader API below, and can
// be extended with custom shaders.
//
// Applications that want to render the same frame several time such as to
// save screenshots, or reuse commands, or add new commands and use the
// already-sorted base, can use the low-level functions below which implement
// steps 1, 3 and 4 individually.
//---

/* azrp_update(): Sort commands, render a frame, and starts another one */
void azrp_update(void);

/* azrp_clear_commands(): Clear the command queue (step 1) */
void azrp_clear_commands(void);

/* azrp_sort_commands(): Sort the command queue (step 3) */
void azrp_sort_commands(void);

/* azrp_render_fragments(): Render and send fragments to the dislay (step 4) */
void azrp_render_fragments(void);

//---
// Standard shaders
//---

enum {
    /* Clears the entire output with a single color */
    AZRP_SHADER_CLEAR = 0,
    /* Renders RGB565 textures/images */
    AZRP_SHADER_TEX2D,

    /* First user-attributable ID */
    AZRP_SHADER_USER,
};

/* azrp_clear(): Clear output [ARZP_SHADER_CLEAR] */
void azrp_clear(uint16_t color);

/* azrp_image(): Queue image command [AZRP_SHADER_TEX2D] */
void azrp_image(int x, int y, uint16_t *pixels, int w, int h, int stride);

//---
// Performance indicators
//
// The following performance counters are run through by the rendering module
// in most stages of the rendering process. The module updates them but doesn't
// use them, so they are safe to write to and reset when they're not running.
//---

/* This counter runs during command generation and queue operations. */
extern prof_t azrp_perf_cmdgen;

/* This counter runs during the command sorting step. */
extern prof_t azrp_perf_sort;

/* This counter runs during shader executions in arzp_render_fragments(). */
extern prof_t azrp_perf_shaders;

/* This counter runs during CPU transfers to the R61524 display. */
extern prof_t azrp_perf_r61524;

/* This counter runs during the whole azrp_update() operation; it is the sum of
   sort, shaders, r61524, plus some logic overhead. */
extern prof_t azrp_perf_render;

/* azrp_perf_clear(): Clear all performance counters
   Generally you want to do this before azrp_update(). */
void azrp_perf_clear(void);

//---
// Definitions for custom shaders
//---

/* azrp_register_shader(): Register a new command type and its shader program

   This function adds the specified shader program to the program array, and
   returns the corresponding command type (which is AZRP_SHADER_USER plus some
   value). Adding new shaders is useful for specialized rendering options (eg.
   tiles with fixed size) or new graphical effects.

   If the maximum number shaders is exceeded, returns -1. */
int azrp_register_shader(azrp_shader_t *program);

/* azrp_queue_command(): Add a new command to be rendered next frame

   The command must be a structure starting with an 8-bit shader ID and an
   8-bit fragment ID.

   Returns true on success, false if the maximum amount of commands or command
   memory is exceeded. */
bool azrp_queue_command(void *command, size_t size);

//---
// Internal shader definitions (for reference; no API guarantee)
//---

struct azrp_shader_tex2d_command {
    /* Shader ID and fragment number */
    uint8_t shader_id;
    uint8_t fragment_id;
    /* Pixels per line */
    int16_t columns;
    /* Already offset by start row and column */
    void *input;
    /* Destination in XRAM (offset) */
    uint16_t output;
    /* Number of lines */
    int16_t lines;
    /* Distance between two lines (columns excluded) */
    int16_t stride;

} GPACKED(2);

AZUR_END_DECLS
