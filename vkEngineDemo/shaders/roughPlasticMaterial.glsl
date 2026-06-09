bool sameHemisphere(vec3 a, vec3 b) 
{
    return a.z * b.z > 0.0;
}

vec3 reflectLocal(vec3 wi, vec3 m)
 {
    // wi, m 都是单位向量
    return 2.0 * dot(wi, m) * m - wi;
}

float D_GGX(float alpha, float NoH) 
{
    float a2 = alpha * alpha;
    float d = (NoH * NoH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

float G1_Smith_GGX(float alpha, float NoV)
 {
    float a2 = alpha * alpha;
    float denom = NoV + sqrt(a2 + (1.0 - a2) * NoV * NoV);
    return 2.0 * NoV / denom;
}

float G_Smith_GGX(float alpha, float NoV, float NoL) 
{
    return G1_Smith_GGX(alpha, NoV) * G1_Smith_GGX(alpha, NoL);
}

vec3 sampleGGX(float alpha, vec2 xi) 
{
    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (alpha * alpha - 1.0) * xi.y));
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));

    return vec3(
        sinTheta * cos(phi),
        sinTheta * sin(phi),
        cosTheta
    );
}

vec3 roughplastic_eval_specular(RoughPlastic mat, vec3 wi, vec3 wo)
 {
    float NoI = wi.z;
    float NoO = wo.z;
    if (NoI <= 0.0 || NoO <= 0.0)
        return vec3(0.0);

    if (!sameHemisphere(wi, wo))
        return vec3(0.0);

    vec3 H = normalize(wi + wo);
    float NoH = max(0.0, H.z);
    float VoH = max(0.0, dot(wo, H));

    float D = D_GGX(mat.alpha, NoH);
    float G = G_Smith_GGX(mat.alpha, NoI, NoO);
    float F = fresnelDielectric(VoH, mat.ext_ior, mat.int_ior);

    // microfacet BRDF
    float denom = 4.0 * NoI * NoO;
    vec3 Fr = mat.specular_reflectance * F * D * G / max(denom, 1e-6);

    return Fr;
}

vec3 roughplastic_eval_diffuse(RoughPlastic mat, vec3 wi, vec3 wo) 
{
    float NoI = wi.z;
    float NoO = wo.z;
    if (NoI <= 0.0 || NoO <= 0.0)
        return vec3(0.0);

    // 入射方向的 Fresnel（外→内）
    float Fi = fresnelDielectric(NoI, mat.ext_ior, mat.int_ior);
    // 出射方向的 Fresnel（内→外）
    float Fo = fresnelDielectric(NoO, mat.int_ior, mat.ext_ior);

    // 简化版 Mitsuba：rho_d / PI * (1 - Fi) * (1 - Fo)
    vec3 fd = mat.diffuse_reflectance * (1.0 - Fi) * (1.0 - Fo) * (1.0 / PI);
    return fd;
}

vec3 rough_plastic_eval(RoughPlastic mat, vec3 wi, vec3 wo) 
{
    if (wi.z <= 0.0 || wo.z <= 0.0)
        return vec3(0.0);

    vec3 fs = roughplastic_eval_specular(mat, wi, wo);
    vec3 fd = roughplastic_eval_diffuse(mat, wi, wo);
    return fs + fd;
}

float roughplastic_pdf_specular(RoughPlastic mat, vec3 wi, vec3 wo) 
{
    float NoI = wi.z;
    float NoO = wo.z;
    if (NoI <= 0.0 || NoO <= 0.0)
        return 0.0;

    if (!sameHemisphere(wi, wo))
        return 0.0;

    vec3 H = normalize(wi + wo);
    float NoH = max(0.0, H.z);
    float VoH = max(0.0, dot(wo, H));

    // GGX NDF 采样的半程向量 pdf：p(H) = D * NoH
    float pH = D_GGX(mat.alpha, NoH) * NoH;
    // 方向变换：p(wo) = p(H) * |dH/dwo| = p(H) * 1 / (4 * VoH)
    float pWo = pH / max(4.0 * VoH, 1e-6);
    return pWo;
}

