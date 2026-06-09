#ifndef TRACE_GLOBAL_GLSL
#define TRACE_GLOBAL_GLSL

// 数据、结构、宏
const float PI = 3.14159265358979323846;

const int MATERIAL_DIFFUSE        = 0;
const int MATERIAL_PLASTIC        = 1;
const int MATERIAL_ROUGHPLASTIC   = 2;
const int MATERIAL_CONDUCTOR      = 3;
const int MATERIAL_ROUGHCONDUCTOR = 4;
const int MATERIAL_DIELECTRIC     = 5;
const int MATERIAL_ROUGHDIELECTRIC= 6;

// demo 场景默认材质（closesthit / miss 共用）
const int DEMO_MATERIAL = MATERIAL_ROUGHPLASTIC;

const uint SOBOL_DIR_X[32] = uint[32](
    0x80000000u, 0x40000000u, 0x20000000u, 0x10000000u,
    0x08000000u, 0x04000000u, 0x02000000u, 0x01000000u,
    0x00800000u, 0x00400000u, 0x00200000u, 0x00100000u,
    0x00080000u, 0x00040000u, 0x00020000u, 0x00010000u,
    0x00008000u, 0x00004000u, 0x00002000u, 0x00001000u,
    0x00000800u, 0x00000400u, 0x00000200u, 0x00000100u,
    0x00000080u, 0x00000040u, 0x00000020u, 0x00000010u,
    0x00000008u, 0x00000004u, 0x00000002u, 0x00000001u
);

const uint SOBOL_DIR_Y[32] = uint[32](
    0x80000000u, 0xC0000000u, 0x60000000u, 0x30000000u,
    0x88000000u, 0x44000000u, 0x22000000u, 0x11000000u,
    0x80800000u, 0x40400000u, 0x20200000u, 0x10100000u,
    0x08080000u, 0x04040000u, 0x02020000u, 0x01010000u,
    0x80808000u, 0x40404000u, 0x20202000u, 0x10101000u,
    0x08080800u, 0x04040400u, 0x02020200u, 0x01010100u,
    0x80808080u, 0x40404040u, 0x20202020u, 0x10101010u,
    0x08080808u, 0x04040404u, 0x02020202u, 0x01010101u
);

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

struct BsdfSample {
    vec3  wo;   // 局部空间
    vec3  f;   
    float pdf;
    bool isDelta;
};

struct EnvSample {
    vec3  direction; // 世界空间方向
    float pdf;   
};

struct PathPayload {
    vec4 hitNormal;    // xyz: 世界空间法线
    vec4 hitPosition;  // xyz: 命中点

    vec4 hitInfo;   // x: 1=命中，y:材质类型

    vec4 material0; // xyz: diffuse_reflectance  a: alpha
    vec4 material1; // xyz: specular_reflectance
    vec4 material2; // x: intIOR y: extIOR
    vec4 material3; // xyz: eta
    vec4 material4; // xyz: k
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

// Mitsuba 材质系统
struct Diffuse {
    vec3 diffuse_reflectance;   // 光谱/纹理
    float alpha;                // 粗糙度
};

struct Plastic {
    vec3 diffuse_reflectance;   // 光谱/纹理
    vec3 specular_reflectance;  // 光谱/纹理
    float int_ior;              // 内部折射率（默认 1.49）
    float ext_ior;              // 外部折射率（默认 1.0）
};

struct RoughPlastic {
    vec3 diffuse_reflectance;   // 光谱/纹理
    vec3 specular_reflectance;  // 光谱/纹理
    float int_ior;
    float ext_ior;
    float alpha;                
};

struct Conductor {
    vec3 eta;   // 折射率实部（光谱）
    vec3 k;     // 折射率虚部（光谱）
};

struct RoughConductor {
    vec3 eta;
    vec3 k;
    float alpha;
};

struct Dielectric {
    float int_ior;
    float ext_ior;
};

struct RoughDielectric {
    float int_ior;
    float ext_ior;
    float alpha;
};

struct Material {
    int type;

