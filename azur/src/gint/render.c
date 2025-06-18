#include <azur/gint/render.h>

#include <gint/drivers/r61524.h>
#include <gint/defs/attributes.h>
#include <gint/defs/util.h>

#include <string.h>
#include <stdlib.h>

/* 16 rows of video memory, occupying 12736/16384 bytes or XYRAM (77.7%). */
u16 *azrp_frag = (void *)0xe500e000 + 32;

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

/* Shader program information. */
typedef struct {
    /* Rendering function. */
    azrp_shader_t *shader;
    /* Uniform parameter. */
    void *uniform;
    /* Configuration function (in response to scale, base offset, etc). */
    azrp_shader_configure_t *configure;

} shader_info_t;

/* Array of shader programs. */
static shader_info_t shaders[AZRP_MAX_SHADERS] = { 0 };

/* Next free index in the shader program array. */
static u16 shaders_next = 0;

/* Hooks. */
static azrp_hook_prefrag_t *azrp_hook_prefrag = NULL;

/* Performance counters. */
prof_t azrp_perf_cmdgen;
prof_t azrp_perf_sort;
prof_t azrp_perf_shaders;
prof_t azrp_perf_r61524;
prof_t azrp_perf_render;

//=== Command queue ==========================================================//
// TODO: Azur command queue: Explore index pre-split by fragment, no sorting

/* Command index; each element is a command defined by
   - Bits 31..16:  Fragment number
   - Bits 15..00:  Offset into command data buffer (bytes)
   This is chosen so that rendering order is the normal order of integers. */
struct azrp_cmdq_index { u32 *buf; int capacity; int cursor; };
static struct azrp_cmdq_index azrp_cmdq_index = { 0 };

/* Command data buffer; this is an untyped concatenation of command bytes,
   referenced by offset in the queue index. It functions as a bump allocator,
   shared for all fragments and shaders. */
struct azrp_cmdq_data { u8 *buf; int capacity; int cursor; };
static struct azrp_cmdq_data azrp_cmdq_data = { 0 };

/* Default command queue parameters. */
#define AZRP_CMDQ_DEFAULT_INDEX_SIZE 1024
#define AZRP_CMDQ_DEFAULT_DATA_SIZE 32768

/* Allocate or re-allocate the command queue to specific dimensions.
   TODO: Absolute limit on 64 kB cmdq size due to encoding of index! */
bool azrp_cmdq_create(int index_capacity, int data_capacity)
{
    struct azrp_cmdq_index *i = &azrp_cmdq_index;
    struct azrp_cmdq_data *d = &azrp_cmdq_data;
    bool ok = true;

    if(data_capacity > 0x10000)
        return false;

    if(index_capacity != i->capacity) {
        free(i->buf);
        i->buf = malloc(index_capacity * sizeof *i->buf);
        i->capacity = i->buf ? index_capacity : 0;
        i->cursor = 0;
        ok &= i->buf != NULL;
    }
    if(data_capacity != d->capacity) {
        free(d->buf);
        d->buf = malloc(data_capacity);
        d->capacity = d->buf ? data_capacity : 0;
        d->cursor = 0;
        ok &= d->buf != NULL;
    }

    return ok;
}

/* Free the command queue. */
void azrp_cmdq_destroy(void)
{
    free(azrp_cmdq_index.buf);
    memset(&azrp_cmdq_index, 0, sizeof azrp_cmdq_index);

    free(azrp_cmdq_data.buf);
    memset(&azrp_cmdq_data, 0, sizeof azrp_cmdq_data);
}

/* Custom quick-sort implementation for the command index. */
static void azrp_cmdq_sort_index_aux(u32 *index, int low, int high)
{
    if(low >= high)
        return;

    u32 pivot = index[(low + high) >> 1];
    int i = low - 1;
    int j = high + 1;

    while(1) {
        do i++;
        while(index[i] < pivot);

        do j--;
        while(index[j] > pivot);

        if(i >= j)
            break;

        u32 tmp = index[i];
        index[i] = index[j];
        index[j] = tmp;
    }

    azrp_cmdq_sort_index_aux(index, low, j);
    azrp_cmdq_sort_index_aux(index, j+1, high);
}
GINLINE static void azrp_cmdq_sort_index(void)
{
    struct azrp_cmdq_index *i = &azrp_cmdq_index;
    return azrp_cmdq_sort_index_aux(i->buf, 0, i->cursor - 1);
}

/* Allocate a command from the data buffer. */
void *azrp_cmdq_alloc(size_t size, int *extra, int count)
{
    struct azrp_cmdq_data *d = &azrp_cmdq_data;
    *extra = d->capacity - d->cursor - size;

    if(*extra < 0) {
        /* If queue is empty, auto-allocate with the defaults and retry */
        if(azrp_cmdq_create(AZRP_CMDQ_DEFAULT_INDEX_SIZE,
                            AZRP_CMDQ_DEFAULT_DATA_SIZE))
            return azrp_cmdq_alloc(size, extra, count);
        return NULL;
    }

    return d->buf + d->cursor;
}

