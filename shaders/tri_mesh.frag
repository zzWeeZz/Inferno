#version 450
layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUVs;

layout (location = 0) out vec4 outFragColor;

void main()
{
    outFragColor = vec4(inUVs.x, inUVs.y, 0.5f, 1.0f);
}