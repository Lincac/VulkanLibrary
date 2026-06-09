// 数据及结构定义
const float PI = 3.14159265358979323846;

const uint MAT_DIFFUSE         = 0u;
const uint MAT_SMOOTH_PLASTIC  = 1u;

// Sobol direction numbers for dimension 1
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

// Sobol direction numbers for dimension 2
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

struct Material {
    uint  type;          // MAT_DIFFUSE / MAT_SMOOTH_PLASTIC

    vec3  diffuseColor;
    float roughness;   // 用于 Oren–Nayar: sigma
    float intIOR;        // 塑料内部折射率，默认 1.49
    float extIOR;        // 外部介质，默认 1.0
};

struct BsdfSample {
    vec3  wo;   // 局部空间
    vec3  f;    // BSDF 值
    float pdf;
    bool isDelta;
};

// 输入 xi in [0,1]^2，输出世界空间方向 + pdf
struct EnvSample {
    vec3  direction;
    float pdf;   // 对 solid angle 的 pdf
};

struct PathPayload {
    vec4 hitNormal; // x: 1=命中, yzw: 世界空间法线
    vec4 position;  // xyz: 命中点
    vec4 material0; // diffuseColor(rgb) roughness(w)
    vec4 material1; // type(r), intIOR(g), extIOR(b)
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

// 辅助接口定义
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
    m.type         = uint(payload.material1.r + 0.5);
    m.diffuseColor = payload.material0.rgb;
    m.roughness    = payload.material0.a;
    m.intIOR       = max(payload.material1.g, 1.0);
    m.extIOR       = max(payload.material1.b, 1.0);
    return m;
}