bool azrp_cmdq_finalize(void const *command, int size)
{
    struct azrp_cmdq_data *d = &azrp_cmdq_data;
    size = (size + 3) & -4;
    (void)command;

    if(d->cursor + size > d->capacity)
        return false;

    d->cursor += size;
    return true;
}

bool azrp_cmdq_queue(void const *command, int fragment, int count)
{
    struct azrp_cmdq_index *i = &azrp_cmdq_index;
    if(i->cursor + count > i->capacity)
        return false;

    int offset = (u8 *)command - azrp_cmdq_data.buf;

    do {
        i->buf[i->cursor++] = (fragment << 16) | offset;
        fragment++;
    }
    while(--count > 0);

    return true;
}

void *azrp_cmdq_command(uint size, int fragment, int count)
{
    int extra;
    void *cmd = azrp_cmdq_alloc(size, &extra, count);
    if(!cmd)
        return NULL;
#if 0
    if(!azrp_cmdq_queue(cmd, fragment, count))
        return NULL;

    azrp_cmdq_finalize(cmd, size);
#else
    azrp_cmdq_finalize(cmd, size);
    azrp_cmdq_queue(cmd, fragment, count);
#endif
    return cmd;
}

//---
// High and low-level pipeline functions
//---

void azrp_clear_commands(void)
{
    azrp_cmdq_index.cursor = 0;
    azrp_cmdq_data.cursor = 0;
}

void azrp_sort_commands(void)
{
    prof_enter_norec(azrp_perf_sort);
    azrp_cmdq_sort_index();
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
    uint32_t cmd = azrp_cmdq_index.buf[i];

    prof_enter_norec(azrp_perf_r61524);
    r61524_start_frame(0, DWIDTH-1, 0, DHEIGHT-1);
    prof_leave_norec(azrp_perf_r61524);

    while(1) {
        while(cmd < next_frag_threshold && i < azrp_cmdq_index.cursor) {
            azrp_commands_total++;
            uint8_t *data = azrp_cmdq_data.buf + (cmd & 0xffff);
            shader_info_t const *info = &shaders[data[0]];

            prof_enter_norec(azrp_perf_shaders);
            info->shader(info->uniform, data, azrp_frag);
            prof_leave_norec(azrp_perf_shaders);

            cmd = azrp_cmdq_index.buf[++i];
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
        // TODO: r61524 x3 output function
        prof_leave_norec(azrp_perf_r61524);

        if(++frag >= azrp_frag_count) break;
        next_frag_threshold += (1 << 16);
    }

    prof_leave_norec(azrp_perf_render);
}

void azrp_update(void)
{
    if(!azrp_cmdq_index.buf || !azrp_cmdq_data.buf)
        return;

    azrp_sort_commands();
    azrp_render_fragments();
    azrp_clear_commands();
}

//---
// Configuration calls
//---

static void reconfigure_all_shaders(void)
{
    for(int i = 0; i < shaders_next; i++) {
        if(shaders[i].configure)
            shaders[i].configure();
    }
}

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
    reconfigure_all_shaders();
}

void azrp_config_frag_offset(int offset)
{
    if(offset < 0)
        return;

    azrp_frag_offset = offset;
    update_frag_count();
    reconfigure_all_shaders();
}

/* Make sure this constructor runs before every shader's registration
   constructor so we don't configure registered shaders before the settings are
   initialized. */
__attribute__((constructor(101)))
static void default_settings(void)
{
    azrp_config_scale(1);
}

void azrp_config_get_line(int y, int *fragment, int *offset)
{
    y += azrp_frag_offset;

    *fragment = y >> 4;
    *offset = y & 15;
}

void azrp_config_get_lines(int y, int height, int *first_fragment,
    int *first_offset, int *fragment_count)
{
    y += azrp_frag_offset;

    *first_fragment = (y >> 4);
    *first_offset = (y & 15);
    *fragment_count = ((y + height - 1) >> 4) - *first_fragment + 1;
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

int azrp_register_shader(azrp_shader_t *program,
    azrp_shader_configure_t *configure)
{
    int id = shaders_next;
    if(id >= AZRP_MAX_SHADERS)
        return -1;

    shader_info_t *info = &shaders[id];
    info->shader = program;
    info->uniform = NULL;
    info->configure = configure;
    shaders_next++;

    return id;
}

void azrp_set_uniforms(int shader_id, void *uniforms)
{
    if((unsigned int)shader_id >= AZRP_MAX_SHADERS)
        return;
    shaders[shader_id].uniform = uniforms;
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
