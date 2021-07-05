/* Vertex position in screen space */
layout(location=0) in vec2 a_vertex;
/* Same in texture space */
layout(location=1) in vec2 a_texture_pos;

/* Location in image space */
out vec2 v_position;

uniform mat3 u_transform;

void main() {
    v_position = a_texture_pos;
    gl_Position = vec4((u_transform * vec3(a_vertex, 1.0)).xy, 0.0, 1.0);
}
