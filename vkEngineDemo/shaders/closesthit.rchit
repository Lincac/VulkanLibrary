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

PbrMaterial materialForPrimitive(uint primId)
{
    PbrMaterial mat;
    const uint slot = primId % 4u;

    if (slot == 0u) {
        // 金属（黄铜）
        mat.baseColor = vec3(1.0, 0.86, 0.57);
        mat.roughness = 0.18;
        mat.metallic = 1.0;
        mat.ior = 1.5;
        mat.transmission = 0.0;
    } else if (slot == 1u) {
        // 塑料（红色亮面）
        mat.baseColor = vec3(0.82, 0.06, 0.08);
        mat.roughness = 0.28;
        mat.metallic = 0.0;
        mat.ior = 1.46;
        mat.transmission = 0.0;
    } else if (slot == 2u) {
        // 非金属（哑光陶瓷/漫反射体）
        mat.baseColor = vec3(0.72, 0.68, 0.62);
        mat.roughness = 0.82;
        mat.metallic = 0.0;
        mat.ior = 1.5;
        mat.transmission = 0.0;
    } else {
        // 玻璃
        mat.baseColor = vec3(1.0);
        mat.roughness = 0.015;
        mat.metallic = 0.0;
        mat.ior = 1.52;
        mat.transmission = 1.0;
    }

    return mat;
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

    const PbrMaterial mat = materialForPrimitive(3u);

    payload.hitNormal = vec4(1.0, n.x, n.y, n.z);
    payload.position = vec4(worldPos, 1.0);
    packMaterial(payload, mat);
}
