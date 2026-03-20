#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 texCoords;

layout(set = 0, binding = 0) uniform sampler2D uOpaqueTex;

float luminance(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

vec3 applyFxaa(sampler2D tex, vec2 uv) {
    vec2 invTexSize = 1.0 / vec2(textureSize(tex, 0));

    vec3 rgbM  = texture(tex, uv).rgb;
    vec3 rgbNW = texture(tex, uv + vec2(-1.0, -1.0) * invTexSize).rgb;
    vec3 rgbNE = texture(tex, uv + vec2( 1.0, -1.0) * invTexSize).rgb;
    vec3 rgbSW = texture(tex, uv + vec2(-1.0,  1.0) * invTexSize).rgb;
    vec3 rgbSE = texture(tex, uv + vec2( 1.0,  1.0) * invTexSize).rgb;

    float lumaM  = luminance(rgbM);
    float lumaNW = luminance(rgbNW);
    float lumaNE = luminance(rgbNE);
    float lumaSW = luminance(rgbSW);
    float lumaSE = luminance(rgbSE);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    float lumaRange = lumaMax - lumaMin;

    // Skip very low-contrast areas to keep details crisp.
    if (lumaRange < max(0.0312, lumaMax * 0.125)) {
        return rgbM;
    }

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * 0.125), 1.0 / 128.0);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2(-8.0), vec2(8.0)) * invTexSize;

    vec3 rgbA = 0.5 * (
        texture(tex, uv + dir * (1.0 / 3.0 - 0.5)).rgb +
        texture(tex, uv + dir * (2.0 / 3.0 - 0.5)).rgb
    );

    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(tex, uv + dir * -0.5).rgb +
        texture(tex, uv + dir *  0.5).rgb
    );

    float lumaB = luminance(rgbB);
    if (lumaB < lumaMin || lumaB > lumaMax) {
        return rgbA;
    }
    return rgbB;
}

void main() {
    outColor = vec4(applyFxaa(uOpaqueTex, texCoords), 1.0);
}
