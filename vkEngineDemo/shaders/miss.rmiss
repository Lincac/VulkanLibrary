#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadEXT vec3 hitColor;

void main() {
    hitColor = vec3(0.1, 0.1, 0.2);  // 未命中：深蓝背景
}