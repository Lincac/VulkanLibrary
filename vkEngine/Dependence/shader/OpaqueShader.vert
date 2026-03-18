#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec3 vTangent;
layout(location = 3) out vec2 vUV;

layout(set = 0, binding = 0) uniform Scene {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 lightDir;
    vec4 lightColor;
} uScene;

void main()
{
    vec4 worldPos = uScene.model * vec4(inPosition, 1.0);
    vWorldPos = worldPos.xyz;

    mat3 normalMat = transpose(inverse(mat3(uScene.model)));
    vNormal  = normalize(normalMat * inNormal);
    vTangent = normalize(normalMat * inTangent);
    vUV = inUV;

    gl_Position = uScene.proj * uScene.view * worldPos;
}
