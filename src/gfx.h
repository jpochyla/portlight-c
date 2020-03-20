#ifndef INCLUDE_GFX
#define INCLUDE_GFX

#include <stdio.h>
#include <stdlib.h>

#define HANDMADE_MATH_NO_SSE
#include <vendor/Handmade-Math/HandmadeMath.h>

#define SOKOL_METAL
#include <vendor/sokol/sokol_app.h>
#include <vendor/sokol/sokol_gfx.h>
#include <vendor/sokol/sokol_glue.h>
#include <vendor/sokol/sokol_time.h>

#include "ui.h"
#include "utils.h"
#include "shd_trace.h"
#include "shd_screen.h"

#define GFX_SHAPE_MAX_COUNT 512
#define GFX_SHAPE_VERTS_PER_PIXEL 2
#define GFX_SHAPE_VERTS 2

typedef enum {
    MAT_DIFFUSE = 0,
    MAT_REFLECT = 1,
    MAT_LIGHT = 2,
} material_type;

typedef struct {
    hmm_vec3 color;
    material_type type;
} material;

typedef void (*gfx_shape_func)(hmm_v2 a, hmm_v2 b, material m, void *data);

static struct {
    struct {
        size_t sample_count;
        sg_image targets[2];
        sg_pass passes[2];
        vs_trace_params_t vsp;
        fs_trace_params_t fsp;
        sg_shader shader;
        sg_pipeline pipeline;
        sg_bindings bindings;
    } trace;
    struct {
        vs_screen_params_t vsp;
        sg_pass_action pass_action;
        sg_buffer vertices;
        sg_buffer indices;
        sg_shader shader;
        sg_pipeline pipeline;
        sg_bindings bindings;
    } screen;
} gfx;

static void gfx_setup(void) {
    sg_setup(&(sg_desc){
        .context = sapp_sgcontext(),
    });
    float width = sapp_width() / sapp_dpi_scale();
    float height = sapp_height() / sapp_dpi_scale();

    gfx.screen.pass_action = (sg_pass_action){
        .colors[0] = {.action = SG_ACTION_CLEAR, .val = {0.0f, 0.0f, 0.0f, 1.0f}},
    };
    float screen_vertices[] = {
        0.0f,
        0.0f,
        0.0f,
        0.0f, // 0
        width,
        0.0f,
        1.0f,
        0.0f, // 1
        width,
        height,
        1.0f,
        1.0f, // 2
        0.0f,
        height,
        0.0f,
        1.0f, // 3
    };
    gfx.screen.vertices = sg_make_buffer(&(sg_buffer_desc){
        .type = SG_BUFFERTYPE_VERTEXBUFFER,
        .size = sizeof(screen_vertices),
        .content = screen_vertices,
        .label = "screen-vertices",
    });
    uint16_t screen_indices[] = {0, 1, 2, 0, 2, 3};
    gfx.screen.indices = sg_make_buffer(&(sg_buffer_desc){
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .size = sizeof(screen_indices),
        .content = screen_indices,
        .label = "screen-indices",
    });

    sg_image_desc target_desc = {
        .render_target = true,
        .width = sapp_width(),
        .height = sapp_height(),
        .pixel_format = SG_PIXELFORMAT_RGBA32F,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .label = "trace-target",
    };
    gfx.trace.sample_count = 0;
    gfx.trace.targets[0] = sg_make_image(&target_desc);
    gfx.trace.targets[1] = sg_make_image(&target_desc);
    gfx.trace.passes[0] = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = gfx.trace.targets[0],
        .label = "trace-pass",
    });
    gfx.trace.passes[1] = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = gfx.trace.targets[1],
        .label = "trace-pass",
    });
    gfx.trace.shader = sg_make_shader(sh_trace_shader_desc());
    gfx.trace.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = gfx.trace.shader,
        .index_type = SG_INDEXTYPE_UINT16,
        .blend =
            {
                .color_format = SG_PIXELFORMAT_RGBA32F,
                .depth_format = SG_PIXELFORMAT_NONE,
            },
        .layout = {.attrs =
                       {
                           [ATTR_vs_trace_position].format = SG_VERTEXFORMAT_FLOAT2,
                           [ATTR_vs_trace_uv0].format = SG_VERTEXFORMAT_FLOAT2,
                       }},
        .label = "trace-pipeline",
    });
    gfx.trace.bindings = (sg_bindings){
        .vertex_buffers = {gfx.screen.vertices},
        .index_buffer = gfx.screen.indices,
    };

    gfx.screen.shader = sg_make_shader(sh_screen_shader_desc());
    gfx.screen.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = gfx.screen.shader,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {.attrs =
                       {
                           [ATTR_vs_screen_position].format = SG_VERTEXFORMAT_FLOAT2,
                           [ATTR_vs_screen_uv0].format = SG_VERTEXFORMAT_FLOAT2,
                       }},
        .label = "screen-pipeline",
    });
    gfx.screen.bindings = (sg_bindings){
        .vertex_buffers = {gfx.screen.vertices},
        .index_buffer = gfx.screen.indices,
    };
}

