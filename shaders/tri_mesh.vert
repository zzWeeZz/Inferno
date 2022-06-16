#version 450
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec2 inUVs;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUVs;

layout (set = 0, binding = 0) uniform CameraBuffer
{
    mat4 view;
    mat4 proj;
    mat4 viewproj;
} cameraData;

layout (push_constant) uniform push_constants
{
    vec4 data;
    mat4 renderMatrix;
} PushConstant;

void main()
{    
    mat4 transformMatrix = (cameraData.viewproj * PushConstant.renderMatrix);
    gl_Position = transformMatrix * vec4(inPosition, 1.0f);
    outColor = inColor;
    outUVs = inUVs;
}