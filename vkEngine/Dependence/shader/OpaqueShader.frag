#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec2 vUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Scene {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 lightDir;
    vec4 lightColor;
} uScene;

layout(set = 1, binding = 0) uniform MaterialParams {
    vec4  baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
    vec3  emissiveFactor;
    float alphaCutoff;
} uMat;

void main()
{
    vec4 baseColor = uMat.baseColorFactor;
    if (baseColor.a < uMat.alphaCutoff) {
        discard;
    }

    vec3 norm = normalize(vNormal);	
    if (norm.z < 0.0) { norm = -1.0*norm; }
    float df = max(0.000001, norm.z);

    outColor = vec4(df * baseColor.rgb, baseColor.a);
}
