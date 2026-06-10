#version 460
#extension GL_EXT_ray_tracing : require

#include "traceGlobal.glsl"

struct Vertex {
    float pos[3];
    float normal[3];
};

layout(set = 0, binding = 2, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};
layout(location = 0) rayPayloadInEXT PathPayload payload;

hitAttributeEXT vec2 attribs;

void main() {
    const uint base = gl_PrimitiveID * 3u;
    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    const vec3 n0 = vec3(vertices[base + 0u].normal[0], vertices[base + 0u].normal[1], vertices[base + 0u].normal[2]);
    const vec3 n1 = vec3(vertices[base + 1u].normal[0], vertices[base + 1u].normal[1], vertices[base + 1u].normal[2]);
    const vec3 n2 = vec3(vertices[base + 2u].normal[0], vertices[base + 2u].normal[1], vertices[base + 2u].normal[2]);
    vec3 n = normalize(n0 * barycentrics.x + n1 * barycentrics.y + n2 * barycentrics.z);

    const vec3 hitPosition = gl_WorldRayOriginEXT + gl_RayTmaxEXT * gl_WorldRayDirectionEXT;
    const vec3 v = normalize(-gl_WorldRayDirectionEXT);
    if (dot(n, v) < 0.0) 
    {
        n = -n;
    }

    payload.hitNormal     = vec4(n, 0.0);
    payload.hitPosition   = vec4(hitPosition, 0.0);

    payload.hitInfo.x = 1.0;
    payload.hitInfo.y = float(DEMO_MATERIAL);

    switch(DEMO_MATERIAL)
    {
        case MATERIAL_DIFFUSE:
            payload.material0 = vec4(vec3(0.8, 0.1, 0.1), 1.0);
            break;
        case MATERIAL_PLASTIC:
            payload.material0 = vec4(vec3(0.8, 0.1, 0.1), 0.0);
            payload.material1 = vec4(vec3(1.0), 0.0);
            payload.material2 = vec4(1.49, 1.0, 0.0, 0.0);
            break;
        case MATERIAL_ROUGHPLASTIC:
            payload.material0 = vec4(vec3(0.8, 0.1, 0.1), 0.05);
            payload.material1 = vec4(vec3(1.0), 0.0);
            payload.material2 = vec4(1.49, 1.0, 0.0, 0.0);
            break;
        case MATERIAL_CONDUCTOR:
            // 金 (Au) @ 550nm 附近，Mitsuba 常用 IOR 数据
            payload.material3 = vec4(0.143, 0.374, 1.442, 0.0);
            payload.material4 = vec4(3.983, 2.453, 1.943, 0.0);
            break;
    }
}