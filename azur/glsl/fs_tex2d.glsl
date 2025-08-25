in vec2 v_position;
in float v_grayscale;

out vec4 color;

uniform sampler2D u_image;

void main() {
	vec4 t = texture(u_image, v_position);
	color = v_grayscale > 0.5 ? vec4(t.r, t.r, t.r, t.a) : t;
}