float roughplastic_pdf_diffuse(RoughPlastic mat, vec3 wi, vec3 wo) 
{
    float NoI = wi.z;
    float NoO = wo.z;
    if (NoI <= 0.0 || NoO <= 0.0)
        return 0.0;

    // 漫反射半球余弦采样：p(wo) = cos / PI
    return NoO * (1.0 / PI);
}

float rough_plastic_pdf(RoughPlastic mat, vec3 wi, vec3 wo) 
{
    float NoI = wi.z;
    if (NoI <= 0.0)
        return 0.0;

    // Fresnel 决定 lobe 权重（入射方向）
    float F = fresnelDielectric(NoI, mat.ext_ior, mat.int_ior);

    float ps = roughplastic_pdf_specular(mat, wi, wo);
    float pd = roughplastic_pdf_diffuse(mat, wi, wo);

    return F * ps + (1.0 - F) * pd;
}

BsdfSample rough_plastic_sample(RoughPlastic mat, vec3 wi, vec2 xi) {
    BsdfSample s;
    s.wo      = vec3(0.0, 0.0, 1.0);
    s.f       = vec3(0.0);
    s.pdf     = 0.0;
    s.isDelta = false;

    float NoI = wi.z;
    if (NoI <= 0.0)
        return s;

    // 入射方向的 Fresnel（外→内），决定 lobe 权重
    float F = fresnelDielectric(NoI, mat.ext_ior, mat.int_ior);

    // 轮盘赌：specular / diffuse
    bool chooseSpec = (xi.x < F);
    vec2 xiRemap = vec2(
        chooseSpec ? xi.y : (xi.x - F) / max(1.0 - F, 1e-6),
        chooseSpec ? fract(xi.x * 17.0 + xi.y * 131.0) : xi.y
    );

    if (chooseSpec) {
        // --- specular lobe ---
        vec3 H = sampleGGX(mat.alpha, xiRemap);
        if (dot(wi, H) <= 0.0) {
            s.pdf = 0.0;
            s.f   = vec3(0.0);
            return s;
        }

        vec3 wo = reflectLocal(wi, H);
        if (wo.z <= 0.0) {
            s.pdf = 0.0;
            s.f   = vec3(0.0);
            return s;
        }

        s.wo = wo;

        float NoO = wo.z;
        float NoH = max(0.0, H.z);
        float VoH = max(0.0, dot(wo, H));

        float D = D_GGX(mat.alpha, NoH);
        float G = G_Smith_GGX(mat.alpha, NoI, NoO);
        float Fs = fresnelDielectric(VoH, mat.ext_ior, mat.int_ior);

        float denom = 4.0 * NoI * NoO;
        vec3 Fr = mat.specular_reflectance * Fs * D * G / max(denom, 1e-6);

        float ps = roughplastic_pdf_specular(mat, wi, wo);
        float pd = roughplastic_pdf_diffuse(mat, wi, wo);
        float pdf = F * ps + (1.0 - F) * pd;

        s.f   = Fr;
        s.pdf = pdf;
        s.isDelta = false; // 粗糙 specular 不是 delta
    } else {
        // --- diffuse lobe ---
        vec3 wo = cosineSampleHemisphere(xiRemap);
        if (wo.z <= 0.0) {
            s.pdf = 0.0;
            s.f   = vec3(0.0);
            return s;
        }

        s.wo = wo;

        float NoO = wo.z;

        float Fi = fresnelDielectric(NoI, mat.ext_ior, mat.int_ior);
        float Fo = fresnelDielectric(NoO, mat.int_ior, mat.ext_ior);

        vec3 fd = mat.diffuse_reflectance * (1.0 - Fi) * (1.0 - Fo) * (1.0 / PI);

        float ps = roughplastic_pdf_specular(mat, wi, wo);
        float pd = roughplastic_pdf_diffuse(mat, wi, wo);
        float pdf = F * ps + (1.0 - F) * pd;

        s.f   = fd;
        s.pdf = pdf;
        s.isDelta = false;
    }

    return s;
}