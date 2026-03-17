#version 450

// ===== Feature toggles =====
// #define USE_BASECOLOR_MAP
// #define USE_METALROUGH_MAP
// #define USE_NORMAL_MAP
// #define USE_EMISSIVE_MAP
// #define ALPHA_MODE_OPAQUE
// #define ALPHA_MODE_MASK
// #define ALPHA_MODE_BLEND

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec2 vUV;

layout(location = 0) out vec4 outColor;

// ---- Material parameters ----
layout(set = 1, binding = 0) uniform MaterialParams {
    vec4  baseColorFactor;   // rgb: baseColor, a: alpha
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
    vec3  emissiveFactor;
    float alphaCutoff;       // for mask
} uMat;

// ---- Textures ----
layout(set = 1, binding = 1) uniform sampler2D uBaseColorMap;
layout(set = 1, binding = 2) uniform sampler2D uMetalRoughMap; // B: metallic, G: roughness
layout(set = 1, binding = 3) uniform sampler2D uNormalMap;
layout(set = 1, binding = 4) uniform sampler2D uOcclusionMap;
layout(set = 1, binding = 5) uniform sampler2D uEmissiveMap;

// ---- Lighting inputs ----
layout(set = 0, binding = 0) uniform Camera {
    vec3 cameraPos;
} uCam;

layout(set = 0, binding = 1) uniform DirectionalLight {
    vec3 direction;
    vec3 color;
} uDirLight;

// IBL hooks（你可以接入 prefiltered env + brdfLUT）
layout(set = 0, binding = 2) uniform samplerCube uEnvIrradiance;
layout(set = 0, binding = 3) uniform samplerCube uEnvSpecular;
layout(set = 0, binding = 4) uniform sampler2D   uBRDFLUT;

// ===== Utility =====
const float PI = 3.14159265359;

vec3 getNormal()
{
#ifdef USE_NORMAL_MAP
    vec3 N = normalize(vNormal);
    vec3 T = normalize(vTangent);
    vec3 B = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    vec3 n = texture(uNormalMap, vUV).xyz * 2.0 - 1.0;
    n.xy *= uMat.normalScale;
    return normalize(TBN * n);
#else
    return normalize(vNormal);
#endif
}

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec3  saturate(vec3  x) { return clamp(x, 0.0, 1.0); }

// ===== PBR BRDF (GGX) =====
float D_GGX(float NoH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = (NoH * NoH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d + 1e-5);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;

    float gv = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
    float gl = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
    return 0.5 / (gv + gl + 1e-5);
}

vec3 F_Schlick(vec3 F0, float VoH)
{
    float f = pow(1.0 - VoH, 5.0);
    return F0 + (1.0 - F0) * f;
}

// ===== Main =====
void main()
{
    // ---- BaseColor ----
    vec4 baseColor = uMat.baseColorFactor;
#ifdef USE_BASECOLOR_MAP
    baseColor *= texture(uBaseColorMap, vUV);
#endif

    // ---- Alpha handling ----
#ifdef ALPHA_MODE_MASK
    if (baseColor.a < uMat.alphaCutoff)
        discard;
#endif

    // ---- Metallic / Roughness ----
    float metallic  = uMat.metallicFactor;
    float roughness = uMat.roughnessFactor;
#ifdef USE_METALROUGH_MAP
    vec4 mr = texture(uMetalRoughMap, vUV);
    metallic  *= mr.b;
    roughness *= mr.g;
#endif
    roughness = saturate(roughness);
    metallic  = saturate(metallic);

    // ---- Normal ----
    vec3 N = getNormal();
    vec3 V = normalize(uCam.cameraPos - vWorldPos);
    vec3 L = normalize(-uDirLight.direction);
    vec3 H = normalize(V + L);

    float NoV = saturate(dot(N, V));
    float NoL = saturate(dot(N, L));
    float NoH = saturate(dot(N, H));
    float VoH = saturate(dot(V, H));

    if (NoL <= 0.0 || NoV <= 0.0) {
        outColor = vec4(0.0);
        return;
    }

    // ---- F0 ----
    vec3 dielectricF0 = vec3(0.04);
    vec3 F0 = mix(dielectricF0, baseColor.rgb, metallic);

    // ---- BRDF ----
    float  D = D_GGX(NoH, roughness);
    float  Vv = V_SmithGGXCorrelated(NoV, NoL, roughness);
    vec3   F = F_Schlick(F0, VoH);

    vec3  specular = (D * Vv) * F;
    vec3  kd = (1.0 - F) * (1.0 - metallic);
    vec3  diffuse = kd * baseColor.rgb / PI;

    vec3  direct = (diffuse + specular) * uDirLight.color * NoL;

    // ---- IBL（简单 hook，可自行增强）----
    vec3 R = reflect(-V, N);
    vec3 irradiance = texture(uEnvIrradiance, N).rgb;
    vec3 diffuseIBL = irradiance * baseColor.rgb;

    float mipCount = 5.0; // 根据你的 specular cubemap mip 数
    float lod = roughness * mipCount;
    vec3 prefiltered = textureLod(uEnvSpecular, R, lod).rgb;
    vec2 brdf = texture(uBRDFLUT, vec2(NoV, roughness)).rg;
    vec3 specIBL = prefiltered * (F0 * brdf.x + brdf.y);

    vec3 ibl = diffuseIBL * kd + specIBL;

    // ---- Occlusion ----
    float ao = 1.0;
    ao *= 1.0;
#ifdef USE_OCCLUSION_MAP
    ao *= mix(1.0, texture(uOcclusionMap, vUV).r, uMat.occlusionStrength);
#endif

    // ---- Emissive ----
    vec3 emissive = uMat.emissiveFactor;
#ifdef USE_EMISSIVE_MAP
    emissive *= texture(uEmissiveMap, vUV).rgb;
#endif

    vec3 color = direct + ibl * ao + emissive;

    // ---- Tone mapping / gamma（简单版本，留接口）----
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

#ifdef ALPHA_MODE_BLEND
    outColor = vec4(color, baseColor.a);
#else
    outColor = vec4(color, 1.0);
#endif
}
