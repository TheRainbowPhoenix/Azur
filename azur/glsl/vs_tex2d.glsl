/* Vertex position in screen space */
layout(location=0) in vec2 a_vertex;
/* Same in texture space */
layout(location=1) in vec2 a_texture_pos;
/* Whether to replicate red on all channels */
layout(location=2) in float a_grayscale;

/* Location in image space */
out vec2 v_position;
out float v_grayscale;

uniform mat3 u_transform;

void main() {
    v_position = a_texture_pos;
    v_grayscale = a_grayscale;
    gl_Position = vec4((u_transform * vec3(a_vertex, 1.0)).xy, 0.0, 1.0);
}
