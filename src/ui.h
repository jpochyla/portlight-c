#ifndef INCLUDE_UI
#define INCLUDE_UI

#include <stdlib.h>
#include <string.h>

#define SOKOL_METAL
#include <vendor/sokol/sokol_app.h>
#include <vendor/sokol/sokol_gfx.h>
#include <vendor/sokol/util/sokol_gl.h>

#include <vendor/microui/src/microui.h>

#include <vendor/microui/demo/atlas.inl>

static struct {
    mu_Context ctx;
    sg_image atlas_img;
    sgl_pipeline pipeline;
} ui;

static void r_init(void) {
    // Atlas image data is in atlas.inl file, this only contains alpha
    // values, need to expand this to RGBA8.
    uint32_t rgba8_size = ATLAS_WIDTH * ATLAS_HEIGHT * 4;
    uint32_t *rgba8_pixels = (uint32_t *)malloc(rgba8_size);
    for (int y = 0; y < ATLAS_HEIGHT; y++) {
        for (int x = 0; x < ATLAS_WIDTH; x++) {
            uint32_t index = y * ATLAS_WIDTH + x;
            rgba8_pixels[index] = 0x00FFFFFF | ((uint32_t)atlas_texture[index] << 24);
        }
    }
    ui.atlas_img = sg_make_image(&(sg_image_desc){
        .width = ATLAS_WIDTH,
        .height = ATLAS_HEIGHT,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .content = {.subimage[0][0] = {.ptr = rgba8_pixels, .size = rgba8_size}},
    });

    ui.pipeline = sgl_make_pipeline(&(sg_pipeline_desc){
        .blend = {.enabled = true,
                  .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                  .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA},
    });

    free(rgba8_pixels);
}

static void r_begin(float width, float height, float dpi_scale) {
    sgl_defaults();
    sgl_push_pipeline();
    sgl_load_pipeline(ui.pipeline);
    sgl_enable_texture();
    sgl_texture(ui.atlas_img);
    sgl_matrix_mode_projection();
    sgl_push_matrix();
    sgl_ortho(0.0f, width / dpi_scale, height / dpi_scale, 0.0f, -1.0f, +1.0f);
    sgl_begin_quads();
}

static void r_end(void) {
    sgl_end();
    sgl_pop_matrix();
    sgl_pop_pipeline();
}

static void r_draw(void) {
    sgl_draw();
}

static void r_push_quad(mu_Rect dst, mu_Rect src, mu_Color color) {
    float u0 = (float)src.x / (float)ATLAS_WIDTH;
    float v0 = (float)src.y / (float)ATLAS_HEIGHT;
    float u1 = (float)(src.x + src.w) / (float)ATLAS_WIDTH;
    float v1 = (float)(src.y + src.h) / (float)ATLAS_HEIGHT;

    float x0 = (float)dst.x;
    float y0 = (float)dst.y;
    float x1 = (float)(dst.x + dst.w);
    float y1 = (float)(dst.y + dst.h);

    sgl_c4b(color.r, color.g, color.b, color.a);
    sgl_v2f_t2f(x0, y0, u0, v0);
    sgl_v2f_t2f(x1, y0, u1, v0);
    sgl_v2f_t2f(x1, y1, u1, v1);
    sgl_v2f_t2f(x0, y1, u0, v1);
}

static void r_draw_rect(mu_Rect rect, mu_Color color) {
    r_push_quad(rect, atlas[ATLAS_WHITE], color);
}

static void r_draw_text(const char *text, mu_Vec2 pos, mu_Color color) {
    mu_Rect dst = {pos.x, pos.y, 0, 0};
    for (const char *p = text; *p; p++) {
        mu_Rect src = atlas[ATLAS_FONT + (unsigned char)*p];
        dst.w = src.w;
        dst.h = src.h;
        r_push_quad(dst, src, color);
        dst.x += dst.w;
    }
}

static void r_draw_icon(int id, mu_Rect rect, mu_Color color) {
    mu_Rect src = atlas[id];
    int x = rect.x + (rect.w - src.w) / 2;
    int y = rect.y + (rect.h - src.h) / 2;
    r_push_quad(mu_rect(x, y, src.w, src.h), src, color);
}

