// 路径追踪公用函数（由 raygen / closesthit / miss include）

const uint MAX_BOUNCES = 10u;
const uint SAMPLES_PER_PIXEL = 8u;
const float PI = 3.14159265358979323846;
const float EXPOSURE = 0.5; // EV 档，tone map 前亮度 *= 2^EXPOSURE

struct PathPayload {
    vec4 hitNormal; // x: 1=命中, yzw: 世界空间法线
    vec4 position;  // xyz: 命中点
    vec4 material0; // baseColor.rgb, roughness
    vec4 material1; // subSurface, sheen, sheenTint, metallic
    vec4 material2; // specular, specularTint, clearcoat, clearcoatGloss
};

struct DisneyMaterial {
    vec3 baseColor;
    float roughness;
    float subSurface;
    float sheen;
    float sheenTint;
    float metallic;
    float specular;
    float specularTint;
    float clearcoat;
    float clearcoatGloss;
};

struct BsdfSample {
    vec3 wi;
    float pdf;
    vec3 weight; // f_r * cos(theta_i) / pdf
};

DisneyMaterial unpackMaterial(PathPayload payload)
{
    DisneyMaterial m;
    m.baseColor = payload.material0.rgb;
    m.roughness = payload.material0.w;
    m.subSurface = payload.material1.x;
    m.sheen = payload.material1.y;
    m.sheenTint = payload.material1.z;
    m.metallic = payload.material1.w;
    m.specular = payload.material2.x;
    m.specularTint = payload.material2.y;
    m.clearcoat = payload.material2.z;
    m.clearcoatGloss = payload.material2.w;
    return m;
}

void packMaterial(inout PathPayload payload, DisneyMaterial m)
{
    payload.material0 = vec4(m.baseColor, m.roughness);
    payload.material1 = vec4(m.subSurface, m.sheen, m.sheenTint, m.metallic);
    payload.material2 = vec4(m.specular, m.specularTint, m.clearcoat, m.clearcoatGloss);
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

vec3 specularF0(DisneyMaterial m)
{
    const vec3 dielectric = vec3(0.08) * m.specular * mix(vec3(1.0), m.baseColor, m.specularTint);
    return mix(dielectric, m.baseColor, m.metallic);
}

vec3 evalDisneyDiffuse(DisneyMaterial m, vec3 n, vec3 v, vec3 l)
{
    if (m.metallic >= 1.0) {
        return vec3(0.0);
    }

    const vec3 h = normalize(l + v);
    const float nDotL = clamp(dot(n, l), 0.0, 1.0);
    const float nDotV = clamp(dot(n, v), 0.0, 1.0);
    const float lDotH = clamp(dot(l, h), 0.0, 1.0);
    const float nDotH = clamp(dot(n, h), 0.0, 1.0);

    const float fd90 = 0.5 + 2.0 * lDotH * lDotH * m.roughness;
    const float fl = pow(1.0 - nDotL, 5.0);
    const float fv = pow(1.0 - nDotV, 5.0);
    float fd = mix(1.0, fd90, fl) * mix(1.0, fd90, fv);

    const float fss90 = lDotH * lDotH * m.roughness;
    const float fss = mix(1.0, fss90, fl) * mix(1.0, fss90, fv);
    fd = mix(fd, fss, m.subSurface);

    vec3 diffuse = fd * m.baseColor * (1.0 / PI);

    const vec3 csheen = mix(vec3(1.0), m.baseColor, m.sheenTint);
    diffuse += m.sheen * schlickFresnel(nDotH, csheen) * (1.0 / PI);

    return diffuse;
}

vec3 evalDisneySpecular(DisneyMaterial m, vec3 n, vec3 v, vec3 l)
{
    const vec3 h = normalize(l + v);
    const float nDotL = clamp(dot(n, l), 0.0, 1.0);
    const float nDotV = clamp(dot(n, v), 0.0, 1.0);
    const float nDotH = clamp(dot(n, h), 0.0, 1.0);
    const float lDotH = clamp(dot(l, h), 0.0, 1.0);

    if (nDotL <= 0.0 || nDotV <= 0.0) {
        return vec3(0.0);
    }

    const float alpha = max(m.roughness * m.roughness, 1e-4);
    const float d = distributionGTR2(nDotH, alpha);
    const float g = visibilitySmithGGXCorrelated(nDotV, nDotL, alpha);
    const vec3 f0 = specularF0(m);
    const vec3 f = schlickFresnel(lDotH, f0);

    vec3 spec = d * g * f / max(4.0 * nDotL * nDotV, 1e-5);

    const float ccAlpha = mix(0.1, 0.001, m.clearcoatGloss);
    const float dr = distributionGTR2(nDotH, ccAlpha);
    const float gr = visibilitySmithGGXCorrelated(nDotV, nDotL, ccAlpha);
    const float fr = mix(0.04, 1.0, schlickWeight(lDotH));
    spec += 0.25 * m.clearcoat * dr * gr * fr;

    return spec;
}

vec3 evalDisneyBsdf(DisneyMaterial m, vec3 n, vec3 v, vec3 l)
{
    return evalDisneyDiffuse(m, n, v, l) + evalDisneySpecular(m, n, v, l);
}

float pdfCosineHemisphere(float nDotL)
{
    return max(nDotL, 0.0) / PI;
}

float pdfGGXReflection(vec3 n, vec3 v, vec3 h, float alphaRoughness)
{
    const float nDotH = clamp(dot(n, h), 0.0, 1.0);
    const float vDotH = clamp(dot(v, h), 0.0, 1.0);
    const float d = distributionGTR2(nDotH, alphaRoughness);
    return d * nDotH / max(4.0 * vDotH, 1e-5);
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

bool sampleGGXReflection(vec3 n, vec3 v, float alphaRoughness, inout uint seed, out vec3 l, out float pdf)
{
    const float r1 = rand01(seed);
    const float r2 = rand01(seed);
    const float phi = 2.0 * PI * r1;
    const float cosTheta = sqrt((1.0 - r2) / (1.0 + (alphaRoughness * alphaRoughness - 1.0) * r2));
    const float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));

    vec3 tangent;
    vec3 bitangent;
    buildOrthonormalBasis(n, tangent, bitangent);

    const vec3 hLocal = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    const vec3 h = normalize(tangent * hLocal.x + bitangent * hLocal.y + n * hLocal.z);

    l = normalize(reflect(-v, h));
    const float nDotL = dot(n, l);
    if (nDotL <= 0.0) {
        pdf = 0.0;
        return false;
    }

    pdf = pdfGGXReflection(n, v, h, alphaRoughness);
    return pdf > 0.0;
}

