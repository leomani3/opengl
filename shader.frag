#version 450

out vec3 color;

in vec3 particle_color;

void main()
{
    color = particle_color;
}