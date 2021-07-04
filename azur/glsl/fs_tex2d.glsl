in vec2 v_position;

_GL(out vec4 color;)

uniform sampler2D u_image;

void main() {
	#ifdef GL_ES
	gl_FragColor = texture2D(u_image, v_position);
	#else
	color = texture(u_image, v_position);
	#endif
}
