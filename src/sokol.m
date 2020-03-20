#define SOKOL_IMPL
#define SOKOL_METAL
#include "vendor/sokol/sokol_app.h"
#include "vendor/sokol/sokol_gfx.h"
#include "vendor/sokol/sokol_glue.h"
#include "vendor/sokol/sokol_time.h"

#define SOKOL_GL_IMPL
#include "vendor/sokol/util/sokol_gl.h"

#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_NO_SSE
#include "vendor/Handmade-Math/HandmadeMath.h"

#define STB_DS_IMPLEMENTATION
#define STBDS_NO_SHORT_NAMES
#include "vendor/stb/stb_ds.h"