    Diffuse diffuse;
    Plastic plastic;
    RoughPlastic roughplastic;
    Conductor conductor;
    RoughConductor roughconductor;
    Dielectric dielectric;
    RoughDielectric roughdielectric;
};

// 接口
uint sobol1D(uint index, const uint dir[32]) 
{
    uint g = index ^ (index >> 1); // Gray code
    uint x = 0u;
    for (uint k = 0u; k < 32u; k++) 
    {
        if ((g & (1u << k)) != 0u)
        {
            x ^= dir[k];
        }
    }
    return x;
}

vec2 sobol2D(uint index)
{
    uint sx = sobol1D(index, SOBOL_DIR_X);
    uint sy = sobol1D(index, SOBOL_DIR_Y);

    const float inv = 1.0 / 4294967296.0; // 1 / 2^32
    return vec2(float(sx) * inv, float(sy) * inv);
}

float range01(uint index, const uint dir[32])
{
    const float inv = 1.0 / 4294967296.0; // 1 / 2^32
    return float(sobol1D(index, dir)) * inv;
}

Material unpackMaterial(PathPayload payload)
{
    Material m;
    m.type = int(payload.hitInfo.y);

    switch(m.type)
    {
        case MATERIAL_DIFFUSE:
            m.diffuse.diffuse_reflectance = payload.material0.rgb;
            m.diffuse.alpha = payload.material0.a;
            break;
        case MATERIAL_PLASTIC:
            m.plastic.diffuse_reflectance = payload.material0.rgb;
            m.plastic.specular_reflectance = payload.material1.rgb;
            m.plastic.int_ior = payload.material2.r;
            m.plastic.ext_ior = payload.material2.g;
            break;
        case MATERIAL_ROUGHPLASTIC:
            m.roughplastic.diffuse_reflectance = payload.material0.rgb;
            m.roughplastic.alpha = payload.material0.a;
            m.roughplastic.specular_reflectance = payload.material1.rgb;
            m.roughplastic.int_ior = payload.material2.r;
            m.roughplastic.ext_ior = payload.material2.g;
            break;
    }

    return m;
}

void packMaterial(inout PathPayload payload, Material m)
{
    payload.hitInfo.y = float(m.type);

    switch(m.type)
    {
        case MATERIAL_DIFFUSE:
            payload.material0 = vec4(m.diffuse.diffuse_reflectance, m.diffuse.alpha);
            break;
        case MATERIAL_PLASTIC:
            payload.material0 = vec4(m.plastic.diffuse_reflectance, 0.0);
            payload.material1 = vec4(m.plastic.specular_reflectance, 0.0);
            payload.material2 = vec4(m.plastic.int_ior, m.plastic.ext_ior, 0.0, 0.0);
            break;
        case MATERIAL_ROUGHPLASTIC:
            payload.material0 = vec4(m.roughplastic.diffuse_reflectance, m.roughplastic.alpha);
            payload.material1 = vec4(m.roughplastic.specular_reflectance, 0.0);
            payload.material2 = vec4(m.roughplastic.int_ior, m.roughplastic.ext_ior, 0.0, 0.0);
            break;
    }
}

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

vec3 envUvToDirection(vec2 uv)
{
    const float phi   = (uv.x - 0.5) * 2.0 * PI;
    const float theta = uv.y * PI;
    const float sinTheta = sin(theta);
    const float cosTheta = cos(theta);
    return normalize(vec3(
        sinTheta * cos(phi),
        cosTheta,
        sinTheta * sin(phi)
    ));
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

float luminance(vec3 c)
{
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

vec3 toneMap(vec3 color, float exposure)
{
    color *= exp2(exposure);
    color = ACESFilm(color);
    return pow(color, vec3(1.0 / 2.2));
}

ShadingFrame makeShadingFrame(vec3 n)
{
    n = normalize(n);

    vec3 t = normalize(abs(n.z) < 0.999 ? cross(vec3(0,0,1), n)
                                        : cross(vec3(0,1,0), n));
    vec3 b = cross(n, t);

    ShadingFrame f;
    f.N = n;   
    f.T = t;  
    f.B = b;  

    return f;
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

#endif // TRACE_GLOBAL_GLSL