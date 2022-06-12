#version 450
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (location = 0) out vec3 outColor;

layout (push_constant) uniform push_constants
{
    vec4 data;
    mat4 renderMatrix;
} PushConstant;

void main()
{    
    gl_Position = PushConstant.renderMatrix * vec4(inPosition, 1.0f);
    outColor = inColor;
}