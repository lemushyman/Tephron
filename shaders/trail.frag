#version 450

layout(location = 0) in float fragAge;
layout(location = 1) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    // Circular point with soft edges
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord) * 2.0;

    if (dist > 1.0) discard;

    // Soft glow falloff
    float core = exp(-dist * dist * 4.0);
    float glow = exp(-dist * dist * 1.5);
    float intensity = core * 0.7 + glow * 0.3;

    // Fade with age
    float ageFade = (1.0 - fragAge);
    ageFade = ageFade * ageFade;  // Quadratic falloff

    float alpha = intensity * ageFade * 0.8;

    outColor = vec4(fragColor * intensity, alpha);
}
