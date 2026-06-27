//---------------------------------------------------------------------------//
//  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                 //
// |  _/__\_  |   Designed by Lephe' and the Planète Casio community.        //
//  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>         //
//---------------------------------------------------------------------------//
// azur.render: Tiled rendering pipeline for 16-bit CASIO calculators
//
// On the CASIO fx-CG and fx-CP, rendering is bottlenecked pretty hard by
// writing the frame data to RAM. Just writing out 396x224 pixels on the fx-CG
// takes 6.1 ms (at default clock speeds).
//
// This Azur rendering pipeline implements tiled rendering [1] so the
// framebuffer can be replaced with smaller fragments (tiles) that fit in the
// much-faster on-chip memory of the SH4AL-DSP, specifically the DSP XRAM and
// YRAM (which total 16 kB).
//
// With this method, draw calls don't render but append to a command queue.
// When all draw calls have been made, the queue is sorted by fragment, and
// then each fragment is rendered and sent to the screen. This results in some
// wonky timings for display refresh times but the non-continuity of the
// display update is mostly invisible, with improved rendering performance.
//
// To fit the system, every rendering program (called "shaders" as in "OpenGL
// fragment shaders") need to render top-to-bottom and take their input from
// the command queue. Azur has utilities to facilitate their implementation.
//
// The prefix for this module is `azrp` for "azur rendering pipeline".
//
// [1] https://en.wikipedia.org/wiki/Tiled_rendering
//---

#pragma once
#include <azur/defs.h>
AZUR_BEGIN_DECLS

#include <gint/display.h>
#include <gint/image.h>
#include <gint/prof.h>

/* arzp_shader_t: Type of shader functions
 * [uniforms] is a pointer to any data the shader might use as uniform.
 * [command] is a structure of the shader's command type.
 * [fragment] is a pointer to azrp_frag. */
typedef void azrp_shader_t(void *uniforms, void *command, void *fragment);

/* azrp_shader_configure_t: Type of shader configuration functions
   This function is mainly called when fragment settings change. */
typedef void azrp_shader_configure_t(void);

/* Video memory fragment used as rendering target (in XRAM/YRAM). */
extern uint16_t *azrp_frag;

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
/* dwindow settings for the display ({ 0, 0, azrp_width, azrp_height }). */
extern struct dwindow azrp_window;

/* azrp_config_scale(): Select the renderer's super-scaling factor

   This pipeline supports integer upscaling by factors of x1, x2 and x3. Unlike
   the traditional VRAM approach, upscaling in this pipeline is fundamentally
   faster on every level, since every bit of graphics data can be handled on an
   actually smaller resolution, leaving the pixel duplication to the display
   transfer. This is because efficient transfers to the display in this system
   are performed by CPU, which is much more versatile than the DMA.

   The settings on each mode are as follow:

   * x1: Display resolution: 396x224
         Fragment size: 16 rows (12672 bytes)
         Number of fragments: 28 (29 if an offset is used)
         Total size of graphics data: 177'408 bytes

   * x2: Display resolution: 198x112
         Fragment size: 16 rows (6336 bytes) # TODO: increase
         Number of fragments 7 (8 if an offset if used)
         Total size of graphics data: 44'352 bytes

   * x3: Display resolution: 132x75 (last row only has 2/3 pixels)
         Fragment size: 16 rows (4224 bytes) # TODO: increase
         Number of fragments: 5 (sometimes 6 if an offset is used)
         Total size of graphics data: 19'800 bytes

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

/* azrp_config_get_line(): Split a line number into fragment/offset

   Sets *fragment to the first fragment that covers line y and *offset to the
   line number within that fragment. */
void azrp_config_get_line(int y, int *fragment, int *offset);

/* azrp_config_get_lines(): Split a line interval into fragments and offset

   Splits the interval [y; y+height) into fragment/offset pairs.
   - Sets *first_fragment to the fragment that covers line y;
   - Sets *first_offset to the line number within that fragment;
   - Sets *fragment_count to the number of fragments the interval will cover. */
