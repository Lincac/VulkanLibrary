#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec2 vUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Scene {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;   // xyz: world-space camera position
    vec4 lightDir;    // xyz: light direction *from* light *to* surface (or opposite, see below)
    vec4 lightColor;  // rgb: light color, a: intensity (optional)
} uScene;

layout(set = 1, binding = 0) uniform MaterialParams {
    vec3  baseColor;
    float roughness;
    float subSurface;
    float sheen;
    float sheenTint;
    
    float metallic;
    float specular;
    float specularTint;
    
    float clearcoat;
    float clearcoatGloss;
} uMat;

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

const float PI = 3.14159265359;

float saturate(float x) {
    return clamp(x, 0.0, 1.0);
}

vec3 saturate(vec3 x) {
    return clamp(x, 0.0, 1.0);
}

float pow5(float x) {
    float x2 = x * x;
    return x2 * x2 * x;
}

// Schlick Fresnel
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow5(1.0 - cosTheta);
}

// GGX / Trowbridge-Reitz NDF
float D_GGX(float NdotH, float alpha) {
    float a2 = alpha * alpha;
    float d  = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d + 1e-7);
}

// Smith GGX geometry term (separable)
float G_Smith_SchlickGGX(float NdotV, float NdotL, float alpha) {
    float k = (alpha + 1.0);
    k = (k * k) / 8.0;

    float gV = NdotV / (NdotV * (1.0 - k) + k);
    float gL = NdotL / (NdotL * (1.0 - k) + k);
    return gV * gL;
}

// Clearcoat uses a simpler GTR1 distribution
float D_GTR1(float NdotH, float alpha) {
    float a2 = alpha * alpha;
    float d  = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return (a2 - 1.0) / (PI * log(a2) * d);
}

// ------------------------------------------------------------
// Disney BRDF core
// ------------------------------------------------------------

void main()
{
    // --------------------------------------------------------
    // Basis vectors
    // --------------------------------------------------------
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uScene.cameraPos.xyz - vWorldPos);

    // 这里假设 lightDir.xyz 是从光源指向场景的方向（即“光线方向”）
    // 如果你存的是从表面指向光源，就改成 normalize(uScene.lightDir.xyz)
    vec3 L = normalize(-uScene.lightDir.xyz);
    vec3 H = normalize(L + V);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float LdotH = saturate(dot(L, H));

    if (NdotL <= 0.0 || NdotV <= 0.0) {
        outColor = vec4(0.0);
        return;
    }

    // --------------------------------------------------------
    // Material parameters
    // --------------------------------------------------------
    float roughness     = saturate(uMat.roughness);
    float metallic      = saturate(uMat.metallic);
    float specular      = saturate(uMat.specular);
    float specularTint  = saturate(uMat.specularTint);
    float sheen         = saturate(uMat.sheen);
    float sheenTint     = saturate(uMat.sheenTint);
    float clearcoat     = saturate(uMat.clearcoat);
    float clearcoatGloss= saturate(uMat.clearcoatGloss);
    float subsurface    = saturate(uMat.subSurface);

    vec3  baseColor     = saturate(uMat.baseColor);

    // --------------------------------------------------------
    // Specular F0 (Disney style)
    // --------------------------------------------------------
    // Base specular from IOR ~ 1.5 → F0 ≈ 0.04
    float dielectricF0 = 0.08 * specular;
    vec3  Ctint = vec3(0.0);
    float lum = dot(baseColor, vec3(0.3, 0.6, 0.1));
    if (lum > 0.0) {
        Ctint = baseColor / lum;
    }

    vec3 F0_dielectric = mix(vec3(dielectricF0), dielectricF0 * Ctint, specularTint);
    vec3 F0 = mix(F0_dielectric, baseColor, metallic);

    // --------------------------------------------------------
    // Microfacet specular (GGX)
    // --------------------------------------------------------
    float alpha = max(0.001, roughness * roughness);

    float  D = D_GGX(NdotH, alpha);
    float  G = G_Smith_SchlickGGX(NdotV, NdotL, alpha);
    vec3   F = fresnelSchlick(LdotH, F0);

    vec3  specularTerm = (D * G * F) / (4.0 * NdotL * NdotV + 1e-7);

    // --------------------------------------------------------
    // Disney diffuse + subsurface
    // --------------------------------------------------------
    float FL = pow5(1.0 - NdotL);
    float FV = pow5(1.0 - NdotV);
    float Fd90 = 0.5 + 2.0 * roughness * LdotH * LdotH;

    float lightScatter = 1.0 + (Fd90 - 1.0) * FL;
    float viewScatter  = 1.0 + (Fd90 - 1.0) * FV;

    float diffuseTerm = lightScatter * viewScatter;

    // Subsurface tweak（Disney 的近似）
    float Fss90 = LdotH * LdotH * roughness;
    float ssScatterL = 1.0 + (Fss90 - 1.0) * FL;
    float ssScatterV = 1.0 + (Fss90 - 1.0) * FV;
    float ss = 1.25 * (ssScatterL * ssScatterV * (1.0 / (NdotL + NdotV + 1e-7)) - 0.5);

    float diffuseMix = mix(diffuseTerm, ss, subsurface);
    vec3  diffuse = (1.0 - metallic) * baseColor * diffuseMix / PI;

    // --------------------------------------------------------
    // Sheen
    // --------------------------------------------------------
    vec3 Csheen = mix(vec3(1.0), Ctint, sheenTint);
    vec3 sheenTerm = sheen * Csheen * pow5(1.0 - LdotH);

    // --------------------------------------------------------
    // Clearcoat (GTR1)
    // --------------------------------------------------------
    float clearAlpha = mix(0.1, 0.001, clearcoatGloss);
    float Dc = D_GTR1(NdotH, clearAlpha);
    float Fc = mix(0.04, 1.0, pow5(1.0 - LdotH));
    float Gc = G_Smith_SchlickGGX(NdotV, NdotL, 0.25); // fixed roughness for clearcoat

    float clearcoatTerm = clearcoat * 0.25 * (Dc * Fc * Gc / (NdotL * NdotV + 1e-7));

    // --------------------------------------------------------
    // Final shading
    // --------------------------------------------------------
    vec3 lightColor = uScene.lightColor.rgb;
    float lightIntensity = (uScene.lightColor.a != 0.0) ? uScene.lightColor.a : 1.0;

    vec3 brdf =
        diffuse +
        specularTerm +
        sheenTerm +
        vec3(clearcoatTerm);

    vec3 color = brdf * lightColor * lightIntensity * NdotL;

    outColor = vec4(color, 1.0);
}