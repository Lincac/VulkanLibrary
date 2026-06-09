vec3 cosineSampleHemisphere(vec2 xi) 
{
    float r = sqrt(xi.x);
    float phi = 2.0 * PI * xi.y;
    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(max(0.0, 1.0 - x*x - y*y));
    return vec3(x, y, z);
}

vec3 diffuse_eval(Diffuse mat, vec3 wi, vec3 wo)
{
    if (wi.z <= 0.0 || wo.z <= 0.0)
        return vec3(0.0);

    float sigma2 = mat.alpha * mat.alpha;

    float sinTi = sqrt(max(0.0, 1.0 - wi.z * wi.z));
    float sinTo = sqrt(max(0.0, 1.0 - wo.z * wo.z));

    float cosAlpha = 1.0;
    float sinAlpha = 0.0;
    if (sinTi > 1e-4 && sinTo > 1e-4)
    {
        vec2 wi_t = vec2(wi.x, wi.y) / sinTi;
        vec2 wo_t = vec2(wo.x, wo.y) / sinTo;
        cosAlpha = clamp(dot(wi_t, wo_t), -1.0, 1.0);
        sinAlpha = sinTi * sinTo;
    }

    float A = 1.0 - 0.5 * sigma2 / (sigma2 + 0.33);
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    float on = A + B * max(0.0, cosAlpha) * sinAlpha;
    return mat.diffuse_reflectance * on * (1.0 / PI);
}

float diffuse_pdf(Diffuse mat, vec3 wi, vec3 wo)
{
    if (wi.z <= 0.0 || wo.z <= 0.0)
        return 0.0;
    return wo.z * (1.0 / PI);
}

BsdfSample diffuse_sample(Diffuse mat, vec3 wi, vec2 xi)
{
    BsdfSample s;
    if (wi.z <= 0.0) {
        s.pdf = 0.0;
        s.f   = vec3(0.0);
        s.wo  = vec3(0.0, 0.0, 1.0);
        return s;
    }

    s.wo  = cosineSampleHemisphere(xi);
    s.pdf = diffuse_pdf(mat, wi, s.wo);
    s.f   = diffuse_eval(mat, wi, s.wo);

    return s;
}
