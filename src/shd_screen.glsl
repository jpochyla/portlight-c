#pragma sokol @ctype mat4 hmm_m4
#pragma sokol @ctype vec2 hmm_v2
#pragma sokol @ctype vec4 hmm_v4

#pragma sokol @vs vs_screen
uniform vs_screen_params {
    mat4 projection;
};

in vec2 position;
in vec2 uv0;

out vec2 uv;

void main() {
    gl_Position = projection * vec4(position, 0.0, 1.0);
    uv = uv0;
}
#pragma sokol @end

#pragma sokol @fs fs_screen
uniform sampler2D tex;

in vec2 uv;

out vec4 frag_color;

void main() {
    frag_color = texture(tex, uv);
}
#pragma sokol @end

#pragma sokol @program sh_screen vs_screen fs_screen
