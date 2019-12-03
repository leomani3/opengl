#version 450

in vec3 position;
in vec3 normal;
in vec2 uv_in;

out vec3 color_out;
out vec2 uv;

uniform float scale;
uniform mat4 Mat;
uniform mat4 projection;
uniform mat4 view;

void main()
{
    gl_Position = projection * view * Mat * vec4(position, 1.0);
	uv = uv_in;
	color_out =  normal;
}