float specularSamplingProbability(DisneyMaterial m, vec3 n, vec3 v)
{
    const float nDotV = clamp(dot(n, v), 0.0, 1.0);
    const vec3 f0 = specularF0(m);
    const vec3 fresnel = schlickFresnel(nDotV, f0);
    const float specWeight = clamp(max(fresnel.r, max(fresnel.g, fresnel.b)), 0.0, 1.0);
    return clamp(mix(specWeight, 1.0, m.metallic), 0.05, 0.95);
}

BsdfSample sampleDisneyBsdf(DisneyMaterial m, vec3 n, vec3 v, inout uint seed)
{
    BsdfSample result;
    result.wi = vec3(0.0);
    result.pdf = 0.0;
    result.weight = vec3(0.0);

    const float alpha = max(m.roughness * m.roughness, 1e-4);
    const float specProb = specularSamplingProbability(m, n, v);
    const bool sampleSpecular = rand01(seed) < specProb;

    if (sampleSpecular) {
        if (!sampleGGXReflection(n, v, alpha, seed, result.wi, result.pdf)) {
            return result;
        }
        result.pdf *= specProb;
    } else {
        result.wi = sampleCosineHemisphere(n, seed);
        const float nDotL = dot(n, result.wi);
        if (nDotL <= 0.0) {
            result.pdf = 0.0;
            return result;
        }
        result.pdf = (1.0 - specProb) * pdfCosineHemisphere(nDotL);
    }

    const float nDotL = dot(n, result.wi);
    const vec3 fr = evalDisneyBsdf(m, n, v, result.wi);
    result.weight = fr * nDotL / max(result.pdf, 1e-8);
    return result;
}

vec3 sampleEnvironment(sampler2D envMap, vec3 direction)
{
    direction = normalize(direction);
    const float phi = atan(direction.z, direction.x);
    const float theta = acos(clamp(direction.y, -1.0, 1.0));
    const vec2 uv = vec2(phi * (0.5 / PI) + 0.5, theta / PI);
    return texture(envMap, uv).rgb;
}

vec3 toneMap(vec3 color)
{
    color *= exp2(EXPOSURE);
    color = color / (color + vec3(1.0));
    return pow(color, vec3(1.0 / 2.2));
}
