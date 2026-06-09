// 由 CDF 纹理尺寸和 (i,j) 算立体角微元 dΩ
float envCellSolidAngle(ivec2 cdfRes, int i, int j)
{
    const float du = 1.0 / float(cdfRes.x);
    const float dv = 1.0 / float(cdfRes.y);
    const float v  = (float(j) + 0.5) / float(cdfRes.y);
    const float sinTheta = max(sin(v * PI), 1e-8);
    // dω = sin(θ) dθ dφ = 2π² sin(θ) du dv
    return 2.0 * PI * PI * sinTheta * du * dv;
}

// 从边缘 CDF（.g）用 xi.y 找行 j，返回 (j, pRow)
int sampleEnvRow(float xiY, sampler2D envCdf, ivec2 cdfRes, out float pRow)
{
    int lo = 0;
    int hi = cdfRes.y - 1;
    while (lo < hi)
    {
        const int mid = (lo + hi) >> 1;
        const float cdf = texelFetch(envCdf, ivec2(0, mid), 0).g;
        if (cdf < xiY)
            lo = mid + 1;
        else
            hi = mid;
    }
    const int j = lo;
    const float curr = texelFetch(envCdf, ivec2(0, j), 0).g;
    const float prev = (j > 0) ? texelFetch(envCdf, ivec2(0, j - 1), 0).g : 0.0;
    pRow = max(curr - prev, 0.0);
    return j;
}

// 在行 j 内用条件 CDF（.r）和 xi.x 找列 i，返回 (i, pColGivenRow)
int sampleEnvCol(float xiX, sampler2D envCdf, ivec2 cdfRes, int j, out float pColGivenRow)
{
    int lo = 0;
    int hi = cdfRes.x - 1;
    while (lo < hi)
    {
        const int mid = (lo + hi) >> 1;
        const float cdf = texelFetch(envCdf, ivec2(mid, j), 0).r;
        if (cdf < xiX)
            lo = mid + 1;
        else
            hi = mid;
    }
    const int i = lo;
    const float curr = texelFetch(envCdf, ivec2(i, j), 0).r;
    const float prev = (i > 0) ? texelFetch(envCdf, ivec2(i - 1, j), 0).r : 0.0;
    pColGivenRow = max(curr - prev, 0.0);
    return i;
}

// 由方向得到离散 cell (i,j) 及 uv 所在格
void directionToEnvCell(vec3 direction, ivec2 cdfRes, out int i, out int j, out vec2 uv)
{
    direction = normalize(direction);
    uv = directionToEnvUv(direction);

    i = clamp(int(floor(uv.x * float(cdfRes.x))), 0, cdfRes.x - 1);
    j = clamp(int(floor(uv.y * float(cdfRes.y))), 0, cdfRes.y - 1);
}

// 给定 cell (i,j) 算离散概率与 solid-angle pdf
float envCellPdf(int i, int j, sampler2D envCdf, ivec2 cdfRes)
{
    const float currRow = texelFetch(envCdf, ivec2(0, j), 0).g;
    const float prevRow = (j > 0) ? texelFetch(envCdf, ivec2(0, j - 1), 0).g : 0.0;
    const float pRow = max(currRow - prevRow, 0.0);
    const float currCol = texelFetch(envCdf, ivec2(i, j), 0).r;
    const float prevCol = (i > 0) ? texelFetch(envCdf, ivec2(i - 1, j), 0).r : 0.0;
    const float pColGivenRow = max(currCol - prevCol, 0.0);
    const float pCell = pRow * pColGivenRow;
    const float dOmega = envCellSolidAngle(cdfRes, i, j);
    return (dOmega > 0.0) ? (pCell / dOmega) : 0.0;
}

EnvSample sampleEnvironmentImportance(vec2 xi, sampler2D envMap, sampler2D envCdf)
{
    EnvSample result;
    result.direction = vec3(0.0, 1.0, 0.0);
    result.pdf = 0.0;

    const ivec2 cdfRes = textureSize(envCdf, 0);
    if (cdfRes.x <= 0 || cdfRes.y <= 0)
        return result;

    // 与 CDF 构建一致：marginal 用 xi.y，conditional 用 xi.x
    float pRow = 0.0;
    const int j = sampleEnvRow(xi.y, envCdf, cdfRes, pRow);

    float pColGivenRow = 0.0;
    const int i = sampleEnvCol(xi.x, envCdf, cdfRes, j, pColGivenRow);

    // 对应 CPU 里 (i+0.5)/W, (j+0.5)/H
    const vec2 uv = vec2(
        (float(i) + 0.5) / float(cdfRes.x),
        (float(j) + 0.5) / float(cdfRes.y)
    );

    result.direction = envUvToDirection(uv);

    const float pCell = pRow * pColGivenRow;
    const float dOmega = envCellSolidAngle(cdfRes, i, j);
    result.pdf = (dOmega > 0.0) ? (pCell / dOmega) : 0.0;

    return result;
}

float environmentPdf(vec3 direction, sampler2D envCdf)
{
    const ivec2 cdfRes = textureSize(envCdf, 0);
    if (cdfRes.x <= 0 || cdfRes.y <= 0)
        return 0.0;

    int i, j;
    vec2 uv;
    directionToEnvCell(direction, cdfRes, i, j, uv);

    return envCellPdf(i, j, envCdf, cdfRes);
}