static int r_get_text_width(const char *text, int len) {
    int res = 0;
    for (const char *p = text; *p && len--; p++) {
        res += atlas[ATLAS_FONT + (unsigned char)*p].w;
    }
    return res;
}

static int r_get_text_height(void) {
    return 18;
}

static void r_set_clip_rect(mu_Rect rect) {
    sgl_end();
    sgl_scissor_rect(rect.x, rect.y, rect.w, rect.h, true);
    sgl_begin_quads();
}

static int text_width_cb(mu_Font font, const char *text, int len) {
    (void)font;
    if (len < 0) {
        len = (int)strlen(text);
    }
    return r_get_text_width(text, len);
}

static int text_height_cb(mu_Font font) {
    (void)font;
    return r_get_text_height();
}

static void ui_setup(void) {
    sgl_setup(&(sgl_desc_t){});
    r_init();
    mu_init(&ui.ctx);
    ui.ctx.text_width = text_width_cb;
    ui.ctx.text_height = text_height_cb;
}

static void ui_shutdown(void) {
    sg_destroy_image(ui.atlas_img);
    sgl_destroy_pipeline(ui.pipeline);
    sgl_shutdown();
}

static void ui_render(float width, float height, float dpi_scale) {
    mu_Command *cmd = NULL;
    r_begin(width, height, dpi_scale);
    while (mu_next_command(&ui.ctx, &cmd)) {
        switch (cmd->type) {
        case MU_COMMAND_TEXT:
            r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color);
            break;
        case MU_COMMAND_RECT:
            r_draw_rect(cmd->rect.rect, cmd->rect.color);
            break;
        case MU_COMMAND_ICON:
            r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color);
            break;
        case MU_COMMAND_CLIP:
            r_set_clip_rect(cmd->clip.rect);
            break;
        }
    }
    r_end();
    r_draw();
}

static void ui_event(const sapp_event *ev) {

    static const char key_map[SAPP_KEYCODE_MENU] = {
        [SAPP_KEYCODE_LEFT_SHIFT] = MU_KEY_SHIFT,
        [SAPP_KEYCODE_RIGHT_SHIFT] = MU_KEY_SHIFT,
        [SAPP_KEYCODE_LEFT_CONTROL] = MU_KEY_CTRL,
        [SAPP_KEYCODE_RIGHT_CONTROL] = MU_KEY_CTRL,
        [SAPP_KEYCODE_LEFT_ALT] = MU_KEY_ALT,
        [SAPP_KEYCODE_RIGHT_ALT] = MU_KEY_ALT,
        [SAPP_KEYCODE_ENTER] = MU_KEY_RETURN,
        [SAPP_KEYCODE_BACKSPACE] = MU_KEY_BACKSPACE,
    };

    float s = sapp_dpi_scale();
    int x = (int)(ev->mouse_x / s);
    int y = (int)(ev->mouse_y / s);

    switch (ev->type) {
    case SAPP_EVENTTYPE_MOUSE_DOWN:
        mu_input_mousedown(&ui.ctx, x, y, (1 << ev->mouse_button));
        break;
    case SAPP_EVENTTYPE_MOUSE_UP:
        mu_input_mouseup(&ui.ctx, x, y, (1 << ev->mouse_button));
        break;
    case SAPP_EVENTTYPE_MOUSE_MOVE:
        mu_input_mousemove(&ui.ctx, x, y);
        break;
    case SAPP_EVENTTYPE_MOUSE_SCROLL:
        mu_input_scroll(&ui.ctx, (int)ev->scroll_x / s, (int)ev->scroll_y / s);
        break;
    case SAPP_EVENTTYPE_KEY_DOWN:
        mu_input_keydown(&ui.ctx, key_map[ev->key_code & 511]);
        break;
    case SAPP_EVENTTYPE_KEY_UP:
        mu_input_keyup(&ui.ctx, key_map[ev->key_code & 511]);
        break;
    case SAPP_EVENTTYPE_CHAR: {
        char txt[2] = {ev->char_code & 255, 0};
        mu_input_text(&ui.ctx, txt);
    } break;
    default:
        break;
    }
}

static bool ui_is_hovered(void) {
    return ui.ctx.hover != 0;
}

#endif
