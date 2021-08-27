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
   * [command] is a structure of the shader's command type.
   * [fragment] is a pointer to azrp_frag. */
typedef void azrp_shader_t(void *uniforms, void *command, void *fragment);

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
//
// There are optional configuration calls that can be performed within step 1;
// the configuration is retained from frame to frame, so it may be enough to
// set it only once.
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
// Configuration calls
//
// The following configuration options can be changed during step 1, so either
// along with arzp_sort_commands(), or just after azrp_update().
//
// Changing display settings usually requires updating the uniforms of shaders.
// See the details of each shader.
//
// Most settings are exposed as global variables. This is for read-only access;
// if you modify the variables directly you will get garbage.
//---

/* Current super-scaling factor. */
extern int azrp_scale;
/* Width and height of display (based on scale). */
extern int azrp_width, azrp_height;
/* Number of fragments in a frame (affected by configuration). */
extern int azrp_frag_count;
/* Offset of first fragment. */
extern int azrp_frag_offset;
/* Height of fragments. */
extern int azrp_frag_height;

/* azrp_config_scale(): Select the renderer's super-scaling factor

   This pipeline supports integer upscaling by factors of x1, x2 and x3. Unlike
   the traditional VRAM approach, upscaling in this pipeline is fundamentally
   faster on every level, since every bit of graphics data can be handled on an
   actually smaller resolution, leaving the pixel duplication to the display
   transfer. This is because efficient transfers to the display in this system
   are performed by CPU, which is much more versatile than the DMA.

   The settings on each mode are as follow:

   * x1: Display resolution: 396x224
         Fragment size: 8 rows (6336 bytes)
         Number of fragments: 28 (29 if an offset is used)
         Total size of graphics data: 177.408 kB

   * x2: Display resolution: 198x112
         Fragment size: 16 rows (6336 bytes)
         Number of fragments 7 (8 if an offset if used)
         Total size of graphics data: 44.352 kB

   * x3: Display resolution: 132x75 (last row only has 2/3 pixels)
         Fragment size: 16 rows (4224 bytes)
         Number of fragments: 5 (sometimes 6 if an offset is used)
         Total size of graphics data: 19.800 kB

   As one would know when playing modern video games, super-resolution is one
   of the most useful ways to increase performance. The reduced amount of
   graphics data (either 4 or 9 times the fullscreen amount) has a huge impact
   on the rendering process. */
void azrp_config_scale(int scale);

/* azrp_config_frag_offset(): Offset fragments along the y-axis

   This call changes the alignment of fragments along the y-axis, so that the
   first fragments starts somewhere above the screen. This tends to add one
   additional fragment for the whole screen to be covered.

   The primary use of this feature is to align grid-based frames with
   fragments. As a prototypical example, top-down games using a tileset spend
   most of the display surface showing the tiled map, so it's pretty beneficial
   to align fragments on map rows so that each fragment handles only one row,
   which makes the shader simpler and faster, uses less commands, and even
   simplifies memory access patterns a little bit.

   Another use is to align the x3 mode roughly to the center of the screen, to
   emulate the 128x64 resolution of black-and-white models with 4 fragments.

   @offset  Fragment offset along the y-axis (0 ... height of fragment-1). */
void azrp_config_frag_offset(int offset);

//---
// Standard shaders
//---

 /* Clears the entire output with a single color */
extern uint8_t AZRP_SHADER_CLEAR;
 /* Renders RGB565 textures/images */
extern uint8_t AZRP_SHADER_TEX2D;

/* azrp_clear(): Clear output [ARZP_SHADER_CLEAR] */
void azrp_clear(uint16_t color);

/* azrp_image(): Queue image command [AZRP_SHADER_TEX2D] */
void azrp_image(int x, int y, bopti_image_t const *image);

/* azrp_subimage(): Queue image subsection command [AZRP_SHADER_TEX2D] */
void azrp_subimage(int x, int y, bopti_image_t const *image,
   int left, int top, int width, int height, int flags);

/* Functions to update uniforms for these shaders. You should call them when:
   * AZRP_SHADER_CLEAR: Changing super-scaling settings.
   * AZRP_SHADER_TEX2D: Changing super-scaling or or fragment offsets. */
void azrp_shader_clear_configure(void);
void azrp_shader_tex2d_configure(void);

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
   returns the corresponding command type. Adding new shaders is useful for
   specialized rendering options (eg. tiles with fixed size) or new graphical
   effects.

   If the maximum number shaders is exceeded, returns -1. */
int azrp_register_shader(azrp_shader_t *program);

/* azrp_set_uniforms(): Set a shader's uniforms pointer

   If the shader has less than 4 bytes of uniform data, an integer may be
   passed as the address; there is no requirement that the pointer be aligned
   or even points to valid memory. */
void azrp_set_uniforms(int shader_id, void *uniforms);

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
    /* Address of the image structure */
    bopti_image_t const *image;
    /* Destination in XRAM (offset) */
    uint16_t output;
    /* Number of lines */
    int16_t lines;
    /* Already offset by start row and column */
    void const *input;
};

AZUR_END_DECLS
