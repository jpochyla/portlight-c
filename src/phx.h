#ifndef INCLUDE_PHX
#define INCLUDE_PHX

#define STBDS_NO_SHORT_NAMES
#include <vendor/stb/stb_ds.h>

#define HANDMADE_MATH_NO_SSE
#include <vendor/Handmade-Math/HandmadeMath.h>

#include <chipmunk/chipmunk.h>

#include "utils.h"

#define PHX_TERRAIN_MAX_VERTS 3

typedef struct {
    void *id;
} phx_handle;

static struct {
    cpSpace *space;
    cpShape **terrain_shapes;
} phx;

static void phx_create(void) {
    phx.space = cpSpaceNew();
    expect(phx.space, "phx.space");
    cpSpaceSetIterations(phx.space, 30);
    cpSpaceSetGravity(phx.space, cpv(0.0, 10.0));
}

static phx_handle phx_append_terrain(const hmm_v2 *verts, size_t count) {
    cpVect cpverts[PHX_TERRAIN_MAX_VERTS];
    for (size_t i = 0; i < count; i++) {
        cpverts[i] = cpv(verts[i].X, verts[i].Y);
    }
    cpBody *body = cpSpaceGetStaticBody(phx.space);
    cpShape *shape = cpPolyShapeNew(body, count, cpverts, cpTransformIdentity, 0.0f);
    cpSpaceAddShape(phx.space, shape);
    stbds_arrput(phx.terrain_shapes, shape);
    return (phx_handle){shape};
}

static void phx_clear_terrain(void) {
    while (stbds_arrlenu(phx.terrain_shapes) > 0) {
        cpShape *shape = stbds_arrpop(phx.terrain_shapes);
        cpSpaceRemoveShape(phx.space, shape);
        cpShapeFree(shape);
    }
}

static void phx_query_vertices(phx_handle sh, hmm_v2 *verts, size_t *vert_count) {
    int count = cpPolyShapeGetCount(sh.id);
    if (count < 0) {
        count = 0;
    }
    for (int i = 0; i < count; i++) {
        cpVect v = cpPolyShapeGetVert(sh.id, i);
        verts[i] = HMM_Vec2(v.x, v.y);
    }
    *vert_count = (size_t)count;
}

static void phx_destroy(void) {
    cpSpaceFree(phx.space);
}

static void phx_simulate(void) {
    cpSpaceStep(phx.space, 1.0f / 100.0f);
}

#endif
