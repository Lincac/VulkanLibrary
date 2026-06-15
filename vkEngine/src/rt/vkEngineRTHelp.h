#pragma once

#include "common/vkEngineHelp.h"

/// @brief 路径追踪 uniform（std430，binding = 5，与 pathtrace.glsl PathTraceSettings 一致）
struct PathTraceSettingsGPU {
    float cameraOrigin[3] = {0.0f, 0.0f, 2.0f};
    float cameraLookAt[3] = {0.0f, 0.0f, 0.0f};
    float cameraUp[3] = {0.0f, 1.0f, 0.0f};

    float exposure = 0.0f;
    float fovYDegrees = 45.0f;

    uint32_t samplesPerPixel = 64;
    uint32_t maxBounces = 50;
};

static_assert(sizeof(PathTraceSettingsGPU) == 52, "PathTraceSettingsGPU std430 size mismatch");
static_assert(offsetof(PathTraceSettingsGPU, cameraOrigin) == 0, "PathTraceSettingsGPU cameraOrigin offset mismatch");
static_assert(offsetof(PathTraceSettingsGPU, cameraLookAt) == 12, "PathTraceSettingsGPU cameraLookAt offset mismatch");
static_assert(offsetof(PathTraceSettingsGPU, cameraUp) == 24, "PathTraceSettingsGPU cameraUp offset mismatch");
static_assert(offsetof(PathTraceSettingsGPU, exposure) == 36, "PathTraceSettingsGPU exposure offset mismatch");
static_assert(offsetof(PathTraceSettingsGPU, fovYDegrees) == 40, "PathTraceSettingsGPU fovYDegrees offset mismatch");
static_assert(offsetof(PathTraceSettingsGPU, samplesPerPixel) == 44, "PathTraceSettingsGPU samplesPerPixel offset mismatch");
static_assert(offsetof(PathTraceSettingsGPU, maxBounces) == 48, "PathTraceSettingsGPU maxBounces offset mismatch");