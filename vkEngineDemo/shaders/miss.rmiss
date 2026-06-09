#version 460
#extension GL_EXT_ray_tracing : require

#include "traceGlobal.glsl"

layout(location = 0) rayPayloadInEXT PathPayload payload;

void main() {
    payload.hitInfo.x = 0.0;
    payload.hitInfo.y = float(DEMO_MATERIAL);
}
