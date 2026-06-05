// 路径追踪公用函数（由 raygen / closesthit / miss include）

const float PI = 3.14159265358979323846;

const bool PATH_REGULARIZE = true;
const float REGULARIZE_MIN_ROUGHNESS = 0.04;
const float GLASS_DELTA_ROUGHNESS = 0.05;
const float RAY_SURFACE_EPS = 0.002;

// std140，与 C++ PathTraceSettingsGPU 一致（binding = 5）
struct PathTraceSettings {
    vec3 cameraOrigin;
    float exposure;
    vec3 cameraLookAt;
    float fovYDegrees;
    vec3 cameraUp;
    float _pad0;
    uint samplesPerPixel;
    uint maxBounces;
    uint _pad1;
    uint _pad2;
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

Camera createCamera(vec3 origin, vec3 lookAt, vec3 worldUp, float verticalFovDegrees)
{
    Camera camera;
    camera.origin = origin;
    camera.forward = normalize(lookAt - origin);
    camera.right = abs(dot(camera.forward, normalize(worldUp))) > 0.999
        ? normalize(cross(camera.forward, vec3(0.0, 0.0, 1.0)))
        : normalize(cross(camera.forward, worldUp));
    camera.up = cross(camera.right, camera.forward);
    camera.tanHalfFovY = tan(radians(verticalFovDegrees * 0.5));
    return camera;
}

Camera getCamera(PathTraceSettings settings)
{
    return createCamera(
        settings.cameraOrigin,
        settings.cameraLookAt,
        settings.cameraUp,
        settings.fovYDegrees);
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

struct PathPayload {
    vec4 hitNormal; // x: 1=命中, yzw: 世界空间法线
    vec4 position;  // xyz: 命中点
    vec4 material0; // baseColor.rgb, transmission
    vec4 material1; // roughness, metallic, ior, unused
};

struct PbrMaterial {
    vec3  baseColor;
    float roughness;
    float metallic;
    float ior;
    float transmission; // 0=不透明, 1=玻璃
};

struct BsdfSample {
    vec3 wi;
    float pdf;
    vec3 weight;
    bool isSpecular;
    bool isTransmissive;
};

PbrMaterial unpackMaterial(PathPayload payload)
{
    PbrMaterial m;
    m.baseColor = payload.material0.rgb;
    m.transmission = payload.material0.w;
    m.roughness = payload.material1.x;
    m.metallic = payload.material1.y;
    m.ior = max(payload.material1.z, 1.0);
    return m;
}

void packMaterial(inout PathPayload payload, PbrMaterial m)
{
    payload.material0 = vec4(m.baseColor, m.transmission);
    payload.material1 = vec4(m.roughness, m.metallic, m.ior, 0.0);
}

PbrMaterial regularizePbrMaterial(PbrMaterial m)
{
    m.roughness = max(m.roughness, REGULARIZE_MIN_ROUGHNESS);
    return m;
}

bool isPbrOpaque(PbrMaterial m)
{
    return m.transmission < 0.01;
}

bool isPbrNonSpecular(PbrMaterial m)
{
    if (!isPbrOpaque(m)) {
        return m.roughness > 0.05;
    }
    return m.metallic < 1.0 && m.roughness > 1e-3;
}

uint pcgHash(inout uint state)
{
    state = state * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float rand01(inout uint seed)
{
    return float(pcgHash(seed)) / 4294967296.0;
}

uint deriveSeed(ivec2 pixel, uint sampleIndex, uint salt)
{
    uint state = uint(pixel.x) ^ (uint(pixel.y) * 747796405u + 2891336453u);
    state ^= sampleIndex * 668265263u + salt * 48271u;
    pcgHash(state);
    pcgHash(state);
    return state;
}

float rand01FromSeed(uint seed)
{
    uint state = seed;
    return float(pcgHash(state)) / 4294967296.0;
}

vec2 samplePrimaryJitter(ivec2 pixel, uint sampleIndex)
{
    const uint seedX = deriveSeed(pixel, sampleIndex, 0u);
    const uint seedY = deriveSeed(pixel, sampleIndex, 1u);
    return vec2(rand01FromSeed(seedX), rand01FromSeed(seedY)) - 0.5;
}

uint initSampleSeed(ivec2 pixel, uint sampleIndex)
{
    return deriveSeed(pixel, sampleIndex, 2u);
}

void buildOrthonormalBasis(vec3 n, out vec3 t, out vec3 b)
{
    t = abs(n.z) < 0.999 ? normalize(cross(n, vec3(0.0, 0.0, 1.0)))
                         : normalize(cross(n, vec3(0.0, 1.0, 0.0)));
    b = cross(n, t);
}

float schlickWeight(float u)
{
    const float m = clamp(1.0 - u, 0.0, 1.0);
    return m * m * m * m * m;
}

vec3 schlickFresnel(float cosTheta, vec3 f0)
{
    return mix(f0, vec3(1.0), schlickWeight(cosTheta));
}

float distributionGTR2(float nDotH, float alphaRoughness)
{
    const float a2 = alphaRoughness * alphaRoughness;
    const float t = 1.0 + (a2 - 1.0) * nDotH * nDotH;
    return a2 / (PI * t * t);
}

float visibilitySmithGGXCorrelated(float nDotV, float nDotL, float alphaRoughness)
{
    const float a2 = alphaRoughness * alphaRoughness;
    const float gv = nDotL * length(vec3(nDotV * a2, nDotV, alphaRoughness));
    const float gl = nDotV * length(vec3(nDotL * a2, nDotL, alphaRoughness));
    return 0.5 / max(gv + gl, 1e-5);
}

float luminance(vec3 c)
{
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

vec3 fresnelF0(PbrMaterial m)
{
    const float f0Scalar = pow((m.ior - 1.0) / (m.ior + 1.0), 2.0);
    const vec3 dielectricF0 = vec3(f0Scalar);
    return mix(dielectricF0, m.baseColor, m.metallic);
}

float ggxSmithG1(float nDotV, float alphaRoughness)
{
    const float a2 = alphaRoughness * alphaRoughness;
    const float b = nDotV * nDotV;
    return 2.0 * nDotV / max(nDotV + sqrt(a2 + b - a2 * b), 1e-5);
}

float pdfCosineHemisphere(float nDotL)
{
    return max(nDotL, 0.0) / PI;
}

vec3 sampleGGXVNDF_Local(vec3 vLocal, float alpha, vec2 xi)
{
    vec3 vh = normalize(vec3(alpha * vLocal.x, alpha * vLocal.y, vLocal.z));
    const float lenSq = dot(vh.xy, vh.xy);

    vec3 t1 = lenSq > 1e-8 ? vec3(-vh.y, vh.x, 0.0) * inversesqrt(lenSq) : vec3(1.0, 0.0, 0.0);
    const vec3 t2 = cross(vh, t1);

    const float r = sqrt(xi.x);
    const float phi = 2.0 * PI * xi.y;
    const float t1c = r * cos(phi);
    float t2c = r * sin(phi);
    const float s = 0.5 * (1.0 + vh.z);
    t2c = mix(sqrt(max(0.0, 1.0 - t1c * t1c)), t2c, s);

    const vec3 nh = t1c * t1 + t2c * t2 + sqrt(max(0.0, 1.0 - t1c * t1c - t2c * t2c)) * vh;
    return normalize(vec3(alpha * nh.x, alpha * nh.y, max(0.0, nh.z)));
}

float pdfGGXVNDF_Local(vec3 vLocal, vec3 hLocal, float alpha)
{
    const float nDotV = max(vLocal.z, 1e-5);
    const float nDotH = clamp(hLocal.z, 0.0, 1.0);
    const float vDotH = max(dot(vLocal, hLocal), 0.0);
    const float d = distributionGTR2(nDotH, alpha);
    const float g1 = ggxSmithG1(nDotV, alpha);
    return g1 * vDotH * d / nDotV;
}

vec3 worldFromLocal(vec3 n, vec3 tangent, vec3 bitangent, vec3 local)
{
    return normalize(tangent * local.x + bitangent * local.y + n * local.z);
}

vec3 localFromWorld(vec3 n, vec3 tangent, vec3 bitangent, vec3 world)
{
    return vec3(dot(world, tangent), dot(world, bitangent), dot(world, n));
}

bool sampleGGXVNDFReflection(vec3 n, vec3 v, float alphaRoughness, inout uint seed, out vec3 l, out float pdf)
{
    vec3 tangent;
    vec3 bitangent;
    buildOrthonormalBasis(n, tangent, bitangent);

    const vec3 vLocal = localFromWorld(n, tangent, bitangent, v);
    if (vLocal.z <= 0.0) {
        pdf = 0.0;
        return false;
    }

    const vec2 xi = vec2(rand01(seed), rand01(seed));
    const vec3 hLocal = sampleGGXVNDF_Local(vLocal, alphaRoughness, xi);
    const vec3 h = worldFromLocal(n, tangent, bitangent, hLocal);

    l = normalize(reflect(-v, h));
    const float nDotL = dot(n, l);
    if (nDotL <= 0.0) {
        pdf = 0.0;
        return false;
    }

    const vec3 hLocalCheck = localFromWorld(n, tangent, bitangent, h);
    const float pdfH = pdfGGXVNDF_Local(vLocal, hLocalCheck, alphaRoughness);
    const float vDotH = max(dot(v, h), 1e-5);
    pdf = pdfH / (4.0 * vDotH);
    return pdf > 0.0;
}

float pdfGGXVNDFReflection(vec3 n, vec3 v, vec3 wi, float alphaRoughness)
{
    const vec3 h = normalize(v + wi);
    const float nDotL = dot(n, wi);
    const float nDotV = dot(n, v);
    if (nDotL <= 0.0 || nDotV <= 0.0) {
        return 0.0;
    }

    vec3 tangent;
    vec3 bitangent;
    buildOrthonormalBasis(n, tangent, bitangent);

    const vec3 vLocal = localFromWorld(n, tangent, bitangent, v);
    const vec3 hLocal = localFromWorld(n, tangent, bitangent, h);
    const float pdfH = pdfGGXVNDF_Local(vLocal, hLocal, alphaRoughness);
    const float vDotH = max(dot(v, h), 1e-5);
    return pdfH / (4.0 * vDotH);
}

vec3 sampleCosineHemisphere(vec3 normal, inout uint seed)
{
    const float r1 = rand01(seed);
    const float r2 = rand01(seed);
    const float phi = 2.0 * PI * r1;
    const float cosTheta = sqrt(1.0 - r2);
    const float sinTheta = sqrt(r2);

    vec3 tangent;
    vec3 bitangent;
    buildOrthonormalBasis(normal, tangent, bitangent);

    const vec3 local = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    return normalize(tangent * local.x + bitangent * local.y + normal * local.z);
}

bool refractSnell(vec3 wi, vec3 n, float eta, out vec3 wt)
{
    float cosThetaI = dot(n, wi);
    vec3 normal = n;
    float etap = eta;
    if (cosThetaI < 0.0) {
        normal = -n;
        cosThetaI = -cosThetaI;
        etap = 1.0 / eta;
    }

    const float sin2T = etap * etap * (1.0 - cosThetaI * cosThetaI);
    if (sin2T >= 1.0) {
        return false;
    }

    const float cosT = sqrt(1.0 - sin2T);
    wt = normalize(etap * (-wi) + (etap * cosThetaI - cosT) * normal);
    return true;
}

float dielectricFresnelReflectProb(vec3 n, vec3 v, PbrMaterial m)
{
    const float nDotV = max(dot(n, v), 0.0);
    const vec3 f0 = fresnelF0(m);
    return clamp(luminance(schlickFresnel(nDotV, f0)), 0.0, 0.999);
}

vec3 evalPbrBsdf(PbrMaterial m, vec3 n, vec3 v, vec3 l)
{
    const float nDotL = max(dot(n, l), 0.0);
    const float nDotV = max(dot(n, v), 0.0);
    if (nDotL <= 0.0 || nDotV <= 0.0) {
        return vec3(0.0);
    }

    const vec3 h = normalize(v + l);
    const float nDotH = max(dot(n, h), 0.0);
    const float lDotH = max(dot(l, h), 0.0);

    const vec3 f0 = fresnelF0(m);
    const vec3 F = schlickFresnel(lDotH, f0);

    const float alpha = max(m.roughness * m.roughness, 1e-4);
    const float D = distributionGTR2(nDotH, alpha);
    const float G = visibilitySmithGGXCorrelated(nDotV, nDotL, alpha);
    const vec3 spec = D * G * F / max(4.0 * nDotL * nDotV, 1e-5);

    const float opaque = 1.0 - m.transmission;
    const vec3 kd = (vec3(1.0) - F) * (1.0 - m.metallic) * opaque;
    const vec3 diffuse = kd * m.baseColor * (1.0 / PI);

    return diffuse + spec * opaque;
}

void computeLobeProbabilities(PbrMaterial m, vec3 n, vec3 v,
                              out float pDiff, out float pSpec, out float pTrans)
{
    pDiff = 0.0;
    pSpec = 0.0;
    pTrans = 0.0;

    const float nDotV = max(dot(n, v), 0.0);
    const vec3 f0 = fresnelF0(m);
    const float fresnel = clamp(luminance(schlickFresnel(nDotV, f0)), 0.0, 1.0);

    if (m.transmission > 0.01) {
        pSpec = fresnel;
        pTrans = 1.0 - fresnel;
        return;
    }

    if (m.metallic >= 1.0) {
        pSpec = 1.0;
        return;
    }

    const float diffW = max(luminance(m.baseColor), 0.01) * (1.0 - m.metallic);
    const float specW = max(fresnel, 0.01);
    const float sum = diffW + specW;
    pDiff = diffW / sum;
    pSpec = specW / sum;
}

vec3 evalGlassReflection(PbrMaterial m, vec3 n, vec3 v, vec3 l, float alpha)
{
    const float nDotL = max(dot(n, l), 0.0);
    const float nDotV = max(dot(n, v), 0.0);
    if (nDotL <= 0.0 || nDotV <= 0.0) {
        return vec3(0.0);
    }
    const vec3 h = normalize(v + l);
    const float nDotH = max(dot(n, h), 0.0);
    const float lDotH = max(dot(l, h), 0.0);
    const vec3 f0 = fresnelF0(m);
    const vec3 F = schlickFresnel(lDotH, f0);
    const float D = distributionGTR2(nDotH, alpha);
    const float G = visibilitySmithGGXCorrelated(nDotV, nDotL, alpha);
    return D * G * F / max(4.0 * nDotL * nDotV, 1e-5);
}

float pdfPbrBsdf(PbrMaterial m, vec3 n, vec3 v, vec3 wi)
{
    const float nDotL = dot(n, wi);
    if (nDotL <= 0.0 && m.transmission < 0.01) {
        return 0.0;
    }

    float pDiff;
    float pSpec;
    float pTrans;
    computeLobeProbabilities(m, n, v, pDiff, pSpec, pTrans);

    const float alpha = max(m.roughness * m.roughness, 1e-4);
    const float nDotWi = max(dot(n, wi), 0.0);

    float pdf = pDiff * pdfCosineHemisphere(nDotWi)
        + pSpec * pdfGGXVNDFReflection(n, v, wi, alpha);

    if (pTrans > 0.0 && nDotWi <= 0.0) {
        pdf += pTrans;
    }

    return pdf;
}

float powerHeuristic(float a, float b)
{
    const float a2 = a * a;
    const float b2 = b * b;
    return a2 / max(a2 + b2, 1e-16);
}

BsdfSample samplePbrBsdf(PbrMaterial m, vec3 n, vec3 v, float mediumIOR, inout uint seed)
{
    BsdfSample result;
    result.wi = vec3(0.0);
    result.pdf = 0.0;
    result.weight = vec3(0.0);
    result.isSpecular = false;
    result.isTransmissive = false;

    float pDiff;
    float pSpec;
    float pTrans;
    computeLobeProbabilities(m, n, v, pDiff, pSpec, pTrans);

    const float alpha = max(m.roughness * m.roughness, 1e-4);
    const float lobeRand = rand01(seed);

    if (m.transmission > 0.01) {
        const float nDotV = max(dot(n, v), 0.0);
        const vec3 f0 = fresnelF0(m);
        const vec3 F = schlickFresnel(nDotV, f0);
        const float Fr = dielectricFresnelReflectProb(n, v, m);
        const bool insideMedium = mediumIOR > 1.0001;
        const float eta = insideMedium ? (mediumIOR / 1.0) : (1.0 / m.ior);

        // 光滑玻璃：delta 反射/折射，避免 GGX pdf 极小导致边缘爆亮
        if (m.roughness < GLASS_DELTA_ROUGHNESS) {
            result.isSpecular = true;
            if (lobeRand < Fr) {
                result.wi = normalize(reflect(-v, n));
                result.pdf = Fr;
                result.weight = F / max(Fr, 1e-8);
            } else if (refractSnell(v, n, eta, result.wi)) {
                result.isTransmissive = true;
                result.pdf = 1.0 - Fr;
                result.weight = (vec3(1.0) - F) * m.baseColor / max(1.0 - Fr, 1e-8);
            } else {
                result.wi = normalize(reflect(-v, n));
                result.pdf = 1.0;
                result.weight = vec3(1.0);
            }
            return result;
        }

        if (lobeRand < Fr) {
            result.isSpecular = false;
            if (!sampleGGXVNDFReflection(n, v, alpha, seed, result.wi, result.pdf)) {
                return result;
            }
            result.pdf *= Fr;
            const float nDotL = max(dot(n, result.wi), 0.0);
            const vec3 fr = evalGlassReflection(m, n, v, result.wi, alpha);
            result.weight = fr * nDotL / max(result.pdf, 1e-8);
        } else {
            result.isSpecular = false;
            result.isTransmissive = true;
            if (!refractSnell(v, n, eta, result.wi)) {
                if (!sampleGGXVNDFReflection(n, v, alpha, seed, result.wi, result.pdf)) {
                    return result;
                }
                result.pdf = Fr;
                const float nDotL = max(dot(n, result.wi), 0.0);
                const vec3 fr = evalGlassReflection(m, n, v, result.wi, alpha);
                result.weight = fr * nDotL / max(result.pdf, 1e-8);
            } else {
                result.pdf = 1.0 - Fr;
                result.weight = (vec3(1.0) - F) * m.baseColor / max(1.0 - Fr, 1e-8);
            }
        }
        return result;
    }

    if (lobeRand < pDiff) {
        result.isSpecular = false;
        result.wi = sampleCosineHemisphere(n, seed);
        const float nDotL = dot(n, result.wi);
        if (nDotL <= 0.0) {
            return result;
        }
        result.pdf = pDiff * pdfCosineHemisphere(nDotL);
    } else {
        result.isSpecular = m.metallic >= 1.0 || m.roughness < 0.05;
        if (!sampleGGXVNDFReflection(n, v, alpha, seed, result.wi, result.pdf)) {
            return result;
        }
        result.pdf *= pSpec;
    }

    const float nDotL = max(dot(n, result.wi), 0.0);
    const vec3 fr = evalPbrBsdf(m, n, v, result.wi);
    result.weight = fr * nDotL / max(result.pdf, 1e-8);
    return result;
}

vec2 directionToEnvUv(vec3 direction)
{
    direction = normalize(direction);
    const float phi = atan(direction.z, direction.x);
    const float theta = acos(clamp(direction.y, -1.0, 1.0));
    return vec2(phi * (0.5 / PI) + 0.5, theta / PI);
}

vec3 uvToDirection(vec2 uv)
{
    const float phi = (uv.x - 0.5) * 2.0 * PI;
    const float theta = uv.y * PI;
    const float sinTheta = sin(theta);
    return normalize(vec3(cos(phi) * sinTheta, cos(theta), sin(phi) * sinTheta));
}

int envCdfSearchMarginal(sampler2D envCdf, float xi, int height)
{
    int left = 0;
    int right = height - 1;
    while (left < right) {
        const int mid = (left + right) >> 1;
        const float cdfVal = texelFetch(envCdf, ivec2(0, mid), 0).g;
        if (cdfVal < xi) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    return left;
}

int envCdfSearchConditional(sampler2D envCdf, int row, float xi, int width)
{
    int left = 0;
    int right = width - 1;
    while (left < right) {
        const int mid = (left + right) >> 1;
        const float cdfVal = texelFetch(envCdf, ivec2(mid, row), 0).r;
        if (cdfVal < xi) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    return left;
}

float environmentPdfAtCell(sampler2D envCdf, ivec2 cdfSize, int col, int row, vec2 uv)
{
    const float marginal = texelFetch(envCdf, ivec2(0, row), 0).g;
    const float marginalPrev = row > 0 ? texelFetch(envCdf, ivec2(0, row - 1), 0).g : 0.0;
    const float rowProb = max(marginal - marginalPrev, 0.0);

    const float cond = texelFetch(envCdf, ivec2(col, row), 0).r;
    const float condPrev = col > 0 ? texelFetch(envCdf, ivec2(col - 1, row), 0).r : 0.0;
    const float colProb = max(cond - condPrev, 0.0);

    const float sinTheta = max(sin(uv.y * PI), 1e-6);
    return rowProb * colProb * float(cdfSize.x * cdfSize.y) / (2.0 * PI * PI * sinTheta);
}

float environmentPdf(sampler2D envCdf, ivec2 cdfSize, vec3 direction)
{
    const vec2 uv = directionToEnvUv(direction);
    const int col = clamp(int(floor(uv.x * float(cdfSize.x))), 0, cdfSize.x - 1);
    const int row = clamp(int(floor(uv.y * float(cdfSize.y))), 0, cdfSize.y - 1);
    return environmentPdfAtCell(envCdf, cdfSize, col, row, uv);
}

struct EnvLightSample {
    vec3 wi;
    vec3 radiance;
    float pdf;
};

EnvLightSample sampleEnvironmentLight(sampler2D envMap, sampler2D envCdf, ivec2 cdfSize, inout uint seed)
{
    EnvLightSample result;
    result.wi = vec3(0.0);
    result.radiance = vec3(0.0);
    result.pdf = 0.0;

    const float xi = rand01(seed);
    const float yi = rand01(seed);
    const int row = envCdfSearchMarginal(envCdf, xi, cdfSize.y);
    const int col = envCdfSearchConditional(envCdf, row, yi, cdfSize.x);

    const vec2 uv = (vec2(float(col), float(row)) + vec2(rand01(seed), rand01(seed))) / vec2(cdfSize);
    result.wi = uvToDirection(uv);
    result.radiance = texture(envMap, uv).rgb;
    result.pdf = environmentPdfAtCell(envCdf, cdfSize, col, row, uv);
    return result;
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
