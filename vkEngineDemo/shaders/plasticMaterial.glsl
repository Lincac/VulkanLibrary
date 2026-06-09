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

BsdfSample smooth_plastic_sample(vec3 rho, vec3 specColor, float etaI, float etaT, vec3 wi, vec2 xi)
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
        s.f = specColor * F / wi.z;         
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