#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 hitColor;

struct Vertex {
    vec3 pos;
    vec3 normal;
};

layout(set = 0, binding = 2) readonly buffer VertexBuffer {
    Vertex vertices[];
};

hitAttributeEXT vec2 attribs;

void main() {
    const uint base = gl_PrimitiveID * 3u;
    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    vec3 N = normalize(
        vertices[base + 0].normal * barycentrics.x +
        vertices[base + 1].normal * barycentrics.y +
        vertices[base + 2].normal * barycentrics.z);

    const vec3 L = normalize(vec3(0.3, 0.7, 0.5));
    const float diff = max(dot(N, L), 0.0);

    hitColor = vec3(1.0, 0.2, 0.2) * (0.15 + 0.85 * diff);
}
