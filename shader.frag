#version 450

out vec4 color;

in vec3 color_out;
in vec2 uv;

uniform sampler2D Texture;

void main()
{
    color = texture(Texture,uv);
}