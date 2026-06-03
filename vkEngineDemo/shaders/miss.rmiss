#version 460
#extension GL_EXT_ray_tracing : require

#include "pathtrace.glsl"

layout(location = 0) rayPayloadInEXT PathPayload payload;

void main() {
    payload.hitNormal.x = 0.0;
}
