#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in float fragSize;
layout(location = 2) in float fragDepth;

layout(binding = 1) uniform RenderUBO {
    float camRotX;
    float camRotY;
    float camZoom;
    float camX, camY, camZ;
    float screenWidth;
    float screenHeight;
    float glowIntensity;
    int colorMode;
    float pointScale;
    float lensingStrength;
    float speedOfLight;
};

layout(location = 0) out vec4 outColor;

void main() {
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord) * 2.0;

    // Discard outside the inscribed circle of the point sprite quad
    // Prevents any square-edge leakage into HDR buffer
    if (dist > 1.0) discard;

    // ============================================
    // STELLAR POINT PROFILE - core-dominant
    // A bright compact core resolves each particle
    // individually; a tighter soft aura still lets
    // dense regions bloom into nebulosity without
    // washing out into a single fused glow.
    // ============================================

    // Compact aura: much tighter than the old wide body
    float body = exp(-dist * dist * 6.0);
    // Dominant core: the star itself
    float core = exp(-dist * dist * 16.0);

    float envelope = core + body * 0.45;

    // Smooth circular fade to zero at dist=1.0
    // Guarantees perfectly circular output — no square artifacts
    float circularFade = 1.0 - dist * dist;
    envelope *= circularFade;

    // Pixel-art regime: sprites below a few pixels render as solid,
    // hard-edged points - crisp stars at native resolution instead of
    // dithery sub-pixel Gaussian dust
    float crisp = clamp((3.5 - fragSize) / 2.0, 0.0, 1.0);
    envelope = mix(envelope, 1.0, crisp);

    // ============================================
    // COLOR — hue-preserving brightening
    // ============================================

    vec3 color = fragColor;

    // Core gets a subtle temperature shift (slightly warmer/brighter)
    vec3 warm = color * vec3(1.1, 0.95, 0.9);
    float maxC = max(max(warm.r, warm.g), warm.b);
    warm = (maxC > 0.001) ? warm / maxC * length(color) : color;
    vec3 blended = mix(color, warm, core * 0.15);

    // Single smooth color output weighted by the glow envelope
    vec3 finalColor = blended * envelope;

    // Glow intensity control - boosted so individual stars stay vivid
    // with the tighter envelope
    finalColor *= glowIntensity * 5.5;

    // Gentle visibility boost for small particles (capped to avoid HDR hotspots)
    float sizeBoost = 1.0 + clamp((5.0 - fragSize) * 0.1, 0.0, 0.5);
    finalColor *= sizeBoost;

    // Distant particles: subtle blue atmospheric tint
    float depthFog = clamp(fragDepth * 0.002, 0.0, 0.15);
    finalColor = mix(finalColor, finalColor * vec3(0.88, 0.92, 1.08), depthFog);

    // ============================================
    // OUTPUT — additive blending
    // circularFade already baked into envelope
    // ============================================

    float alpha = envelope * 0.9;

    outColor = vec4(finalColor, alpha);
}
