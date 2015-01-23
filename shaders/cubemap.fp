#version 330

in vec3 tex_pos;
out vec3 o_color;

uniform samplerCube cube_texture;

void main() {
	o_color = texture(cube_texture, tex_pos).rgb;
}
