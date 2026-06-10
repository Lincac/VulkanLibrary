// Mitsuba roughconductor：GGX 微表面 + 导体 Fresnel + Heitz 可见法线采样

float smithG1_GGX(float alpha, vec3 v, vec3 m)
{
    if (dot(v, m) * v.z <= 0.0)
        return 0.0;

    float tanTheta = length(vec2(v.x, v.y)) / max(v.z, 1e-6);
    if (tanTheta <= 0.0)
        return 1.0;

    float root = alpha * tanTheta;
    return 2.0 / (1.0 + sqrt(1.0 + root * root));
}

float G_GGX(float alpha, vec3 wi, vec3 wo, vec3 m)
{
    return smithG1_GGX(alpha, wi, m) * smithG1_GGX(alpha, wo, m);
}

// Heitz & D'Eon — GGX visible normal sampling (alpha = 1 case)
vec2 sampleVisible11_GGX(float thetaI, vec2 xi)
{
    if (thetaI < 1e-4) {
        float r = sqrt(xi.x / max(1.0 - xi.x, 1e-6));
        float phi = 2.0 * PI * xi.y;
        return vec2(r * cos(phi), r * sin(phi));
    }

    float tanThetaI = tan(thetaI);
    float a = 1.0 / tanThetaI;
    float G1 = 2.0 / (1.0 + sqrt(1.0 + 1.0 / (a * a)));

    float A = 2.0 * xi.x / G1 - 1.0;
    if (abs(A) >= 1.0 - 1e-6)
        A -= sign(A) * 1e-6;

    float tmp = 1.0 / (A * A - 1.0);
    float B = tanThetaI;
    float D = sqrt(max(0.0, B * B * tmp * tmp - (A * A - B * B) * tmp));
    float slope_x_1 = B * tmp - D;
    float slope_x_2 = B * tmp + D;
    float slope_x = (A < 0.0 || slope_x_2 > 1.0 / tanThetaI) ? slope_x_1 : slope_x_2;

    float S;
    float sy = xi.y;
    if (sy > 0.5) {
        S = 1.0;
        sy = 2.0 * (sy - 0.5);
    } else {
        S = -1.0;
        sy = 2.0 * (0.5 - sy);
    }

    float z =
        (sy * (sy * (sy * (-0.365728915865723) + 0.790235037209296) -
               0.424965825137544) + 0.000152998850436920) /
        (sy * (sy * (sy * (sy * 0.169507819808272 - 0.397203533833404) -
               0.232500544458471) + 1.0) - 0.539825872510702);

    float slope_y = S * z * sqrt(1.0 + slope_x * slope_x);
    return vec2(slope_x, slope_y);
}

vec3 sampleGGXVisibleNormal(vec3 wi, float alpha, vec2 xi)
{
    alpha = max(alpha, 1e-4);

    vec3 wiStretch = normalize(vec3(alpha * wi.x, alpha * wi.y, wi.z));

    float theta = 0.0;
    float phi = 0.0;
    if (wiStretch.z < 0.99999) {
        theta = acos(clamp(wiStretch.z, -1.0, 1.0));
        phi = atan(wiStretch.y, wiStretch.x);
    }

    vec2 slope = sampleVisible11_GGX(theta, xi);

    float cosPhi = cos(phi);
    float sinPhi = sin(phi);
    slope = vec2(
        cosPhi * slope.x - sinPhi * slope.y,
        sinPhi * slope.x + cosPhi * slope.y
    );

    slope *= alpha;

    float norm = 1.0 / sqrt(slope.x * slope.x + slope.y * slope.y + 1.0);
    return vec3(-slope.x * norm, -slope.y * norm, norm);
}

vec3 fresnelConductorRough(RoughConductor mat, float cosThetaI)
{
    Conductor c;
    c.eta = mat.eta;
    c.k   = mat.k;
    return fresnelConductor(c, cosThetaI);
}

vec3 rough_conductor_eval(RoughConductor mat, vec3 wi, vec3 wo)
{
    float NoI = wi.z;
    float NoO = wo.z;
    if (NoI <= 0.0 || NoO <= 0.0)
        return vec3(0.0);

    if (!sameHemisphere(wi, wo))
        return vec3(0.0);

    vec3 H = normalize(wi + wo);
    float NoH = max(0.0, H.z);
    float WiH = max(0.0, dot(wi, H));

    float alpha = max(mat.alpha, 1e-4);
    float D = D_GGX(alpha, NoH);
    if (D <= 0.0)
        return vec3(0.0);

    float G = G_GGX(alpha, wi, wo, H);
    vec3 F = fresnelConductorRough(mat, WiH);

  // Mitsuba: F * D * G / (4 * cosTheta(wi))
    float model = D * G / (4.0 * NoI);
    return F * model;
}

// 渲染方程被积函数 f(wi, wo) * cos(wo)
vec3 rough_conductor_eval_radiance(RoughConductor mat, vec3 wi, vec3 wo)
{
    if (wi.z <= 0.0 || wo.z <= 0.0)
        return vec3(0.0);

    return rough_conductor_eval(mat, wi, wo) * wo.z;
}

float rough_conductor_pdf(RoughConductor mat, vec3 wi, vec3 wo)
{
    float NoI = wi.z;
    float NoO = wo.z;
    if (NoI <= 0.0 || NoO <= 0.0)
        return 0.0;

    if (!sameHemisphere(wi, wo))
        return 0.0;

    vec3 H = normalize(wi + wo);
    float NoH = max(0.0, H.z);

    float alpha = max(mat.alpha, 1e-4);
    float D = D_GGX(alpha, NoH);
    if (D <= 0.0)
        return 0.0;

  // Mitsuba visible-normal pdf: D(H) * smithG1(wi, H) / (4 * cosTheta(wi))
    return D * smithG1_GGX(alpha, wi, H) / (4.0 * NoI);
}

BsdfSample rough_conductor_sample(RoughConductor mat, vec3 wi, vec2 xi)
{
    BsdfSample s;
    s.wo      = vec3(0.0, 0.0, 1.0);
    s.f       = vec3(0.0);
    s.pdf     = 0.0;
    s.isDelta = false;

    float NoI = wi.z;
    if (NoI <= 0.0)
        return s;

    float alpha = max(mat.alpha, 1e-4);
    vec3 m = sampleGGXVisibleNormal(wi, alpha, xi);
    if (m.z <= 0.0)
        return s;

    if (dot(wi, m) <= 0.0)
        return s;

    vec3 wo = reflectLocal(wi, m);
    if (wo.z <= 0.0)
        return s;

    s.wo = wo;
    s.f   = rough_conductor_eval(mat, wi, wo);
    s.pdf = rough_conductor_pdf(mat, wi, wo);
    s.isDelta = false;

    return s;
}
