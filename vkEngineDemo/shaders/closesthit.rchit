#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 hitColor;

// 与 C++ ObjVertex 一致：pos[3] + normal[3]，共 24 字节（避免 std430 下 vec3 的 16 字节对齐）
struct Vertex {
    float pos[3];
    float normal[3];
};

layout(set = 0, binding = 2, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

// BLAS 为三角形
// attrib 会自动被 Vulkan 填充为 barycentric 坐标
hitAttributeEXT vec2 attribs;

vec3 fetchNormal(uint index)
{
    return vec3(vertices[index].normal[0], vertices[index].normal[1], vertices[index].normal[2]);
}

void main() {
    const uint base = gl_PrimitiveID * 3u;
    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    vec3 N = normalize(
        fetchNormal(base + 0) * barycentrics.x +
        fetchNormal(base + 1) * barycentrics.y +
        fetchNormal(base + 2) * barycentrics.z);

    const vec3 L = normalize(vec3(0.3, 0.7, 0.5));
    const float diff = max(dot(N, L), 0.0);

    hitColor = vec3(1.0, 0.2, 0.2) * (0.15 + 0.85 * diff);
}
