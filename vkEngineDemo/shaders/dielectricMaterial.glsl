// Mitsuba smooth dielectric（光滑玻璃）：delta 反射 + delta 透射

float dielectric_eta(Dielectric mat)
{
    return mat.int_ior / mat.ext_ior;
}

float dielectric_inv_eta(Dielectric mat)
{
    return mat.ext_ior / mat.int_ior;
}

// eta = intIOR / extIOR；cosThetaT 与 cosThetaI 异号
float fresnelDielectricExt(float cosThetaI, out float cosThetaT, float eta)
{
    float cosI = cosThetaI;
    float etaRel = eta;

    if (cosI < 0.0) {
        etaRel = 1.0 / eta;
        cosI = -cosI;
    }

    float sin2ThetaI = max(0.0, 1.0 - cosI * cosI);
    float sin2ThetaT = sin2ThetaI / (etaRel * etaRel);

    if (sin2ThetaT >= 1.0) {
        cosThetaT = 0.0;
        return 1.0;
    }

    float cosT = sqrt(max(0.0, 1.0 - sin2ThetaT));
    cosThetaT = (cosThetaI > 0.0) ? -cosT : cosT;

    float rPar  = (etaRel * cosI - cosT) / (etaRel * cosI + cosT);
    float rPerp = (cosI - etaRel * cosT) / (cosI + etaRel * cosT);
    return clamp(0.5 * (rPar * rPar + rPerp * rPerp), 0.0, 1.0);
}

vec3 dielectricReflectLocal(vec3 wi)
{
    return vec3(-wi.x, -wi.y, wi.z);
}

vec3 dielectricRefractLocal(vec3 wi, float cosThetaT, float eta, float invEta)
{
    float scale = -(cosThetaT < 0.0 ? invEta : eta);
    return vec3(scale * wi.x, scale * wi.y, cosThetaT);
}

float dielectricRadianceFactor(float cosThetaT, float eta, float invEta)
{
    return (cosThetaT < 0.0) ? invEta : eta;
}

bool isDielectricReflection(vec3 wi, vec3 wo)
{
    vec3 wr = dielectricReflectLocal(wi);
    return dot(wr, wo) > 1.0 - 1e-4;
}

bool isDielectricTransmission(vec3 wi, vec3 wo, float cosThetaT, float eta, float invEta)
{
    vec3 wt = dielectricRefractLocal(wi, cosThetaT, eta, invEta);
    return dot(wt, wo) > 1.0 - 1e-4;
}

vec3 smooth_dielectric_eval(Dielectric mat, vec3 wi, vec3 wo)
{
    float cosI = wi.z;
    float cosO = wo.z;
    if (abs(cosI) < 1e-6 || abs(cosO) < 1e-6)
        return vec3(0.0);

    float cosThetaT;
    float eta    = dielectric_eta(mat);
    float invEta = dielectric_inv_eta(mat);
    float F      = fresnelDielectricExt(cosI, cosThetaT, eta);
    float absCosI = max(abs(cosI), 1e-6);

    if (cosI * cosO >= 0.0) {
        if (!isDielectricReflection(wi, wo))
            return vec3(0.0);
        return vec3(F / absCosI);
    }

    if (!isDielectricTransmission(wi, wo, cosThetaT, eta, invEta))
        return vec3(0.0);

    float factor = dielectricRadianceFactor(cosThetaT, eta, invEta);
    return vec3(factor * factor * (1.0 - F) / absCosI);
}

// 渲染方程被积函数 f(wi, wo) * cos(wo)；wo 为入射光方向
vec3 smooth_dielectric_eval_radiance(Dielectric mat, vec3 wi, vec3 wo)
{
    if (abs(wi.z) < 1e-6 || abs(wo.z) < 1e-6)
        return vec3(0.0);

    return smooth_dielectric_eval(mat, wi, wo) * wo.z;
}

float smooth_dielectric_pdf(Dielectric mat, vec3 wi, vec3 wo)
{
    float cosI = wi.z;
    float cosO = wo.z;
    if (abs(cosI) < 1e-6 || abs(cosO) < 1e-6)
        return 0.0;

    float cosThetaT;
    float eta = dielectric_eta(mat);
    float invEta = dielectric_inv_eta(mat);
    float F = fresnelDielectricExt(cosI, cosThetaT, eta);

    if (cosI * cosO >= 0.0) {
        if (!isDielectricReflection(wi, wo))
            return 0.0;
        return F;
    }

    if (!isDielectricTransmission(wi, wo, cosThetaT, eta, invEta))
        return 0.0;

    return 1.0 - F;
}

BsdfSample smooth_dielectric_sample(Dielectric mat, vec3 wi, vec2 xi)
{
    BsdfSample s;
    s.wo      = vec3(0.0, 0.0, 1.0);
    s.f       = vec3(0.0);
    s.pdf     = 0.0;
    s.isDelta = true;

    float cosI = wi.z;
    if (abs(cosI) < 1e-6)
        return s;

    float cosThetaT;
    float eta    = dielectric_eta(mat);
    float invEta = dielectric_inv_eta(mat);
    float F      = fresnelDielectricExt(cosI, cosThetaT, eta);
    float absCosI = max(abs(cosI), 1e-6);

    if (xi.x <= F) {
        s.wo  = dielectricReflectLocal(wi);
        s.f   = vec3(F / absCosI);
        s.pdf = F;
    } else {
        s.wo  = dielectricRefractLocal(wi, cosThetaT, eta, invEta);
        float factor = dielectricRadianceFactor(cosThetaT, eta, invEta);
        s.f   = vec3(factor * factor * (1.0 - F) / absCosI);
        s.pdf = max(1.0 - F, 1e-6);
    }

    s.isDelta = true;
    return s;
}
