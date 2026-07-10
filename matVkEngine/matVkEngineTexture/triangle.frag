#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D u_image;

void main() {
    outColor = texture(u_image, texCoord);
}