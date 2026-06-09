float fresnelDielectric(float cosThetaI, float etaI, float etaT)
{
    cosThetaI = abs(cosThetaI);

    float sin2ThetaI = max(0.0, 1.0 - cosThetaI * cosThetaI);
    float sin2ThetaT = (etaI / etaT) * (etaI / etaT) * sin2ThetaI;

    if (sin2ThetaT >= 1.0)
        return 1.0;

    float cosThetaT = sqrt(max(0.0, 1.0 - sin2ThetaT));

    float rPar  = ((etaT * cosThetaI) - (etaI * cosThetaT))
                / ((etaT * cosThetaI) + (etaI * cosThetaT));
    float rPerp = ((etaI * cosThetaI) - (etaT * cosThetaT))
                / ((etaI * cosThetaI) + (etaT * cosThetaT));

    return 0.5 * (rPar * rPar + rPerp * rPerp);
}

float fresnelExt(float cosTheta, float etaI, float etaT)
{
    return fresnelDielectric(cosTheta, etaI, etaT);
}

vec3 plasticReflectLocal(vec3 wi)
{
    return vec3(-wi.x, -wi.y, wi.z);
}

bool isSpecularReflection(vec3 wi, vec3 wo)
{
    vec3 wr = plasticReflectLocal(wi);
    return dot(wr, wo) > 1.0 - 1e-4;
}

vec3 smooth_plastic_eval(Plastic mat, vec3 wi, vec3 wo)
{
    if (wi.z <= 0.0 || wo.z <= 0.0)
        return vec3(0.0);

    vec3 result = vec3(0.0);

    if (isSpecularReflection(wi, wo))
    {
        float F = fresnelExt(wi.z, mat.ext_ior, mat.int_ior);
        result += mat.specular_reflectance * (F / wi.z);
    }

    float Fi = fresnelExt(wi.z, mat.ext_ior, mat.int_ior);
    float Fo = fresnelExt(wo.z, mat.ext_ior, mat.int_ior);
    result += mat.diffuse_reflectance * (1.0 - Fi) * (1.0 - Fo) * (1.0 / PI);

    return result;
}

float smooth_plastic_pdf(Plastic mat, vec3 wi, vec3 wo)
{
    if (wi.z <= 0.0 || wo.z <= 0.0)
        return 0.0;

    float F = fresnelExt(wi.z, mat.ext_ior, mat.int_ior);
    float pdf = 0.0;

    if (isSpecularReflection(wi, wo))
        pdf += F;

    pdf += (1.0 - F) * wo.z * (1.0 / PI);

    return pdf;
}

BsdfSample smooth_plastic_sample(Plastic mat, vec3 wi, vec2 xi)
{
    BsdfSample s;
    s.wo  = vec3(0.0, 0.0, 1.0);
    s.f   = vec3(0.0);
    s.pdf = 0.0;

    if (wi.z <= 0.0)
        return s;

    float F = fresnelExt(wi.z, mat.ext_ior, mat.int_ior);

    if (xi.x < F)
    {
        s.wo  = plasticReflectLocal(wi);
        s.pdf = F;
        s.f   = mat.specular_reflectance * (F / wi.z);
    }
    else
    {
        vec2 xiDiff = vec2((xi.x - F) / max(1.0 - F, 1e-8), xi.y);
        s.wo  = cosineSampleHemisphere(xiDiff);

        float Fo = fresnelExt(s.wo.z, mat.ext_ior, mat.int_ior);
        s.f   = mat.diffuse_reflectance * (1.0 - F) * (1.0 - Fo) * (1.0 / PI);
        s.pdf = (1.0 - F) * s.wo.z * (1.0 / PI);
    }

    return s;
}
