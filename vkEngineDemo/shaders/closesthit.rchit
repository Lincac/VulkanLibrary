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
    mat.baseColor = vec3(0.82, 0.67, 0.52);
    mat.roughness = 0.35;
    mat.subSurface = 0.0;
    mat.sheen = 0.0;
    mat.sheenTint = 0.0;
    mat.metallic = 0.85;
    mat.specular = 0.5;
    mat.specularTint = 0.0;
    mat.clearcoat = 0.0;
    mat.clearcoatGloss = 0.0;

    payload.hitNormal = vec4(1.0, n.x, n.y, n.z);
    payload.position = vec4(worldPos, 1.0);
    packMaterial(payload, mat);
}