void packMaterial(inout PathPayload payload, Material m)
{
    payload.material0 = vec4(m.diffuseColor, m.roughness);
    payload.material1 = vec4(float(m.type), m.intIOR, m.extIOR, 0.0);
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

// bsdf 相关接口定义
// Mitsuba 风格 dielectric Fresnel
float fresnelDielectric(float cosThetaI, float etaI, float etaT)
{
    cosThetaI = abs(cosThetaI);

    float sin2ThetaI = max(0.0, 1.0 - cosThetaI * cosThetaI);
    float sin2ThetaT = (etaI / etaT) * (etaI / etaT) * sin2ThetaI;

    if (sin2ThetaT >= 1.0)
        return 1.0; // 全内反射

    float cosThetaT = sqrt(max(0.0, 1.0 - sin2ThetaT));

    float rPar  = ((etaT * cosThetaI) - (etaI * cosThetaT))
                / ((etaT * cosThetaI) + (etaI * cosThetaT));
    float rPerp = ((etaI * cosThetaI) - (etaT * cosThetaT))
                / ((etaI * cosThetaI) + (etaT * cosThetaT));

    return 0.5 * (rPar * rPar + rPerp * rPerp);
}

// 外部反射 Fresnel: air -> plastic
float fresnelExt(float cosTheta, float etaI, float etaT)
{
    return fresnelDielectric(cosTheta, etaI, etaT);
}

// 内部出射 Fresnel: plastic -> air
float fresnelInt(float cosTheta, float etaI, float etaT)
{
    return fresnelDielectric(cosTheta, etaT, etaI);
}

vec3 reflectLocal(vec3 wi)
{
    // wi: 局部空间入射方向 (z>0)
    return vec3(-wi.x, -wi.y, wi.z);
}

bool isSpecularReflection(vec3 wi, vec3 wo)
{
    vec3 wr = reflectLocal(wi);
    return dot(wr, wo) > 1.0 - 1e-4;
}

// 由 CDF 纹理尺寸和 (i,j) 算立体角微元 dΩ
float envCellSolidAngle(ivec2 cdfRes, int i, int j)
{
    const float du = 1.0 / float(cdfRes.x);
    const float dv = 1.0 / float(cdfRes.y);
    const float v  = (float(j) + 0.5) / float(cdfRes.y);
    const float sinTheta = max(sin(v * PI), 1e-8);
    // dω = sin(θ) dθ dφ = 2π² sin(θ) du dv
    return 2.0 * PI * PI * sinTheta * du * dv;
}

// 从边缘 CDF（.g）用 xi.y 找行 j，返回 (j, pRow)
int sampleEnvRow(float xiY, sampler2D envCdf, ivec2 cdfRes, out float pRow)
{
    int lo = 0;
    int hi = cdfRes.y - 1;
    while (lo < hi)
    {
        const int mid = (lo + hi) >> 1;
        const float cdf = texelFetch(envCdf, ivec2(0, mid), 0).g;
        if (cdf < xiY)
            lo = mid + 1;
        else
            hi = mid;
    }
    const int j = lo;
    const float curr = texelFetch(envCdf, ivec2(0, j), 0).g;
    const float prev = (j > 0) ? texelFetch(envCdf, ivec2(0, j - 1), 0).g : 0.0;
    pRow = max(curr - prev, 0.0);
    return j;
}

// 在行 j 内用条件 CDF（.r）和 xi.x 找列 i，返回 (i, pColGivenRow)
int sampleEnvCol(float xiX, sampler2D envCdf, ivec2 cdfRes, int j, out float pColGivenRow)
{
    int lo = 0;
    int hi = cdfRes.x - 1;
    while (lo < hi)
    {
        const int mid = (lo + hi) >> 1;
        const float cdf = texelFetch(envCdf, ivec2(mid, j), 0).r;
        if (cdf < xiX)
            lo = mid + 1;
        else
            hi = mid;
    }
    const int i = lo;
    const float curr = texelFetch(envCdf, ivec2(i, j), 0).r;
    const float prev = (i > 0) ? texelFetch(envCdf, ivec2(i - 1, j), 0).r : 0.0;
    pColGivenRow = max(curr - prev, 0.0);
    return i;
}

// 由方向得到离散 cell (i,j) 及 uv 所在格
void directionToEnvCell(vec3 direction, ivec2 cdfRes, out int i, out int j, out vec2 uv)
{
    direction = normalize(direction);
    uv = directionToEnvUv(direction);

    i = clamp(int(floor(uv.x * float(cdfRes.x))), 0, cdfRes.x - 1);
    j = clamp(int(floor(uv.y * float(cdfRes.y))), 0, cdfRes.y - 1);
}

// 给定 cell (i,j) 算离散概率与 solid-angle pdf
float envCellPdf(int i, int j, sampler2D envCdf, ivec2 cdfRes)
{
    const float currRow = texelFetch(envCdf, ivec2(0, j), 0).g;
    const float prevRow = (j > 0) ? texelFetch(envCdf, ivec2(0, j - 1), 0).g : 0.0;
    const float pRow = max(currRow - prevRow, 0.0);
    const float currCol = texelFetch(envCdf, ivec2(i, j), 0).r;
    const float prevCol = (i > 0) ? texelFetch(envCdf, ivec2(i - 1, j), 0).r : 0.0;
    const float pColGivenRow = max(currCol - prevCol, 0.0);
    const float pCell = pRow * pColGivenRow;
    const float dOmega = envCellSolidAngle(cdfRes, i, j);
    return (dOmega > 0.0) ? (pCell / dOmega) : 0.0;
}

EnvSample sampleEnvironmentImportance(vec2 xi, sampler2D envMap, sampler2D envCdf)
{
    EnvSample result;
    result.direction = vec3(0.0, 1.0, 0.0);
    result.pdf = 0.0;

    const ivec2 cdfRes = textureSize(envCdf, 0);
    if (cdfRes.x <= 0 || cdfRes.y <= 0)
        return result;

    // 与 CDF 构建一致：marginal 用 xi.y，conditional 用 xi.x
    float pRow = 0.0;
    const int j = sampleEnvRow(xi.y, envCdf, cdfRes, pRow);

    float pColGivenRow = 0.0;
    const int i = sampleEnvCol(xi.x, envCdf, cdfRes, j, pColGivenRow);

    // 对应 CPU 里 (i+0.5)/W, (j+0.5)/H
    const vec2 uv = vec2(
        (float(i) + 0.5) / float(cdfRes.x),
        (float(j) + 0.5) / float(cdfRes.y)
    );

    result.direction = envUvToDirection(uv);

    const float pCell = pRow * pColGivenRow;
    const float dOmega = envCellSolidAngle(cdfRes, i, j);
    result.pdf = (dOmega > 0.0) ? (pCell / dOmega) : 0.0;

    return result;
}

float environmentPdf(vec3 direction, sampler2D envCdf)
{
    const ivec2 cdfRes = textureSize(envCdf, 0);
    if (cdfRes.x <= 0 || cdfRes.y <= 0)
        return 0.0;

    int i, j;
    vec2 uv;
    directionToEnvCell(direction, cdfRes, i, j, uv);

    return envCellPdf(i, j, envCdf, cdfRes);
}

vec3 cosineSampleHemisphere(vec2 xi) 
{
    float r = sqrt(xi.x);
    float phi = 2.0 * PI * xi.y;
    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(max(0.0, 1.0 - x*x - y*y));
    return vec3(x, y, z);
}

vec3 smooth_plastic_eval(vec3 rho, float etaI, float etaT, vec3 wi, vec3 wo)
{
    if (wi.z <= 0.0 || wo.z <= 0.0)
        return vec3(0.0);

    vec3 result = vec3(0.0);

    // 1) 镜面 delta
    if (isSpecularReflection(wi, wo))
    {
        float F = fresnelExt(wi.z, etaI, etaT);
        result += vec3(F / wi.z);
    }

    // 2) 漫反射 — Fi/Fo 均用外部界面 Fresnel（Mitsuba plastic 一致）
    float Fi  = fresnelExt(wi.z, etaI, etaT);
    float Fo  = fresnelExt(wo.z, etaI, etaT);
    result += rho * (1.0 - Fi) * (1.0 - Fo) * (1.0 / PI);

    return result;
}

float smooth_plastic_pdf(vec3 rho, float etaI, float etaT, vec3 wi, vec3 wo)
{
    if (wi.z <= 0.0 || wo.z <= 0.0)
        return 0.0;

    float F = fresnelExt(wi.z, etaI, etaT);
    float specProb = F;
    float diffProb = 1.0 - F;

    float pdf = 0.0;

    // 镜面 lobe（delta 的 mixture pdf）
    if (isSpecularReflection(wi, wo))
        pdf += specProb;

    // 漫反射 lobe
    pdf += diffProb * wo.z * (1.0 / PI);

    return pdf;
}

BsdfSample smooth_plastic_sample(vec3 rho, float etaI, float etaT, vec3 wi, vec2 xi)
{
    BsdfSample s;
    s.wo  = vec3(0.0, 0.0, 1.0);
    s.f   = vec3(0.0);
    s.pdf = 0.0;

    if (wi.z <= 0.0)
        return s;

    float F = fresnelExt(wi.z, etaI, etaT);

    // 按 Fresnel 在镜面 / 漫反射之间选 lobe
    if (xi.x < F)
    {
        // --- 镜面 ---
        s.wo  = reflectLocal(wi);
        s.pdf = F;                          // mixture pdf
        s.f   = vec3(F / wi.z);             // delta BSDF * 与 throughput 公式配套
    }
    else
    {
        // --- 漫反射 ---
        // 重映射 xi，避免浪费随机数
        vec2 xiDiff = vec2((xi.x - F) / max(1.0 - F, 1e-8), xi.y);
        s.wo  = cosineSampleHemisphere(xiDiff);

        float Fo = fresnelExt(s.wo.z, etaI, etaT);
        s.f   = rho * (1.0 - F) * (1.0 - Fo) * (1.0 / PI);
        s.pdf = (1.0 - F) * s.wo.z * (1.0 / PI);
    }

    return s;
}

vec3 oren_nayar_eval(vec3 albedo, float sigma, vec3 wi, vec3 wo)
{
    if (wi.z <= 0.0 || wo.z <= 0.0)
        return vec3(0.0);

    float sigma2 = sigma * sigma;

    float sinTi = sqrt(max(0.0, 1.0 - wi.z * wi.z));
    float sinTo = sqrt(max(0.0, 1.0 - wo.z * wo.z));

    // 切平面上投影方向的 cos(alpha)
    float cosAlpha = 1.0;
    float sinAlpha = 0.0;
    if (sinTi > 1e-4 && sinTo > 1e-4)
    {
        // wi, wo 在切平面 (x,y) 归一化后的点积
        vec2 wi_t = vec2(wi.x, wi.y) / sinTi;
        vec2 wo_t = vec2(wo.x, wo.y) / sinTo;
        cosAlpha = clamp(dot(wi_t, wo_t), -1.0, 1.0);
        sinAlpha = sinTi * sinTo;
    }

    float A = 1.0 - 0.5 * sigma2 / (sigma2 + 0.33);
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    float on = A + B * max(0.0, cosAlpha) * sinAlpha;
    return albedo * on * (1.0 / PI);
}

float oren_nayar_pdf(vec3 wi, vec3 wo)
{
    if (wi.z <= 0.0 || wo.z <= 0.0)
        return 0.0;
    return wo.z * (1.0 / PI);
}

BsdfSample oren_nayar_sample(vec3 albedo, float sigma, vec3 wi, vec2 xi)
{
    BsdfSample s;
    if (wi.z <= 0.0) {
        s.pdf = 0.0;
        s.f   = vec3(0.0);
        s.wo  = vec3(0.0, 0.0, 1.0);
        return s;
    }

    s.wo  = cosineSampleHemisphere(xi);
    s.pdf = oren_nayar_pdf(wi, s.wo);
    s.f   = oren_nayar_eval(albedo, sigma, wi, s.wo);

    return s;
}

vec3 bsdf_eval(in Material m, vec3 wi, vec3 wo)
{
    if (m.type == MAT_SMOOTH_PLASTIC)
        return smooth_plastic_eval(m.diffuseColor, m.extIOR, m.intIOR, wi, wo);

    return oren_nayar_eval(m.diffuseColor, m.roughness, wi, wo);
}

float bsdf_pdf(in Material m, vec3 wi, vec3 wo)
{
    if (m.type == MAT_SMOOTH_PLASTIC)
        return smooth_plastic_pdf(m.diffuseColor, m.extIOR, m.intIOR, wi, wo);

    return oren_nayar_pdf(wi, wo);
}

BsdfSample bsdf_sample(in Material m, vec3 wi, vec2 xi)
{
    if (m.type == MAT_SMOOTH_PLASTIC)
        return smooth_plastic_sample(m.diffuseColor, m.extIOR, m.intIOR, wi, xi);

    return oren_nayar_sample(m.diffuseColor, m.roughness, wi, xi);
}