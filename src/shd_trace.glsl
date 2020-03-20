#pragma sokol @ctype mat4 hmm_m4
#pragma sokol @ctype vec2 hmm_v2
#pragma sokol @ctype vec4 hmm_v4

#pragma sokol @vs vs_trace
uniform vs_trace_params {
    mat4 projection;
};

in vec2 position;
in vec2 uv0;

void main() {
    gl_Position = projection * vec4(position, 0.0, 1.0);
}
#pragma sokol @end

#pragma sokol @fs fs_trace

#define PI 3.1415927

#define SHAPE_MAX_COUNT 512

#define MAT_DIFFUSE 0.0
#define MAT_REFLECT 1.0
#define MAT_LIGHT 2.0

uniform fs_trace_params {
    vec4 shape_vertices[SHAPE_MAX_COUNT];
    vec4 shape_materials[SHAPE_MAX_COUNT];
    float shape_count;
    float sample_count;
    float time;
};
uniform sampler2D prev_target;

out vec4 frag_color;

struct intersection {
    float tMin;
    float tMax;
    float mat;
    vec3 color;
    vec2 n;
};

struct ray {
    vec2 origin;
    vec2 dir;
};

struct shape {
    vec2 a;
    vec2 b;
    vec3 color;
    float mat;
};

float rand(vec2 scale, float seed) {
    return fract(sin(dot(gl_FragCoord.xy + seed, scale)) * 43758.5453 + seed);
}

vec2 sampleCircle(float seed) {
    float xi = rand(vec2(6.7264, 10.873), seed);
    float theta = 2.0 * PI * xi;
    return vec2(cos(theta), sin(theta));
}

void fetchShape(uint i, out shape s) {
    vec4 v = shape_vertices[i];
    s.a = v.xy;
    s.b = v.zw;
    vec4 m = shape_materials[i];
    s.color = m.rgb;
    s.mat = m.a;
}

void intersectLine(ray r, vec2 a, vec2 b, float mat, vec3 color, inout intersection isect) {
    vec2 sT = b - a;
    vec2 sN = vec2(-sT.y, sT.x);
    float t = dot(sN, a - r.origin) / dot(sN, r.dir);
    float u = dot(sT, r.origin + r.dir*t - a);
    if (t < isect.tMin || t >= isect.tMax || u < 0.0 || u > dot(sT, sT))
        return;

    isect.tMax = t;
    isect.mat = mat;
    isect.color = color;
    isect.n = normalize(sN);
}

void intersect(ray r, inout intersection isect) {
    shape s;
    for (uint i = 0; i < shape_count; i++) {
        fetchShape(i, s);
        intersectLine(r, s.a, s.b, s.mat, s.color, isect);
    }
}

#define BOUNCE_COUNT 4
#define T_MIN 1e-4
#define T_MAX 1e30

#define DIST_COEF 0.35
#define LIGHT_COEF 2.0

vec3 traceRay(ray r, uint isample) {
    vec3 color = vec3(0.0);
    vec3 mask = vec3(1.0);

    for (uint ibounce = 0; ibounce < BOUNCE_COUNT; ibounce++) {
        intersection isect;
        isect.tMin = T_MIN;
        isect.tMax = T_MAX;
        intersect(r, isect);

        if (isect.tMax == T_MAX) {
            // no hit
            break;
        }
        if (isect.mat == MAT_LIGHT) {
            float cosTheta = abs(dot(normalize(r.dir), isect.n));
            float d = 1.0 - isect.tMax * DIST_COEF;
            color += mask * isect.color * cosTheta * d * LIGHT_COEF;
            break;
        }
        else if (isect.mat == MAT_REFLECT) {
            vec2 hit = r.origin + r.dir * isect.tMax;
            r.origin = hit;
            r.dir = reflect(r.dir, isect.n);
            // r.origin += normalize(r.dir);
            mask *= isect.color;
        }
        else if (isect.mat == MAT_DIFFUSE) {
            vec2 hit = r.origin + r.dir * isect.tMax;
            r.origin = hit;
            r.dir = (isect.n + sampleCircle(time + sample_count + isample + ibounce)) * 200;
            // r.origin += normalize(r.dir);
            mask *= isect.color;
        }
    }

    return color;
}

#define SAMPLE_COUNT 1

vec3 computeColor(vec2 coord) {
    vec3 color = vec3(0.0);

    for (uint isample = 0; isample < SAMPLE_COUNT; isample++) {
        ray r;
        r.origin = coord;
        r.dir = sampleCircle(time + sample_count * isample) * 500;
        color += traceRay(r, isample);
    }
    color /= SAMPLE_COUNT;

    return color;
}

void main() {
    vec2 coord = gl_FragCoord.xy;
    vec3 color0 = texelFetch(prev_target, ivec2(coord), 0).rgb;
    vec3 color = computeColor(coord);
    vec3 mixed = mix(color, color0, sample_count / (sample_count + 1));
    frag_color = vec4(mixed, 1.0);
}
#pragma sokol @end

#pragma sokol @program sh_trace vs_trace fs_trace
