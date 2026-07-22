#version 450

// Trail vertex shader - renders trail points in world space

struct TrailPoint {
    vec4 pos;    // xyz = position, w = age (0 = newest, 1 = oldest)
};

layout(std430, binding = 0) readonly buffer TrailBuffer {
    TrailPoint trails[];  // [particle0_point0, particle0_point1, ..., particle1_point0, ...]
};

layout(binding = 1) uniform TrailUBO {
    float camRotX;
    float camRotY;
    float camZoom;          // Zoom level (1.0 = normal, >1 = zoomed in)
    float camX, camY, camZ; // Camera position
    float screenWidth;
    float screenHeight;
    float pointScale;
    int trailLength;
    int numParticles;
    int trailHead;          // ring-buffer newest slot
};

layout(std430, binding = 2) readonly buffer TrailRowMap {
    int rowMap[];    // particle index -> trail buffer row
};

layout(location = 0) out float fragAge;
layout(location = 1) out vec3 fragColor;

void main() {
    int particleIdx = gl_VertexIndex / trailLength;
    int trailIdx = gl_VertexIndex % trailLength;

    TrailPoint tp = trails[rowMap[particleIdx] * trailLength + trailIdx];
    vec3 pos = tp.pos.xyz;

    // Age from the ring-buffer head: 0 = newest sample, 1 = oldest.
    // w < 0 marks slots never written since the last reset.
    int ageSteps = (trailHead - trailIdx + trailLength) % trailLength;
    float age = float(ageSteps) / float(max(trailLength - 1, 1));

    if (tp.pos.w < 0.0) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);  // Off-screen
        gl_PointSize = 0.0;
        fragAge = 1.0;
        fragColor = vec3(0.0);
        return;
    }

    // Subtract camera position to get view-relative position
    vec3 viewPos = pos - vec3(camX, camY, camZ);

    // Camera rotation (same as particle shader)
    float cx = cos(camRotX), sx = sin(camRotX);
    float cy = cos(camRotY), sy = sin(camRotY);

    vec3 rotated;
    rotated.x = viewPos.x * cy + viewPos.z * sy;
    rotated.y = viewPos.y;
    rotated.z = -viewPos.x * sy + viewPos.z * cy;

    vec3 final;
    final.x = rotated.x;
    final.y = rotated.y * cx - rotated.z * sx;
    final.z = rotated.y * sx + rotated.z * cx;

    // Perspective projection - proper perspective divide
    float nearPlane = 1.0;
    float farPlane = 20000.0;

    float viewZ = max(final.z, nearPlane);

    // Match particle shader projection (fixed 30° half-FOV with zoom)
    float tanHalfFov = 0.577;  // tan(30°) ≈ 0.577, gives 60° total FOV
    float aspectRatio = screenWidth / screenHeight;

    float projX = (final.x / viewZ) / (tanHalfFov * aspectRatio) * camZoom;
    float projY = (final.y / viewZ) / tanHalfFov * camZoom;
    float normalizedZ = clamp((viewZ - nearPlane) / (farPlane - nearPlane), 0.0, 1.0);

    if (final.z < nearPlane) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        gl_PointSize = 0.0;
    } else {
        gl_Position = vec4(projX, projY, normalizedZ, 1.0);
    }

    // Point size decreases with age and distance
    float distScale = 500.0 / viewZ * camZoom;
    float ageFade = 1.0 - age;
    gl_PointSize = clamp(distScale * ageFade * ageFade * 0.8, 0.5, 15.0);

    fragAge = age;

    // Color based on age - hot white/yellow to cool blue/purple
    vec3 young = vec3(1.0, 0.9, 0.7);   // Hot white-yellow
    vec3 mid = vec3(0.8, 0.4, 0.2);     // Orange
    vec3 old = vec3(0.3, 0.1, 0.5);     // Purple

    if (age < 0.5) {
        fragColor = mix(young, mid, age * 2.0);
    } else {
        fragColor = mix(mid, old, (age - 0.5) * 2.0);
    }
}