void azrp_config_get_lines(int y, int height, int *first_fragment,
                           int *first_offset, int *fragment_count);

//---
// Hooks
//---

/* Hook called before a fragment is sent to the display. The fragment can be
   accessed and modified freeely (however, the time spent in the hook is
   counted as overhead and only part of [azrp_perf_render]). */
typedef void azrp_hook_prefrag_t(int id, void *fragment, int size);

/* Get or set the prefrag hook. */
azrp_hook_prefrag_t *azrp_hook_get_prefrag(void);
void azrp_hook_set_prefrag(azrp_hook_prefrag_t *);

//---
// Standard shaders
//
// None of the functions below acturally draw to the display; they all queue
// commands that get executed when azrp_render_fragment() or azrp_update() is
// called. They all return rather quickly and the time they take executing is
// counted towards command generation, not rendering.
//---

/* azrp_clear(): Clear output with a flat color */
void azrp_clear(uint16_t color);

/* azrp_image(): Render a full image, like dimage(). */
void azrp_image(int x, int y, bopti_image_t const *image);

/* azrp_subimage(): Render a section of an image, like dsubimage(). */
void azrp_subimage(int x, int y, bopti_image_t const *image,
   int left, int top, int width, int height, int flags);

/* See below for more detailed image functions. Dynamic effects are provided
   with the same naming convention as gint. */

/* azrp_triangle(): Render a flat triangle. Points can be in any order. */
void azrp_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int color);

/* azrp_rect(): Render a rectangle with a flat color or color transform. */
void azrp_rect(int x1, int y1, int width, int height, int color_or_effect);

/* Effects for azrp_rect(). */
enum {
  /* Invert colors in gamma space. */
  AZRP_RECT_INVERT = -1,
  /* Darken by halving all components in gamma space. */
  AZRP_RECT_DARKEN = -2,
  /* Whiten by halving the distance to white in gamma space. */
  AZRP_RECT_WHITEN = -3,
};

/* azrp_line(): Draw a straight line with the Bresenham algorithm. */
void azrp_line(int x1, int y1, int x2, int y2, int color);

/* azrp_text(): Render a string of text, like dtext(). */
void azrp_text(int x, int y, int fg, char const *str);

/* azrp_text_opt(): Render text with options similar to dtext_opt(). */
void azrp_text_opt(int x, int y, font_t const *font, int fg, int halign,
                   int valign, char const *str, int size);

/* azrp_print(): Like azrp_text() but with printf-formatting. */
void azrp_print(int x, int y, int fg, char const *fmt, ...);

/* azrp_print_opt(): Like azrp_text_opt() but with printf-formatting. */
void azrp_print_opt(int x, int y, font_t const *font, int fg, int halign,
                    int valign, char const *fmt, ...);

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

/* This counter runs during CPU transfers to the R61523/R61524 display. */
extern prof_t azrp_perf_lcd;

/* This counter runs during rendering; it is the sum of shaders and
   r61524/r61523, plus some logic overhead. */
extern prof_t azrp_perf_render;

/* azrp_perf_clear(): Clear all performance counters
   Generally you want to do this before azrp_update(). */
void azrp_perf_clear(void);

//---
// Definitions for custom shaders
//---

/* azrp_register_shader(): Register a new command type and its shader program

   This function registers a new shader program to the program array, along
   with its configuration function. The configuration function is called
   immediately upon registration.

   There is often a choice between creating a new shader and generalizing an
   existing one. The impact is small; the difference only really matters if
   there is a lot of commands, but in that case command management becomes a
   stronger bottleneck. The choice should be made for optimal code structure
   and reuse.

   Returns the shader ID to be set in commands, or -1 if the maximum number of
   shaders has been exceeded. */
int azrp_register_shader(azrp_shader_t *program,
                         azrp_shader_configure_t *configure);

/* azrp_set_uniforms(): Set a shader's uniforms pointer

   If the shader has less than 4 bytes of uniform data, an integer may be
   passed as the address; there is no requirement that the pointer be aligned
   or even points to valid memory. */
