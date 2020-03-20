#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdnoreturn.h>

#define STBDS_NO_SHORT_NAMES
#include <vendor/stb/stb_ds.h>

#define HANDMADE_MATH_NO_SSE
#include <vendor/Handmade-Math/HandmadeMath.h>

#include "gfx.h"
#include "phx.h"
#include "ui.h"

#define SAVE_FILE "state/game.data"

typedef struct {
    size_t vert_count;
    hmm_v2 verts[PHX_TERRAIN_MAX_VERTS];
    material mat;
} terrain_data;

typedef struct {
    material mat;
    phx_handle phx;
} terrain_handle;

static struct {
    material terrain_material;
    terrain_handle *terrain;
} world;

static void terrain_append(terrain_data data) {
    phx_handle phx = phx_append_terrain(data.verts, data.vert_count);
    terrain_handle handle = {
        .phx = phx,
        .mat = data.mat,
    };
    stbds_arrput(world.terrain, handle);
}

static void terrain_clear(void) {
    phx_clear_terrain();
    stbds_arrsetlen(world.terrain, 0);
}

static void terrain_render(void) {
    for (size_t i = 0; i < stbds_arrlenu(world.terrain); i++) {
        terrain_handle handle = world.terrain[i];
        terrain_data data = {
            .mat = handle.mat,
        };
        phx_query_vertices(handle.phx, data.verts, &data.vert_count);
        for (size_t i = 0; i < data.vert_count; i++) {
            hmm_v2 a = data.verts[i];
            hmm_v2 b = data.verts[(i + 1) % data.vert_count];
            gfx_append_shape(a, b, data.mat);
        }
    }
}

static void load(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        return;
    }
    fread(&world.terrain_material, sizeof(world.terrain_material), 1, file);
    terrain_data data;
    while (fread(&data, sizeof(data), 1, file)) {
        terrain_append(data);
    }
    fclose(file);
}

static void save(const char *path) {
    FILE *file = fopen(path, "w");
    if (!file) {
        return;
    }
    fwrite(&world.terrain_material, sizeof(world.terrain_material), 1, file);
    for (size_t i = 0; i < stbds_arrlenu(world.terrain); i++) {
        terrain_handle handle = world.terrain[i];
        terrain_data data = {
            .mat = handle.mat,
        };
        phx_query_vertices(handle.phx, data.verts, &data.vert_count);
        fwrite(&data, sizeof(data), 1, file);
    }
    fclose(file);
}

static void init(void) {
    world.terrain_material = (material){
        .type = MAT_DIFFUSE,
        .color = HMM_Vec3(1.0, 1.0, 1.0),
    };

    stm_setup();
    gfx_setup();
    ui_setup();
    phx_create();
    load(SAVE_FILE);
}

static double get_frame_time(void) {
    static uint64_t last_time;
    return stm_ms(stm_laptime(&last_time));
}

static const char *get_frame_time_str(void) {
    static char b[64];
    snprintf(b, sizeof(b), "%.2lf ms", get_frame_time());
    return b;
}

static const char *get_material_str(void) {
    switch (world.terrain_material.type) {
    case MAT_DIFFUSE:
        return "M: Diffuse";
    case MAT_REFLECT:
        return "M: Reflect";
    case MAT_LIGHT:
        return "M: Light";
    default:
        return "M: Unknown";
    }
}

static void hud_render(void) {
    mu_begin(&ui.ctx);
    if (mu_begin_window_ex(&ui.ctx, "", mu_rect(10, 10, 200, 200), MU_OPT_NOFRAME | MU_OPT_NOTITLE)) {
        mu_label(&ui.ctx, get_frame_time_str());
        mu_label(&ui.ctx, get_material_str());
        mu_slider(&ui.ctx, &world.terrain_material.color.R, 0.0f, 1.0f);
        mu_slider(&ui.ctx, &world.terrain_material.color.G, 0.0f, 1.0f);
        mu_slider(&ui.ctx, &world.terrain_material.color.B, 0.0f, 1.0f);
        mu_draw_rect(&ui.ctx,
                     mu_layout_next(&ui.ctx),
                     (mu_Color){
                         .r = world.terrain_material.color.R * 255,
                         .g = world.terrain_material.color.G * 255,
                         .b = world.terrain_material.color.B * 255,
                         .a = 255,
                     });
        mu_end_window(&ui.ctx);
    }
    mu_end(&ui.ctx);
}

