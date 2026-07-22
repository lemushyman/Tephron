#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D hdrImage;

layout(binding = 1) uniform BHUBO {
    vec4 bh[8];      // xy = uv center, z = Einstein radius (v units), w = shadow radius
    ivec4 bhCount;   // x = number of active black holes
};

layout(push_constant) uniform Push {
    float camRotX;      // camera pitch
    float camRotY;      // camera yaw
    float camZoom;      // projection zoom factor
    float aspect;       // width / height
    float time;         // seconds, for star twinkle
    float starsEnabled; // > 0.5 = draw the deep-sky background
    float lowFx;        // > 0.5 = skip the bloom stack (weak GPUs)
    float pad0;
} pc;

// Khronos PBR Neutral Tone Mapping
vec3 PBRNeutralToneMapping(vec3 color) {
    const float startCompression = 0.8 - 0.04;
    const float desaturation = 0.15;
    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;
    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression) return color;
    const float d = 1.0 - startCompression;
    float newPeak = 1.0 - d * d / (peak + d - startCompression);
    color *= newPeak / peak;
    float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
    return mix(color, newPeak * vec3(1.0), g);
}

// Circular ring bloom: 8 samples evenly spaced around a circle
// Golden-angle offset per layer prevents alignment with rectangular grid
vec3 ringBloom(vec2 uv, float lod, float radius, float angleOffset) {
    vec2 texelSize = 1.0 / vec2(textureSize(hdrImage, 0));
    vec3 center = textureLod(hdrImage, uv, lod).rgb;
    vec3 ring = vec3(0.0);
    const int SAMPLES = 8;
    for (int i = 0; i < SAMPLES; i++) {
        float angle = angleOffset + float(i) * (6.2831853 / float(SAMPLES));
        vec2 off = vec2(cos(angle), sin(angle)) * radius * texelSize;
        ring += textureLod(hdrImage, uv + off, lod).rgb;
    }
    ring /= float(SAMPLES);
    // Blend center energy with ring shape
    return mix(center, ring, 0.6);
}

// Simple hash for film grain
float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

// ============================================================
// PROCEDURAL DEEP-SKY BACKGROUND
//
// Stars live on the sphere at infinity: the view ray direction
// for each pixel is hashed into 3D cells (direction scaled by a
// layer constant), each cell rolling for a star. Rotating the
// camera pans across a fixed sky; translating the camera (WASD)
// leaves it unchanged, exactly like a real starfield.
// ============================================================

float hash13(vec3 p) {
    p = fract(p * 0.1031);
    p += dot(p, p.zyx + 31.32);
    return fract((p.x + p.y) * p.z);
}

