#version 450

in vec3 position;
in vec3 colorP;

out vec3 particle_color;

uniform mat4 view;

void main()
{
    gl_Position = vec4(position, 1.f);
	particle_color = colorP;
}