in vec2 v_position;
flat in int v_grayscale;

_GL(out vec4 color;)

uniform sampler2D u_image;

void main() {
	#ifdef GL_ES
	vec4 t = texture2D(u_image, v_position);
	gl_FragColor = v_grayscale != 0 ? vec4(t.r, t.r, t.r, t.a) : t;
	#else
	vec4 t = texture(u_image, v_position);
	color = v_grayscale != 0 ? vec4(t.r, t.r, t.r, t.a) : t;
	#endif
}
