#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec3 vTangent;
layout(location = 3) out vec2 vUV;

layout(push_constant) uniform Push {
    mat4 model;
    mat4 viewProj;
} uPush;

void main()
{
    vec4 worldPos = uPush.model * vec4(inPosition, 1.0);
    vWorldPos = worldPos.xyz;

    mat3 normalMat = transpose(inverse(mat3(uPush.model)));
    vNormal  = normalize(normalMat * inNormal);
    vTangent = normalize(normalMat * inTangent);
    vUV = inUV;

    gl_Position = uPush.viewProj * worldPos;
}
