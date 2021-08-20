#include <azur/gint/render.h>

#include <gint/drivers/r61524.h>
#include <gint/defs/attributes.h>

#include <string.h>
#include <stdlib.h>

#define YRAM ((void *)0xe5017000)

/* 8 rows of video memory, occupying 6338/8192 bytes of XRAM. */
GXRAM GALIGNED(32) uint16_t azrp_frag[DWIDTH * 8];

/* Super-scaling factor, width and height of output. */
int azrp_scale;
int azrp_width, azrp_height;
/* Offset of first fragment for alignment, and number of fragments. */
int azrp_frag_offset;
int azrp_frag_count;

/* Number and total size of queued commands. */
GXRAM int commands_count = 0, commands_length = 0;

/* Array of pointers to queued commands (stored as an offset into YRAM). */
GXRAM uint16_t commands_array[AZRP_MAX_COMMANDS];

/* Array of shader programs and uniforms. */
static azrp_shader_t *shaders[AZRP_MAX_SHADERS] = { NULL };
static void *shader_uniforms[AZRP_MAX_SHADERS] = { NULL };

/* Next free index in the shader program array. */
GXRAM static uint16_t shaders_next = 0;

/* Performance counters. */
GXRAM prof_t azrp_perf_cmdgen;
GXRAM prof_t azrp_perf_sort;
GXRAM prof_t azrp_perf_shaders;
GXRAM prof_t azrp_perf_r61524;
GXRAM prof_t azrp_perf_render;

//---
// High and low-level pipeline functions
//---

void azrp_clear_commands(void)
{
    commands_count = 0;
    commands_length = 0;
}

static int compare_commands(void const *c1, void const *c2)
{
    uint16_t offset1 = *(uint16_t *)c1;
    uint16_t offset2 = *(uint16_t *)c2;

    uint8_t *ptr1 = (uint8_t *)(0xe5017000 + offset1);
    uint8_t *ptr2 = (uint8_t *)(0xe5017000 + offset2);

    int diff_fragments = (int)ptr1[1] - (int)ptr2[1];
    if(diff_fragments) return diff_fragments;

    return (int)offset1 - (int)offset2;
}

void azrp_sort_commands(void)
{
    prof_enter(azrp_perf_sort);
    /* TODO: azrp_sort_commands: Use a custom sorter */
    qsort(commands_array, commands_count, sizeof commands_array[0],
        compare_commands);
    prof_leave(azrp_perf_sort);
}

void azrp_render_fragments(void)
{
    prof_enter(azrp_perf_render);

    int i = 0;
    int frag = 0;

    uint8_t *cmd = (uint8_t *)YRAM + commands_array[i];

    prof_enter(azrp_perf_r61524);
    r61524_start_frame(0, 244);
    prof_leave(azrp_perf_r61524);

    while(1) {
        if(cmd[1] == frag) {
            if(shaders[cmd[0]]) {
                prof_enter(azrp_perf_shaders);
                shaders[cmd[0]](shader_uniforms[cmd[0]], cmd, azrp_frag);
                prof_leave(azrp_perf_shaders);
            }
            cmd = YRAM + commands_array[++i];
        }
        else {
            prof_enter(azrp_perf_r61524);
            xram_frame(azrp_frag, 396 * 8);
            prof_leave(azrp_perf_r61524);
            frag++;
            if(frag >= azrp_frag_count) break;
        }
    }

    prof_leave(azrp_perf_render);
}

void azrp_update(void)
{
    azrp_sort_commands();
    azrp_render_fragments();
    azrp_clear_commands();
}

//---
// Configuration calls
//---

static void update_frag_count(void)
{
    if(azrp_scale == 1)
        azrp_frag_count = 28 + (azrp_frag_offset > 0);
    else if(azrp_scale == 2)
        azrp_frag_count = 7 + (azrp_frag_offset > 0);
    else if(azrp_scale == 3)
        azrp_frag_count = 5 + (azrp_frag_offset > 5);
}

static void update_size(void)
{
    if(azrp_scale == 1)
        azrp_width = 396, azrp_height = 198;
    else if(azrp_scale == 2)
        azrp_width = 198, azrp_height = 112;
    else if(azrp_scale == 3)
        azrp_width = 132, azrp_height = 75;
}

void azrp_config_scale(int scale)
{
    if(scale < 1 || scale > 3)
        return;

    azrp_scale = scale;
    update_size();
    update_frag_count();
}

void azrp_config_frag_offset(int offset)
{
    if(offset < 0)
        return;

    azrp_frag_offset = offset;
    update_frag_count();
}

__attribute__((constructor))
static void default_settings(void)
{
    azrp_config_scale(1);
}

//---
// Custom shaders
//---

int azrp_register_shader(azrp_shader_t *program)
{
    int id = shaders_next;

    if(id >= AZRP_MAX_SHADERS)
        return -1;

    shaders[shaders_next++] = program;
    return id;
}

void azrp_set_uniforms(int shader_id, void *uniforms)
{
    if((unsigned int)shader_id >= AZRP_MAX_SHADERS)
        return;
    if(shaders[shader_id] == NULL)
        return;

    shader_uniforms[shader_id] = uniforms;
}

bool azrp_queue_command(void *command, size_t size)
{
    if(commands_count >= AZRP_MAX_COMMANDS)
        return false;
    if(commands_length + size >= 8192)
        return false;

    uint8_t *dst = YRAM + commands_length;
    uint8_t *src = command;

    for(size_t i = 0; i < size; i++)
        dst[i] = src[i];

    commands_array[commands_count++] = commands_length;
    commands_length += size;

    return true;
}

//---
// Performance indicators
//---

/* Also run this function once at startup */
GCONSTRUCTOR
void azrp_perf_clear(void)
{
    azrp_perf_cmdgen  = prof_make();
    azrp_perf_sort    = prof_make();
    azrp_perf_shaders = prof_make();
    azrp_perf_r61524  = prof_make();
    azrp_perf_render  = prof_make();
}
