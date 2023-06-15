#include <azur/gint/render.h>
#include <gint/defs/util.h>
#include <gint/display.h>
#include <string.h>

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
   3. Precompute the set of glyphs so the list can be reused when crossing
      fragment boundaries, the shader can be written entirely in assembler, and
      the command can possibly be reused?
   4. Provide noclip toplevel functions, which I believe should provide a
      nontrivial speed boost. */

void azrp_text_glyph(uint16_t *fragment, uint32_t const *data, int color,
    int height, int width, int stride, int index);

struct command {
    uint8_t shader_id;
    uint8_t _;
    int16_t x, y;
    int16_t height, top;
    font_t const *font;
    char const *str;
    int fg;
    int size;
};

void azrp_shader_text(void *uniforms0, void *cmd0, void *frag0)
{
    struct command *cmd = cmd0;
    int x = cmd->x;
    int y = cmd->y;
    font_t const *f = cmd->font;
    int fg = cmd->fg;
    int size = cmd->size;
    /* Storage height, top position within glyph */
    int height = min(cmd->height, azrp_frag_height - y);
    int top = cmd->top;

    uint8_t const *str = (void *)cmd->str;
    uint8_t const *str0 = str;

    /* Raw glyph data */
    uint32_t const *data = f->data;

    /* Update for next fragment */
    cmd->height -= height;
    cmd->top += height;
    cmd->y = 0;

    /* Move to top row */
    uint16_t *frag = (uint16_t *)frag0 + azrp_width * y;

    /* Read each character from the input string */
    while(x < azrp_window.right)
    {
        uint32_t code_point = dtext_utf8_next(&str);
        if(!code_point || (size >= 0 && str - str0 > size)) break;

        int glyph = dfont_glyph_index(f, code_point);
        if(glyph < 0) continue;

        int dataw = f->prop ? f->glyph_width[glyph] : f->width;

        int index = dfont_glyph_offset(f, glyph);

        /* Compute horizontal intersection between glyph and screen */

        int width = dataw, left = 0;

        if(x + dataw <= azrp_window.left)
        {
            x += dataw + f->char_spacing;
            continue;
        }
        if(x < azrp_window.left) {
            left = azrp_window.left - x;
            width -= left;
        }
        width = min(width, azrp_window.right - x);

        /* Render glyph */
        azrp_text_glyph(frag + x + left, data + index, fg, height, width,
            dataw - width, top * dataw + left);

        x += dataw + f->char_spacing;
    }
}

void azrp_text(int x, int y, font_t const *f, char const *str,
    int fg, int size)
{
    prof_enter(azrp_perf_cmdgen);

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

    struct command *cmd =
        azrp_new_command(sizeof *cmd, frag_first, frag_count);
    if(!cmd) {
        prof_leave(azrp_perf_cmdgen);
        return;
    }

    cmd->shader_id = AZRP_SHADER_TEXT;
    cmd->x = x;
    cmd->y = first_offset;
    cmd->height = height;
    cmd->top = top;
    cmd->font = f;
    cmd->str = str;
    cmd->fg = fg;
    cmd->size = size;

    prof_leave(azrp_perf_cmdgen);
}

void azrp_text_opt(int x, int y, font_t const *font, int fg, int halign,
    int valign, char const *str, int size)
{
    if(halign != DTEXT_LEFT || valign != DTEXT_TOP) {
        int w, h;
        dnsize(str, size, font, &w, &h);

        if(halign == DTEXT_RIGHT)  x -= w - 1;
        if(halign == DTEXT_CENTER) x -= (w >> 1);
        if(valign == DTEXT_BOTTOM) y -= h - 1;
        if(valign == DTEXT_MIDDLE) y -= (h >> 1);
    }

    azrp_text(x, y, font, str, fg, size);
}