vec3 hash33(vec3 p) {
    p = fract(p * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return fract((p.xxy + p.yxx) * p.zyx);
}

// One star layer: scale direction into a 3D lattice, one candidate
// star per cell. density in (0,1] is the fraction of cells occupied.
vec3 starLayer(vec3 dir, float cellScale, float density, float sharpness, float brightness) {
    vec3 p = dir * cellScale;
    vec3 cell = floor(p);
    float roll = hash13(cell + 17.0);
    if (roll > density) return vec3(0.0);

    vec3 h = hash33(cell);
    vec3 starPos = cell + 0.25 + 0.5 * h;   // keep star well inside its cell
    float d = length(p - starPos);
    float core = exp(-d * d * sharpness);

    // Stellar color temperature: blue-white / white / warm / red
    float temp = hash13(cell + 71.0);
    vec3 col = (temp < 0.4) ? mix(vec3(0.75, 0.82, 1.0), vec3(1.0, 1.0, 1.0), temp * 2.5)
             : (temp < 0.8) ? mix(vec3(1.0, 1.0, 1.0), vec3(1.0, 0.85, 0.6), (temp - 0.4) * 2.5)
                            : mix(vec3(1.0, 0.85, 0.6), vec3(1.0, 0.55, 0.4), (temp - 0.8) * 5.0);

    // Magnitude distribution: most stars faint, a few bright
    float mag = pow(hash13(cell + 137.0), 4.0);
    // Gentle twinkle
    float tw = 0.8 + 0.2 * sin(pc.time * (1.5 + 3.0 * h.x) + h.y * 40.0);

    return col * core * (0.15 + mag) * brightness * tw;
}

vec3 deepSky(vec2 uv) {
    // Reconstruct the world-space view ray for this pixel
    // (inverse of the particle vertex shader's projection + rotation)
    vec2 ndc = uv * 2.0 - 1.0;
    float tanHalfFov = 0.577;
    vec3 dv = normalize(vec3(ndc.x * pc.aspect * tanHalfFov / pc.camZoom,
                             ndc.y * tanHalfFov / pc.camZoom,
                             1.0));
    float cx = cos(-pc.camRotX), sx = sin(-pc.camRotX);
    float cy = cos(-pc.camRotY), sy = sin(-pc.camRotY);
    vec3 r1 = vec3(dv.x, cx * dv.y - sx * dv.z, sx * dv.y + cx * dv.z);
    vec3 dir = vec3(cy * r1.x + sy * r1.z, r1.y, -sy * r1.x + cy * r1.z);

    // Galactic band: a great circle tilted across the sky
    vec3 bandNormal = normalize(vec3(0.25, 1.0, 0.4));
    float bandDist = dot(dir, bandNormal);
    float band = exp(-bandDist * bandDist * 12.0);

    vec3 col = vec3(0.0);
    // Dense faint layer (density boosted inside the galactic band)
    col += starLayer(dir, 160.0, 0.10 + 0.20 * band, 180.0, 0.9);
    // Sparse bright layer
    col += starLayer(dir, 70.0, 0.06, 300.0, 2.2);

    // Faint milky-way glow with large-scale mottling
    float mottle = 0.6 + 0.4 * hash13(floor(dir * 9.0));
    col += band * mottle * vec3(0.020, 0.024, 0.035);

    // Barely-there deep space color so black isn't dead flat
    col += vec3(0.0025, 0.0030, 0.0050);

    return col;
}

void main() {
    // ============================================================
    // BLACK HOLES: gravitational lensing warp + shadow + photon ring
    //
    // Each black hole is a point-mass lens: a ray at angular
    // distance theta from the hole samples the background at
    // beta = theta - thetaE^2/theta. Inside the Einstein radius
    // beta goes negative - the background appears inverted, which
    // is exactly the real Einstein-ring geometry. The shadow disc
    // swallows everything inside ~the photon sphere, ringed by hot
    // lensed light.
    // ============================================================
    // No painted glow, no decorative ring: a black hole here is nothing but
    // OPTICS. The point-mass lens equation warps every sample of real scene
    // light (disk particles, trails, starfield); magnification diverges as
    // beta -> 0, so the bright photon ring EMERGES from whatever true light
    // sits behind the hole. Rays inside the critical impact parameter are
    // captured - that, and only that, is the shadow.
    vec2 uv = fragUV;
    float shadow = 1.0;
    for (int i = 0; i < bhCount.x && i < 8; i++) {
        vec2 c = bh[i].xy;
        vec2 dv = uv - c;
        dv.x *= pc.aspect;              // aspect-true angular units
        float r = length(dv) + 1e-5;
        float thetaE = bh[i].z;
        float rS = bh[i].w;

        float beta = r - (thetaE * thetaE) / r;
        // Finite-resolution floor: at the critical curve beta -> 0 and a
        // whole circle of screen pixels would sample the SAME texel at the
        // hole's center - one bright particle behind the center then strobes
        // as a pixel-thin ring. A real lens conserves surface brightness of
        // an infinitesimal source; a texture sampler photocopies it. Clamp
        // the source offset to ~a texel so only extended sources (disk,
        // starfield) form rings - stable ones.
        float betaMin = 1.5 / float(textureSize(hdrImage, 0).y);
        if (abs(beta) < betaMin) beta = (beta >= 0.0) ? betaMin : -betaMin;
        uv = c + (dv / r) * beta / vec2(pc.aspect, 1.0);

        // Ray capture: pixel-tight antialiased edge, nothing painted
        float edge = max(rS * 0.03, 0.0012);
        shadow *= smoothstep(rS - edge, rS + edge, r);
    }

    // Sharp detail: single tap at full resolution (through the lens warp)
    vec3 mip0 = textureLod(hdrImage, uv, 0.0).rgb;

    // Weak GPUs skip the whole bloom stack (the HDR image has 1 mip there)
    if (pc.lowFx > 0.5) {
        vec3 hdrLow = mip0;
        if (pc.starsEnabled > 0.5) {
            float fgLumL = dot(hdrLow, vec3(0.2126, 0.7152, 0.0722));
            hdrLow += deepSky(uv) / (1.0 + fgLumL * 4.0);
        }
        hdrLow *= shadow;
        vec3 mappedLow = PBRNeutralToneMapping(hdrLow);
        float lumL = dot(mappedLow, vec3(0.2126, 0.7152, 0.0722));
        mappedLow = mix(vec3(lumL), mappedLow, 1.4);
        outColor = vec4(mappedLow, 1.0);
        return;
    }

    // Circular ring bloom layers with golden-angle rotation (2.39996 rad ~ 137.5 deg)
    // Radii are small — mip levels already provide spatial spread (mip N = 2^N pixel area)
    // Warm-to-cool color tinting per layer
    vec3 bloom1 = ringBloom(uv, 1.0, 1.5, 1.0 * 2.39996) * vec3(1.04, 1.00, 0.96);
    vec3 bloom2 = ringBloom(uv, 2.0, 2.0, 2.0 * 2.39996) * vec3(1.02, 1.00, 0.98);
    vec3 bloom3 = ringBloom(uv, 3.0, 3.0, 3.0 * 2.39996) * vec3(1.00, 1.00, 1.00);
    vec3 bloom4 = ringBloom(uv, 4.0, 4.0, 4.0 * 2.39996) * vec3(0.97, 0.98, 1.03);
    vec3 bloom5 = ringBloom(uv, 5.0, 5.0, 5.0 * 2.39996) * vec3(0.94, 0.97, 1.06);

    // Weighted combination: sharp base dominates so individual particles
    // stay resolved; bloom layers add halo without fusing neighbors
    vec3 hdrColor = mip0   * 0.74
                  + bloom1 * 0.10
                  + bloom2 * 0.065
                  + bloom3 * 0.045
                  + bloom4 * 0.025
                  + bloom5 * 0.015;

    // Deep-sky background, suppressed where the foreground is bright
    // (sampled through the lens warp, so stars smear around black holes)
    if (pc.starsEnabled > 0.5) {
        float fgLum = dot(hdrColor, vec3(0.2126, 0.7152, 0.0722));
        hdrColor += deepSky(uv) / (1.0 + fgLum * 4.0);
    }

    // Captured rays carry no light
    hdrColor *= shadow;

    // Tone mapping
    vec3 mapped = PBRNeutralToneMapping(hdrColor);

    // Saturation recovery (tone mapping desaturates)
    float lum = dot(mapped, vec3(0.2126, 0.7152, 0.0722));
    mapped = mix(vec3(lum), mapped, 1.4);

    // Cinematic vignette: subtle radial darkening at screen edges
    vec2 centered = fragUV - 0.5;
    float vignette = 1.0 - 0.25 * dot(centered, centered);
    mapped *= vignette;

    // Film grain: ultra-fine noise to break up banding in dark gradients
    vec2 screenPos = fragUV * vec2(textureSize(hdrImage, 0));
    float grain = (hash(screenPos) - 0.5) * 0.015;
    mapped += grain;

    outColor = vec4(mapped, 1.0);
}
