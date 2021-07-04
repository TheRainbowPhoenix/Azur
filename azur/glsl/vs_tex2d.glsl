/* Vertex position in screen space */
layout(location=0) in vec2 a_vertex;
/* Same in texture space */
layout(location=1) in vec2 a_texture_pos;

/* Location in image space */
out vec2 v_position;

uniform vec3 u_windowSize;

void main() {
	v_position = a_texture_pos;

	float w = u_windowSize.x / 2.0;
	float h = u_windowSize.y / 2.0;

	gl_Position.x =  (a_vertex.x - w) / w;
	gl_Position.y = -(a_vertex.y - h) / h;
	gl_Position.z = 0.0;
	gl_Position.w = 1.0;
}