void azrp_set_uniforms(int shader_id, void *uniforms);

/* Setup the command queue. This can be called when intializing Azur or
   in-between frames to change the way commands are stored.
   * The index size determines the maximum number of draw calls that can be
     made (specificially the number of commands, counting fragments).
   * The data size limits the total size of draw calls' parameter structures.
     Currently it can't be set higher than 64 kiB.
   Returns true on success, false if an allocation error occurs or the
   parameters are invalid.

   If either limit is exceeded while preparing a frame, commands will stop
   being recorded, leading to an incomplete frame being displayed.

   If this function is not called when setting up Azur, the queue will be
   automatically setup upon first use with the default settings below.

   TODO: azrp_cmdq_setup: expose default sizes?
   TODO: azrp_cmdq_setup: use vram, split index, etc. */
bool azrp_cmdq_setup(int index_size, int data_size);
/* Default queue settings */
#define AZRP_CMDQ_DEFAULT_INDEX_SIZE 1024
#define AZRP_CMDQ_DEFAULT_DATA_SIZE 32768

/* Create and queue a new command for next frame. This allocates `size` bytes
   in the command data buffer and returns a pointer. Caller should then fill
   the command. The command is queued for fragments in the interval [fragment..
   fragment+count). Returns NULL if the command queue index is out of space.

   The command's data must start with an 8-bit shader ID; everything else is up
   to the shader. The command data will be preserved in-between calls to the
   shader fragment function and can be modified. */
void *azrp_cmdq_command(size_t size, int fragment, int count);

/* Lower-level functions for allocating a variable-length command in two steps.
   First, azrp_cmdq_alloc returns a pointer and the maximum possible size, then
   the caller fills the command and, when done, indicates its chosen size with
   azrp_cmdq_finalize. No other allocation must take place in-between the calls
   to azrp_cmdq_alloc and azrp_cmdq_finalize. The command is later queued to
   fragments with azrp_cmdq_queue. See the text shader for an example.

   azrp_cmdq_alloc allocates memory for a command of size at least `size` (the
   fixed part in the command structure), returns a writable pointer, and sets
   the amount of memory available beyond `size` in `*extra` (for the variable-
   length part). NULL is returned if `size` doesn't fit. The caller must check
   that the variable-length data is shorter than `*extra` bytes and **MUST
   NOT** write beyond this limit.

   azrp_cmdq_finalize finalizes the allocation; the caller must specify the
   total size (both fixed and variable-length). If it turns out that `*extra`
   is too small for the variable-length part, forget about the command and
   don't call azrp_cmdq_finalize. Then you can continue making draw calls.

   If the number of fragments affected is known in advance, it can be specified
   in `fragment_count` to enable some early checks. You can always set it to 0
   and wait for the checks in azrp_cmdq_queue.
   TODO: Fragment count check in azrp_cmdq_alloc won't support split index */
void *azrp_cmdq_alloc(size_t size, int *extra, int fragment_count);
bool azrp_cmdq_finalize(void const *command, int total_size);

/* Queue an allocated command for a range of fragments. This fills the command
   queue index for fragments in the range [fragment .. fragment+count). Returns
   true on success, false if the command queue index is out of space.

   In the rare case that a command should be queued for a set of fragments
   that's not an interval, the command can be allocated with azrp_cmdq_alloc
   and azrp_cmdq_finalized (even if not variable-length), then queued in
   multiple calls to this function. azrp_cmdq_command doesn't allow that. */
bool azrp_cmdq_queue(void const *command, int fragment, int count);

__attribute__((deprecated("Use azrp_cmdq_command")))
void *azrp_new_command(size_t size, int fragment, int count);
__attribute__((deprecated("Use azrp_cmdq_alloc")))
void *azrp_alloc_command(size_t size, int *extra, int count);
__attribute__((deprecated("Use azrp_cmdq_finalize")))
bool azrp_finalize_command(void const *command, int total_size);
__attribute__((deprecated("Use azrp_cmdq_queue")))
bool azrp_instantiate_command(void const *command, int fragment, int count);

