#version 450
layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUVs;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 1) uniform sampler2D tex1;

void main()
{
vec3 color = texture(tex1, inUVs).xyz;
    outFragColor = vec4(color, 1.0f);
}