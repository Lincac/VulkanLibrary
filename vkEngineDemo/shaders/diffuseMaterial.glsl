#include "traceGlobal.glsl"

vec3 cosineSampleHemisphere(vec2 xi) 
{
    float r = sqrt(xi.x);
    float phi = 2.0 * PI * xi.y;
    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(max(0.0, 1.0 - x*x - y*y));
    return vec3(x, y, z);
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