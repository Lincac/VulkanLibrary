const float PI = 3.14159265358979323846;

struct Vertex {
    float pos[3];
    float normal[3];
};

struct PathTraceSettings {
    float cameraOrigin[3];
    float cameraLookAt[3];
    float cameraUp[3];

    float exposure;
    float fovYDegrees;

    uint samplesPerPixel;
    uint maxBounces;
};

struct ShadingFrame {
    vec3 T; // local +X
    vec3 B; // local +Y
    vec3 N; // local +Z (shading normal)
};

struct PathPayload {
    vec4 hitNormal; // x: 1=命中, yzw: 世界空间法线
    vec4 position;  // xyz: 命中点
};

struct Camera {
    vec3 origin;
    vec3 forward;
    vec3 right;
    vec3 up;
    float tanHalfFovY;
};

struct CameraRay {
    vec3 origin;
    vec3 direction;
};

Camera getCamera(PathTraceSettings settings)
{
    const vec3 origin = vec3(settings.cameraOrigin[0], settings.cameraOrigin[1], settings.cameraOrigin[2]);
    const vec3 lookAt = vec3(settings.cameraLookAt[0], settings.cameraLookAt[1], settings.cameraLookAt[2]);
    const vec3 up = vec3(settings.cameraUp[0], settings.cameraUp[1], settings.cameraUp[2]);

    Camera camera;
    camera.origin = origin;
    camera.forward = normalize(lookAt - origin);
    camera.right = abs(dot(camera.forward, normalize(up))) > 0.999
        ? normalize(cross(camera.forward, vec3(0.0, 0.0, 1.0)))
        : normalize(cross(camera.forward, up));
    camera.up = cross(camera.right, camera.forward);
    camera.tanHalfFovY = tan(radians(settings.fovYDegrees * 0.5));

    return camera;
}

CameraRay getCameraRay(Camera camera, vec2 pixelCenter, vec2 imageSize, vec2 jitter)
{
    const vec2 pixel = pixelCenter + jitter;
    const vec2 uv = pixel / imageSize;
    const vec2 ndc = vec2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    const float aspect = imageSize.x / imageSize.y;

    CameraRay ray;
    ray.origin = camera.origin;
    ray.direction = normalize(
        camera.forward
        + camera.right * (ndc.x * aspect * camera.tanHalfFovY)
        + camera.up * (ndc.y * camera.tanHalfFovY));
    return ray;
}

vec2 directionToEnvUv(vec3 direction)
{
    direction = normalize(direction);
    const float phi = atan(direction.z, direction.x);
    const float theta = acos(clamp(direction.y, -1.0, 1.0));
    return vec2(phi * (0.5 / PI) + 0.5, theta / PI);
}

vec3 sampleEnvironment(sampler2D envMap, vec3 direction)
{
    return texture(envMap, directionToEnvUv(direction)).rgb;
}

vec3 ACESFilm(vec3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 toneMap(vec3 color, float exposure)
{
    color *= exp2(exposure);
    color = ACESFilm(color);
    return pow(color, vec3(1.0 / 2.2));
}

mat3 shadingFrameToWorld(ShadingFrame frame)
{
    return mat3(frame.T, frame.B, frame.N); // 列 = T,B,N
}

vec3 worldToLocal(vec3 vWorld, ShadingFrame frame)
{
    return transpose(shadingFrameToWorld(frame)) * vWorld;
}

vec3 localToWorld(vec3 vLocal, ShadingFrame frame)
{
    return shadingFrameToWorld(frame) * vLocal;
}