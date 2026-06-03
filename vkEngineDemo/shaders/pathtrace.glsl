// 路径追踪公用函数（由 raygen / closesthit / miss include）

const uint MAX_BOUNCES = 10u;
const uint SAMPLES_PER_PIXEL = 8u;
const float PI = 3.14159265358979323846;

struct PathPayload {
    vec4 hitNormal; // x: 1=命中, yzw: 世界空间法线
    vec4 position;  // xyz: 命中点, w 未用
    vec4 albedo;    // xyz: 漫反射反照率
};

uint pcgHash(inout uint state)
{
    state = state * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float rand01(inout uint seed)
{
    return float(pcgHash(seed)) / 4294967296.0;
}

vec3 sampleCosineHemisphere(vec3 normal, inout uint seed)
{
    const float r1 = rand01(seed);
    const float r2 = rand01(seed);
    const float phi = 2.0 * PI * r1;
    const float cosTheta = sqrt(1.0 - r2);
    const float sinTheta = sqrt(r2);

    vec3 tangent = abs(normal.z) < 0.999 ? normalize(cross(normal, vec3(0.0, 0.0, 1.0)))
                                         : normalize(cross(normal, vec3(0.0, 1.0, 0.0)));
    vec3 bitangent = cross(normal, tangent);

    const vec3 local = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    return normalize(tangent * local.x + bitangent * local.y + normal * local.z);
}

vec3 sampleEnvironment(sampler2D envMap, vec3 direction)
{
    direction = normalize(direction);
    const float phi = atan(direction.z, direction.x);
    const float theta = acos(clamp(direction.y, -1.0, 1.0));
    const vec2 uv = vec2(phi * (0.5 / PI) + 0.5, theta / PI);
    return texture(envMap, uv).rgb;
}

vec3 toneMap(vec3 color)
{
    color = color / (color + vec3(1.0));
    return pow(color, vec3(1.0 / 2.2));
}
