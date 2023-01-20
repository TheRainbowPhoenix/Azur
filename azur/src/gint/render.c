#include <azur/gint/render.h>

#include <gint/drivers/r61524.h>
#include <gint/defs/attributes.h>
#include <gint/defs/util.h>

#include <string.h>
#include <stdlib.h>

/* 16 rows of video memory, occupying 12736/16384 bytes or XYRAM (77.7%). */
uint16_t *azrp_frag = (void *)0xe500e000 + 32;

/* Super-scaling factor, width and height of output. */
int azrp_scale;
int azrp_width, azrp_height;
/* Offset of first fragment for alignment, and number of fragments. */
int azrp_frag_offset;
int azrp_frag_count;
/* Height of fragment. */
int azrp_frag_height;
/* dwindow settings for the display ({ 0, 0, azrp_width, azrp_height }). */
struct dwindow azrp_window;

/* Number and total size of queued commands. */
static int commands_count=0, commands_length=0;

/* Array of pointers to queued commands. Each command has:
   * Top 16 bits: fragment number
   * Bottom 16 bits: offset into command data buffer
   Rendering order is integer order. */
static uint32_t commands_array[AZRP_MAX_COMMANDS];

static GALIGNED(4) uint8_t commands_data[16384];

/* Array of shader programs and uniforms. */
static azrp_shader_t *shaders[AZRP_MAX_SHADERS] = { NULL };
static void *shader_uniforms[AZRP_MAX_SHADERS] = { NULL };

/* Next free index in the shader program array. */
static uint16_t shaders_next = 0;

/* Hooks. */
static azrp_hook_prefrag_t *azrp_hook_prefrag = NULL;

/* Performance counters. */
prof_t azrp_perf_cmdgen;
prof_t azrp_perf_sort;
prof_t azrp_perf_shaders;
prof_t azrp_perf_r61524;
prof_t azrp_perf_render;

//---
// High and low-level pipeline functions
//---

void azrp_clear_commands(void)
{
    commands_count = 0;
    commands_length = 0;
}

/* Custom quick sort for commands */

static void cmdsort(int low, int high)
{
    if(low >= high) return;

    uint32_t pivot = commands_array[(low + high) >> 1];

    int i = low - 1;
    int j = high + 1;

    while(1) {
        do i++;
        while(commands_array[i] < pivot);

        do j--;
        while(commands_array[j] > pivot);

        if(i >= j) break;

        uint32_t tmp = commands_array[i];
        commands_array[i] = commands_array[j];
        commands_array[j] = tmp;
    }

    cmdsort(low, j);
    cmdsort(j+1, high);
}

void azrp_sort_commands(void)
{
    prof_enter_norec(azrp_perf_sort);
    cmdsort(0, commands_count - 1);
    prof_leave_norec(azrp_perf_sort);
}

int azrp_commands_total;

void azrp_render_fragments(void)
{
    prof_enter_norec(azrp_perf_render);

    azrp_commands_total = 0;

    int i = 0;
    int frag = 0;
    uint32_t next_frag_threshold = (frag + 1) << 16;
    uint32_t cmd = commands_array[i];

    prof_enter_norec(azrp_perf_r61524);
    r61524_start_frame(0, DWIDTH-1, 0, DHEIGHT-1);
    prof_leave_norec(azrp_perf_r61524);

    while(1) {
        while(cmd < next_frag_threshold && i < commands_count) {
            azrp_commands_total++;
            uint8_t *data = commands_data + (cmd & 0xffff);
            prof_enter_norec(azrp_perf_shaders);
            shaders[data[0]](shader_uniforms[data[0]], data, azrp_frag);
            prof_leave_norec(azrp_perf_shaders);
            cmd = commands_array[++i];
        }

        if(azrp_hook_prefrag) {
            int size = azrp_width * azrp_frag_height * 2;
            (*azrp_hook_prefrag)(frag, azrp_frag, size);
        }

        prof_enter_norec(azrp_perf_r61524);
        if(azrp_scale == 1)
            azrp_r61524_fragment_x1(azrp_frag, 396 * azrp_frag_height);
        else if(azrp_scale == 2)
            azrp_r61524_fragment_x2(azrp_frag, azrp_width, azrp_frag_height);
        prof_leave_norec(azrp_perf_r61524);

        if(++frag >= azrp_frag_count) break;
        next_frag_threshold += (1 << 16);
    }

    prof_leave_norec(azrp_perf_render);
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

// TODO: Use larger fragments in upscales x2 and x3

static void update_frag_count(void)
{
    if(azrp_scale == 1)
        azrp_frag_count = 14 + (azrp_frag_offset > 0);
    else if(azrp_scale == 2)
        azrp_frag_count = 7 + (azrp_frag_offset > 0);
    else if(azrp_scale == 3)
        azrp_frag_count = 5 + (azrp_frag_offset > 5);
}

static void update_size(void)
{
    if(azrp_scale == 1)
        azrp_width = 396, azrp_height = 224, azrp_frag_height = 16;
    else if(azrp_scale == 2)
        azrp_width = 198, azrp_height = 112, azrp_frag_height = 16;
    else if(azrp_scale == 3)
        azrp_width = 132, azrp_height = 75,  azrp_frag_height = 16;

    azrp_window = (struct dwindow){ 0, 0, azrp_width, azrp_height };
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
// Hooks
//---

azrp_hook_prefrag_t *azrp_hook_get_prefrag(void)
{
    return azrp_hook_prefrag;
}

void azrp_hook_set_prefrag(azrp_hook_prefrag_t *hook)
{
    azrp_hook_prefrag = hook;
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

bool azrp_queue_command(void *command, size_t size, int fragment, int count)
{
    if(commands_count + count > AZRP_MAX_COMMANDS)
        return false;
    if(commands_length + size >= sizeof command_data)
        return false;

    uint8_t *dst = commands_data + commands_length;
    uint8_t *src = command;

    for(size_t i = 0; i < size; i++)
        dst[i] = src[i];

    do {
        commands_array[commands_count++] = (fragment << 16) | commands_length;
        fragment++;
    }
    while(--count > 0);

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
