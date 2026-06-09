#version 460
#extension GL_EXT_ray_tracing : require

#include "pathtrace.glsl"

layout(location = 0) rayPayloadInEXT PathPayload payload;

void main() {
    payload.hitNormal     = vec4(normalize(gl_WorldRayDirectionEXT), 0.0);
    payload.hitPosition   = vec4(0);

    payload.hitInfo.x = 0;
    payload.hitInfo.y = float(MAT_TYPE);
}
