#version 460
#extension GL_EXT_ray_tracing : require

#include "pathtrace.glsl"

layout(location = 0) rayPayloadInEXT PathPayload payload;

struct Vertex {
    float pos[3];
    float normal[3];
};

layout(set = 0, binding = 2, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

hitAttributeEXT vec2 attribs;

vec3 fetchNormal(uint index)
{
    return vec3(vertices[index].normal[0], vertices[index].normal[1], vertices[index].normal[2]);
}

void main() {
    const uint base = gl_PrimitiveID * 3u;
    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    vec3 n = normalize(
        fetchNormal(base + 0) * barycentrics.x +
        fetchNormal(base + 1) * barycentrics.y +
        fetchNormal(base + 2) * barycentrics.z);

    const vec3 worldPos = gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT;
    const vec3 v = normalize(-gl_WorldRayDirectionEXT);
    if (dot(n, v) < 0.0) {
        n = -n;
    }

    DisneyMaterial mat;
    mat.baseColor      = vec3(1.00, 0.78, 0.34);   // 更接近真实金的谱反射
    mat.roughness      = 0.25;                     // 金属表面较光滑
    mat.subSurface     = 0.0;                      // 金属无次表面散射
    mat.sheen          = 0.0;                      // 金属无布料光泽
    mat.sheenTint      = 0.0;

    mat.metallic       = 1.0;                      // 金属必须是 1.0
    mat.specular       = 0.9;                      // 强烈镜面反射
    mat.specularTint   = 0.6;                      // 高光染上金色（关键）
    mat.clearcoat      = 0.0;                      // 金属一般不加清漆
    mat.clearcoatGloss = 0.0;

    payload.hitNormal = vec4(1.0, n.x, n.y, n.z);
    payload.position = vec4(worldPos, 1.0);
    packMaterial(payload, mat);
}
