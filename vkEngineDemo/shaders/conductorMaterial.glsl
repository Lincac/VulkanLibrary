vec3 reflectLocalPerfect(vec3 wi) 
{
    // 法线 = (0,0,1)，完美镜面反射
    return vec3(-wi.x, -wi.y, wi.z);
}

vec3 fresnelConductor(Conductor mat, float cosThetaI) 
{
    float c = clamp(abs(cosThetaI), 0.0, 1.0);
    float c2 = c * c;
    vec3 eta2 = mat.eta * mat.eta;
    vec3 k2   = mat.k * mat.k;

    vec3 t0 = eta2 - k2 - vec3(c2);
    vec3 a2plusb2 = sqrt(t0 * t0 + 4.0 * eta2 * k2);

    vec3 t1 = a2plusb2 + vec3(c2);
    vec3 a  = sqrt(0.5 * (a2plusb2 + t0));
    vec3 t2 = 2.0 * a * c;

    vec3 Rs = (t1 - t2) / (t1 + t2);

    vec3 t3 = c2 * a2plusb2 + vec3(1.0);
    vec3 t4 = t2;
    vec3 Rp = Rs * (t3 - t4) / (t3 + t4);

    return 0.5 * (Rs + Rp);
}

vec3 smooth_conductor_eval(Conductor mat, vec3 wi, vec3 wo) 
{
    float NoI = wi.z;
    float NoO = wo.z;
    if (NoI <= 0.0 || NoO <= 0.0)
        return vec3(0.0);

    // 必须是完美镜面方向
    vec3 wr = reflectLocalPerfect(wi);
    if (dot(normalize(wr), normalize(wo)) < 1.0 - 1e-4)
        return vec3(0.0);

    // Fresnel at incident direction
    vec3 F = fresnelConductor(mat, NoI);

    // delta BRDF 通常写成：F / |cosThetaI|
    return F / max(abs(NoI), 1e-6);
}


float smooth_conductor_pdf(Conductor mat, vec3 wi, vec3 wo)
{
    float NoI = wi.z;
    float NoO = wo.z;
    if (NoI <= 0.0 || NoO <= 0.0)
        return 0.0;

    vec3 wr = reflectLocalPerfect(wi);
    if (dot(normalize(wr), normalize(wo)) < 1.0 - 1e-4)
        return 0.0;

    // 纯 delta：pdf = 1（在 mixture 里再乘权重）
    return 1.0;
}

BsdfSample smooth_conductor_sample(Conductor mat, vec3 wi, vec2 xi) 
{
    BsdfSample s;
    s.wo      = vec3(0.0, 0.0, 1.0);
    s.f       = vec3(0.0);
    s.pdf     = 0.0;
    s.isDelta = true;

    float NoI = wi.z;
    if (NoI <= 0.0)
        return s;

    vec3 wo = reflectLocalPerfect(wi);
    float NoO = wo.z;
    if (NoO <= 0.0)
        return s;

    s.wo = wo;

    vec3 F = fresnelConductor(mat, NoI);

    // 和 eval 保持一致：f = F / |cosI|
    s.f   = F / max(abs(NoI), 1e-6);
    s.pdf = 1.0;      // 纯 delta，pdf=1
    s.isDelta = true; // 告诉 integrator：这是完美镜面

    return s;
}