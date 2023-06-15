#include <azur/gint/render.h>
#include <gint/defs/util.h>
#include <gint/display.h>
#include <string.h>
#include <stdio.h>

uint8_t AZRP_SHADER_TEXT = -1;

__attribute__((constructor))
static void register_shader(void)
{
    extern azrp_shader_t azrp_shader_text;
    AZRP_SHADER_TEXT = azrp_register_shader(azrp_shader_text, NULL);
}

//---

/* This version of the text shader is a very rough adaptation of dtext(). It
   should be optimized in several important ways in the future:
   1. Use left-shifting in azrp_text_glyph(), which is probably faster both for
      partial and full glyphs.
   2. Optimize the heck out of the full-width case, which is almost every
      single call.
   3. Provide noclip toplevel functions, which I believe should provide a
      nontrivial speed boost. */

void azrp_text_glyph(uint16_t *fragment, uint32_t const *data, int color,
    int height, int width, int stride, int index);

struct command {
    uint8_t shader_id;
    uint8_t y;
    int16_t x;

    int16_t height, top;

    uint16_t fg;
    int16_t glyph_count;

    font_t const *font;

    /* TODO
    uint8_t first_left, first_dataw;
    uint8_t last_left, last_dataw; */

    /* TODO: We use two entries per glyph; offset and data width. Can we do
       something that doesn't require both of these? */
    uint16_t glyphs[];
};

void azrp_shader_text(void *uniforms0, void *cmd0, void *frag0)
{
    (void)uniforms0;
    struct command *cmd = cmd0;
    int x = cmd->x;
    font_t const *f = cmd->font;
    int fg = cmd->fg;
    /* Storage height, top position within glyph */
    int height = min(cmd->height, azrp_frag_height - cmd->y);
    int top = cmd->top;

    uint16_t *frag = (uint16_t *)frag0 + azrp_width * cmd->y;

    /* Update for next fragment */
    cmd->height -= height;
    cmd->top += height;
    cmd->y = 0;

    int glyph_count = cmd->glyph_count;
    uint16_t *glyphs = cmd->glyphs;

    /* Read each character from the input string */
    do {
        int dataw = *glyphs++;
        int index = *glyphs++;
        glyph_count -= 2;

        /* Compute horizontal intersection between glyph and screen */
        int width = dataw, left = 0;

        if(x < azrp_window.left) {
            left = azrp_window.left - x;
            width -= left;
        }
        width = min(width, azrp_window.right - x);

        /* Render glyph */
        azrp_text_glyph(frag + x + left, f->data + index, fg, height, width,
            dataw - width, top * dataw + left);

        x += dataw + f->char_spacing;
    } while(glyph_count);
}

void azrp_text_opt(int x, int y, font_t const *f, int fg, int halign,
    int valign, char const *str0, int size)
{
    prof_enter(azrp_perf_cmdgen);

    if(!f) {
        f = dfont(NULL);
        dfont(f);
    }

    if(halign != DTEXT_LEFT || valign != DTEXT_TOP) {
        int w, h;
        dnsize(str0, size, f, &w, &h);

        if(halign == DTEXT_RIGHT)  x -= w - 1;
        if(halign == DTEXT_CENTER) x -= (w >> 1);
        if(valign == DTEXT_BOTTOM) y -= h - 1;
        if(valign == DTEXT_MIDDLE) y -= (h >> 1);
    }

    /* Clipping */
    if(x >= azrp_window.right || y >= azrp_window.bottom ||
       y + f->data_height <= azrp_window.top) {
        prof_leave(azrp_perf_cmdgen);
        return;
    }

    int top_overflow = y - azrp_window.top;
    int top = 0;
    int height = f->data_height;

    if(top_overflow < 0) {
        top = -top_overflow;
        height += top_overflow;
        y -= top_overflow;
    }
    height = min(height, azrp_window.bottom - y);
    if(height <= 0) {
        prof_leave(azrp_perf_cmdgen);
        return;
    }

    int frag_first, first_offset, frag_count;
    azrp_config_get_lines(y, f->data_height,
        &frag_first, &first_offset, &frag_count);

    int extra;
    struct command *cmd = azrp_alloc_command(sizeof *cmd, &extra, frag_count);
    if(!cmd) {
        prof_leave(azrp_perf_cmdgen);
        return;
    }

    cmd->shader_id = AZRP_SHADER_TEXT;
    cmd->x = x;
    cmd->y = first_offset;
    cmd->glyph_count = 0;
    cmd->height = height;
    cmd->top = top;
    cmd->font = f;
    cmd->fg = fg;

    uint8_t const *str = (void *)str0;
    uint8_t const *str_end = (size >= 0) ? str + size : (void *)-1;

    /* Compute the list of glyphs to be rendered */
    while(x < azrp_window.right) {
        uint32_t code_point = dtext_utf8_next(&str);
        if(!code_point || str > str_end) break;

        int glyph = dfont_glyph_index(f, code_point);
        if(glyph < 0) continue;

        int dataw = f->prop ? f->glyph_width[glyph] : f->width;
        int index = dfont_glyph_offset(f, glyph);

        if((cmd->glyph_count + 1) * (int)sizeof *cmd->glyphs > extra) {
            prof_leave(azrp_perf_cmdgen);
            return;
        }

        /* Glyph is entirely left clipped: skip it */
        if(x + dataw <= azrp_window.left) {
            x += dataw + f->char_spacing;
            cmd->x = x;
            continue;
        }

        cmd->glyphs[cmd->glyph_count++] = dataw;
        cmd->glyphs[cmd->glyph_count++] = index;
        x += dataw + f->char_spacing;
    }
    if(cmd->glyph_count == 0) {
        prof_leave(azrp_perf_cmdgen);
        return;
    }

    azrp_finalize_command(cmd, sizeof *cmd + 2 * cmd->glyph_count);
    azrp_instantiate_command(cmd, frag_first, frag_count);
    prof_leave(azrp_perf_cmdgen);
}

void azrp_text(int x, int y, int fg, char const *str)
{
    azrp_text_opt(x, y, NULL, fg, DTEXT_LEFT, DTEXT_TOP, str, -1);
}

void azrp_print(int x, int y, int fg, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char str[128];
    vsnprintf(str, sizeof str, fmt, args);
    va_end(args);

    azrp_text_opt(x, y, NULL, fg, DTEXT_LEFT, DTEXT_TOP, str, -1);
}

void azrp_print_opt(int x, int y, font_t const *font, int fg, int halign,
    int valign, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char str[128];
    vsnprintf(str, sizeof str, fmt, args);
    va_end(args);

    azrp_text_opt(x, y, font, fg, halign, valign, str, -1);
}
