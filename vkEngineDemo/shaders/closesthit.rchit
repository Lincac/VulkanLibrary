#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadEXT vec3 hitColor;

void main() {
    hitColor = vec3(1.0, 0.2, 0.2);  // 命中：红色三角形
}