static void gfx_dump(void) {
    for (size_t i = 0; i < gfx.trace.fsp.shape_count; i++) {
        hmm_v4 s = gfx.trace.fsp.shape_vertices[i];
        hmm_v4 m = gfx.trace.fsp.shape_materials[i];
        printf("%f %f -> %f %f, mat=%f\n", s.X, s.Y, s.Z, s.W, m.X);
    }
}

static void gfx_append_shape(hmm_v2 a, hmm_v2 b, material m) {
    size_t c = gfx.trace.fsp.shape_count;
    expect(c < GFX_SHAPE_MAX_COUNT, "max shape count");
    gfx.trace.fsp.shape_vertices[c] = HMM_Vec4(a.X, a.Y, b.X, b.Y);
    gfx.trace.fsp.shape_materials[c] = HMM_Vec4v(m.color, m.type);
    gfx.trace.fsp.shape_count++;
    gfx.trace.sample_count = 0;
}

static void gfx_clear_shapes(void) {
    gfx.trace.fsp.shape_count = 0;
    gfx.trace.sample_count = 0;
}

static void gfx_query_shapes(gfx_shape_func func, void *data) {
    for (size_t i = 0; i < gfx.trace.fsp.shape_count; i++) {
        hmm_vec4 v = gfx.trace.fsp.shape_vertices[i];
        hmm_vec4 m = gfx.trace.fsp.shape_materials[i];
        material mat = {
            .color = m.RGB,
            .type = m.W,
        };
        func(v.XY, v.ZW, mat, data);
    }
}

static void gfx_render(int width, int height, float dpi_scale) {
    hmm_m4 projection = HMM_Orthographic(0.0f, width / dpi_scale, height / dpi_scale, 0.0f, -1.0f, 1.0f);
    size_t current_target = gfx.trace.sample_count & 0x01;
    size_t prev_target = !current_target;

    // trace
    gfx.trace.fsp.time = stm_sec(stm_now());
    gfx.trace.fsp.sample_count = gfx.trace.sample_count++;
    gfx.trace.vsp.projection = projection;
    gfx.trace.bindings.fs_images[SLOT_prev_target] = gfx.trace.targets[prev_target];
    sg_begin_pass(gfx.trace.passes[current_target], &gfx.screen.pass_action);
    sg_apply_pipeline(gfx.trace.pipeline);
    sg_apply_bindings(&gfx.trace.bindings);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_trace_params, &gfx.trace.vsp, sizeof(gfx.trace.vsp));
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_trace_params, &gfx.trace.fsp, sizeof(gfx.trace.fsp));
    sg_draw(0, 3 * 2, 1); // draw 2 triangles
    sg_end_pass();

    // screen
    gfx.screen.vsp.projection = projection;
    gfx.screen.bindings.fs_images[SLOT_tex] = gfx.trace.targets[current_target];
    sg_begin_default_pass(&gfx.screen.pass_action, width, height);
    sg_apply_pipeline(gfx.screen.pipeline);
    sg_apply_bindings(&gfx.screen.bindings);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_trace_params, &gfx.screen.vsp, sizeof(gfx.screen.vsp));
    sg_draw(0, 3 * 2, 1); // draw 2 triangles
    ui_render(width, height, dpi_scale);
    sg_end_pass();

    sg_commit();
}

static void gfx_shutdown(void) {
    sg_destroy_buffer(gfx.screen.vertices);
    sg_destroy_buffer(gfx.screen.indices);
    sg_destroy_shader(gfx.trace.shader);
    sg_destroy_pipeline(gfx.trace.pipeline);
    sg_shutdown();
}

#endif
