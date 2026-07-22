#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;

layout(binding = 0) uniform sampler2D fontTexture;

layout(location = 0) out vec4 outColor;

void main() {
    // UV (0,0) signals a solid rectangle - use vertex color directly
    // This makes menu backgrounds fully opaque
    if (fragUV.x < 0.001 && fragUV.y < 0.001) {
        outColor = fragColor;
        return;
    }

    // Normal text rendering - use font texture alpha
    float alpha = texture(fontTexture, fragUV).r;
    if (alpha < 0.1) discard;
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