/* Split and queue a gint image command. The command must have been completely
   prepared with gint_image_mkcmd() and have had its color effect sections
   filled. This function sets the shader ID and adjusts the command for tiled
   rendering. This is mostly an internal function. */
void azrp_queue_image(struct gint_image_box *box, image_t const *img,
                      struct gint_image_cmd *cmd);

//---
// Internal R61524 functions
//---

void azrp_r61524_fragment_x1(void *fragment, int size);

void azrp_r61524_fragment_x2(void *fragment, int width, int height);

//---
// Internal R61523 functions
//---

void azrp_r61523_fragment_x1(void *fragment, int size);

void azrp_r61523_fragment_x2(void *fragment, int width, int height);

//---
// Internal functions for the image shader
//
// We use gint's image rendering API but replace some of the core loops with
// Azur-specific versions that are faster in the CPU-bound context of this
// rendering engine. Some of the main loops from Azur actually perform better
// in RAM than bopti used to do, and are already in gint.
//---

/* azrp_image_effect(): Generalized azrp_image() with dynamic effects */
#define azrp_image_effect(x, y, img, eff, ...) \
    azrp_image_effect(x, y, img, 0, 0, (img)->width, (img)->height, eff, \
                    ##__VA_ARGS__)
/* azrp_subimage_effect(): Generalized azrp_subimage() with dynamic effects */
void azrp_subimage_effect(int x, int y, image_t const *img,
    int left, int top, int w, int h, int effects, ...);

/* Specific versions for each format */
#define AZRP_IMAGE_SIG1(NAME, ...) \
    void azrp_image_ ## NAME(int x, int y, image_t const *img,##__VA_ARGS__); \
    void azrp_subimage_ ## NAME(int x, int y, image_t const *img, \
        int left, int top, int w, int h, ##__VA_ARGS__);
#define AZRP_IMAGE_SIG(NAME, ...) \
    AZRP_IMAGE_SIG1(rgb16 ## NAME, ##__VA_ARGS__) \
    AZRP_IMAGE_SIG1(p8 ## NAME, ##__VA_ARGS__) \
    AZRP_IMAGE_SIG1(p4 ## NAME, ##__VA_ARGS__)

AZRP_IMAGE_SIG(_effect, int effects, ...)
AZRP_IMAGE_SIG(, int effects)
AZRP_IMAGE_SIG(_clearbg, int effects, int bg_color_or_index)
AZRP_IMAGE_SIG(_swapcolor, int effects, int source, int replacement)
AZRP_IMAGE_SIG(_addbg, int effects, int bg_color)
AZRP_IMAGE_SIG(_dye, int effects, int dye_color)

#define azrp_image_rgb16_effect(x, y, img, eff, ...) \
    azrp_subimage_rgb16_effect(x, y, img, 0, 0, (img)->width, (img)->height, \
                             eff, ##__VA_ARGS__)
#define azrp_image_p8_effect(x, y, img, eff, ...) \
    azrp_subimage_p8_effect(x, y, img, 0, 0, (img)->width, (img)->height, \
        eff, ##__VA_ARGS__)
#define azrp_image_p4_effect(x, y, img, eff, ...) \
    azrp_subimage_p4_effect(x, y, img, 0, 0, (img)->width, (img)->height, \
        eff, ##__VA_ARGS__)

#undef AZRP_IMAGE_SIG
#undef AZRP_IMAGE_SIG1

/* Main loop provided by Azur; as usual, these are not real functions; their
   only use is as the [.loop] field of a command. */

void azrp_image_shader_rgb16_normal(void);
void azrp_image_shader_rgb16_clearbg(void);
void azrp_image_shader_rgb16_swapcolor(void);
void azrp_image_shader_rgb16_dye(void);
void azrp_image_shader_p8_normal(void);
void azrp_image_shader_p8_swapcolor(void);
void azrp_image_shader_p4_normal(void);
void azrp_image_shader_p4_clearbg(void);

AZUR_END_DECLS
