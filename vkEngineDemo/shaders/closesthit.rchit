#version 460
#extension GL_EXT_ray_tracing : require

#include "pathtrace.glsl"

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
    const vec3 wo = normalize(-gl_WorldRayDirectionEXT);
    
    if (dot(n, wo) < 0.0) 
    {
        n = -n;
    }

    payload.hitNormal = vec4(1.0, n.x, n.y, n.z);
    payload.position = vec4(hitPosition, 1.0);
}