static void frame(void) {
    int width = sapp_width();
    int height = sapp_height();
    int dpi_scale = sapp_dpi_scale();

    phx_simulate();
    hud_render();
    gfx_clear_shapes();
    terrain_render();
    gfx_render(width, height, dpi_scale);
}

static bool mouse_buttons[] = {
    [SAPP_MOUSEBUTTON_LEFT] = false,
    [SAPP_MOUSEBUTTON_RIGHT] = false,
    [SAPP_MOUSEBUTTON_MIDDLE] = false,
};

static void on_mouse_down(const sapp_event *event) {
    mouse_buttons[event->mouse_button] = true;

    float x = event->mouse_x / sapp_dpi_scale();
    float y = event->mouse_y / sapp_dpi_scale();
    (void)x;
    (void)y;
}

static void on_mouse_up(const sapp_event *event) {
    mouse_buttons[event->mouse_button] = false;

    float x = event->mouse_x; // / sapp_dpi_scale();
    float y = event->mouse_y; // / sapp_dpi_scale();
    static terrain_data ter;

    if (event->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        ter.verts[ter.vert_count] = HMM_Vec2(x, y);
        ter.vert_count++;
        if (ter.vert_count == 3) {
            ter.vert_count = 0;
            ter.mat = world.terrain_material;
            terrain_append(ter);
        }
    }
}

static void on_mouse_move(const sapp_event *event) {
    float x = event->mouse_x / sapp_dpi_scale();
    float y = event->mouse_y / sapp_dpi_scale();
    (void)x;
    (void)y;
}

static void on_key_up(const sapp_event *event) {
    switch (event->key_code) {
    case SAPP_KEYCODE_ESCAPE:
        sapp_quit();
        break;
    case SAPP_KEYCODE_D:
        gfx_dump();
        break;
    case SAPP_KEYCODE_C:
        terrain_clear();
        break;
    default:
        break;
    }
}

static void on_key_down(const sapp_event *event) {
    switch (event->key_code) {
    case SAPP_KEYCODE_1:
        world.terrain_material.type = MAT_DIFFUSE;
        break;
    case SAPP_KEYCODE_2:
        world.terrain_material.type = MAT_REFLECT;
        break;
    case SAPP_KEYCODE_3:
        world.terrain_material.type = MAT_LIGHT;
        break;
    default:
        break;
    }
}

static void event(const sapp_event *event) {
    ui_event(event);
    switch (event->type) {
    case SAPP_EVENTTYPE_MOUSE_MOVE:
        if (!ui_is_hovered()) {
            on_mouse_move(event);
        }
        break;
    case SAPP_EVENTTYPE_MOUSE_DOWN:
        if (!ui_is_hovered()) {
            on_mouse_down(event);
        }
        break;
    case SAPP_EVENTTYPE_MOUSE_UP:
        if (!ui_is_hovered()) {
            on_mouse_up(event);
        }
        break;
    case SAPP_EVENTTYPE_KEY_UP:
        on_key_up(event);
        break;
    case SAPP_EVENTTYPE_KEY_DOWN:
        on_key_down(event);
        break;
    default:
        break;
    }
}

static void cleanup(void) {
    save(SAVE_FILE);
    phx_destroy();
    ui_shutdown();
    gfx_shutdown();
}

sapp_desc sokol_main(int argc, char *argv[]) {
    (void)(argc);
    (void)(argv);
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = event,
        .cleanup_cb = cleanup,
        .width = 800,
        .height = 800,
        .high_dpi = true,
        .window_title = "Light",
    };
}
