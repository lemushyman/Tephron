#version 450

struct Particle {
    vec4 posM;   // position.xyz + mass
    vec4 vel;    // velocity.xyz + age
    vec4 color;  // rgb + density
};

layout(std430, binding = 0) readonly buffer Particles {
    Particle particles[];
};

layout(binding = 1) uniform RenderUBO {
    float camRotX;
    float camRotY;
    float camZoom;          // Zoom level (1.0 = normal, >1 = zoomed in)
    float camX, camY, camZ; // Camera position
    float screenWidth;
    float screenHeight;
    float glowIntensity;
    int colorMode;
    float pointScale;
    float lensingStrength;  // Gravitational lensing intensity
    float speedOfLight;     // For relativistic visual effects
    float simTime;          // For pulsar beacon strobing
};

layout(location = 0) out vec3 fragColor;
layout(location = 1) out float fragSize;
layout(location = 2) out float fragDepth;

// ============================================
// COLOR PALETTE FUNCTIONS
// ============================================

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Attempt to convert RGB back to HSV for manipulation
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// Plasma/nebula palette
vec3 plasmaPalette(float t) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.263, 0.416, 0.557);
    return a + b * cos(6.28318 * (c * t + d));
}

// Fire/stellar palette
vec3 firePalette(float t) {
    t = clamp(t, 0.0, 1.0);
    if (t < 0.25) return mix(vec3(0.1, 0.0, 0.0), vec3(0.5, 0.0, 0.0), t * 4.0);
    if (t < 0.5) return mix(vec3(0.5, 0.0, 0.0), vec3(1.0, 0.3, 0.0), (t - 0.25) * 4.0);
    if (t < 0.75) return mix(vec3(1.0, 0.3, 0.0), vec3(1.0, 0.8, 0.2), (t - 0.5) * 4.0);
    return mix(vec3(1.0, 0.8, 0.2), vec3(1.0, 1.0, 0.9), (t - 0.75) * 4.0);
}

// Cosmic/galaxy palette
vec3 cosmicPalette(float t) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 0.7, 0.4);
    vec3 d = vec3(0.0, 0.15, 0.20);
    return a + b * cos(6.28318 * (c * t + d));
}

// Ocean/deep sea palette
vec3 oceanPalette(float t) {
    vec3 deep = vec3(0.0, 0.05, 0.2);
    vec3 mid = vec3(0.0, 0.3, 0.5);
    vec3 shallow = vec3(0.1, 0.6, 0.8);
    vec3 foam = vec3(0.7, 0.9, 1.0);
    if (t < 0.33) return mix(deep, mid, t * 3.0);
    if (t < 0.66) return mix(mid, shallow, (t - 0.33) * 3.0);
    return mix(shallow, foam, (t - 0.66) * 3.0);
}

// Sunset palette
vec3 sunsetPalette(float t) {
    vec3 night = vec3(0.1, 0.0, 0.2);
    vec3 purple = vec3(0.4, 0.1, 0.4);
    vec3 red = vec3(0.9, 0.2, 0.1);
    vec3 orange = vec3(1.0, 0.5, 0.1);
    vec3 gold = vec3(1.0, 0.85, 0.3);
    if (t < 0.25) return mix(night, purple, t * 4.0);
    if (t < 0.5) return mix(purple, red, (t - 0.25) * 4.0);
    if (t < 0.75) return mix(red, orange, (t - 0.5) * 4.0);
    return mix(orange, gold, (t - 0.75) * 4.0);
}

// Neon/cyberpunk palette
vec3 neonPalette(float t) {
    float tt = fract(t * 3.0);
    vec3 pink = vec3(1.0, 0.0, 0.5);
    vec3 cyan = vec3(0.0, 1.0, 1.0);
    vec3 purple = vec3(0.5, 0.0, 1.0);
    int section = int(t * 3.0) % 3;
    if (section == 0) return mix(pink, cyan, tt);
    if (section == 1) return mix(cyan, purple, tt);
    return mix(purple, pink, tt);
}

// Ice/frost palette
vec3 icePalette(float t) {
    vec3 darkBlue = vec3(0.1, 0.15, 0.3);
    vec3 ice = vec3(0.5, 0.7, 0.9);
    vec3 frost = vec3(0.8, 0.9, 1.0);
    vec3 white = vec3(1.0, 1.0, 1.0);
    if (t < 0.33) return mix(darkBlue, ice, t * 3.0);
    if (t < 0.66) return mix(ice, frost, (t - 0.33) * 3.0);
    return mix(frost, white, (t - 0.66) * 3.0);
}

// Toxic/radioactive palette
vec3 toxicPalette(float t) {
    vec3 dark = vec3(0.05, 0.1, 0.0);
    vec3 green = vec3(0.2, 0.8, 0.1);
    vec3 lime = vec3(0.6, 1.0, 0.2);
    vec3 glow = vec3(0.8, 1.0, 0.4);
    if (t < 0.33) return mix(dark, green, t * 3.0);
    if (t < 0.66) return mix(green, lime, (t - 0.33) * 3.0);
    return mix(lime, glow, (t - 0.66) * 3.0);
}

// Gold/treasure palette
vec3 goldPalette(float t) {
    vec3 bronze = vec3(0.4, 0.25, 0.1);
    vec3 gold = vec3(0.85, 0.65, 0.2);
    vec3 bright = vec3(1.0, 0.85, 0.4);
    vec3 white = vec3(1.0, 0.95, 0.8);
    if (t < 0.33) return mix(bronze, gold, t * 3.0);
    if (t < 0.66) return mix(gold, bright, (t - 0.33) * 3.0);
    return mix(bright, white, (t - 0.66) * 3.0);
}

// Electric/storm palette
vec3 electricPalette(float t) {
    vec3 dark = vec3(0.05, 0.0, 0.15);
    vec3 purple = vec3(0.3, 0.1, 0.6);
    vec3 electric = vec3(0.4, 0.6, 1.0);
    vec3 white = vec3(0.9, 0.95, 1.0);
    if (t < 0.4) return mix(dark, purple, t * 2.5);
    if (t < 0.7) return mix(purple, electric, (t - 0.4) * 3.33);
    return mix(electric, white, (t - 0.7) * 3.33);
}

// Lava/magma palette
vec3 lavaPalette(float t) {
    vec3 black = vec3(0.1, 0.05, 0.0);
    vec3 darkRed = vec3(0.4, 0.0, 0.0);
    vec3 red = vec3(0.8, 0.1, 0.0);
    vec3 orange = vec3(1.0, 0.4, 0.0);
    vec3 yellow = vec3(1.0, 0.9, 0.3);
    if (t < 0.25) return mix(black, darkRed, t * 4.0);
    if (t < 0.5) return mix(darkRed, red, (t - 0.25) * 4.0);
    if (t < 0.75) return mix(red, orange, (t - 0.5) * 4.0);
    return mix(orange, yellow, (t - 0.75) * 4.0);
}

// Galaxy spiral palette
vec3 galaxyPalette(float t, float angle) {
    // Spiral arm colors
    float armPhase = fract(angle / 6.28318 + t * 0.5);
    vec3 arm = mix(vec3(0.8, 0.6, 1.0), vec3(0.4, 0.7, 1.0), armPhase);
    vec3 core = vec3(1.0, 0.9, 0.7);
    vec3 dust = vec3(0.3, 0.2, 0.4);
    return mix(dust, mix(arm, core, t * t), t);
}

// Particle type constants
const int PTYPE_GENERIC = 0;
const int PTYPE_ELECTRON = 1;
const int PTYPE_PROTON = 2;
const int PTYPE_NEUTRON = 3;
const int PTYPE_UP_QUARK = 4;
const int PTYPE_DOWN_QUARK = 5;
const int PTYPE_POSITRON = 6;

void main() {
    Particle p = particles[gl_VertexIndex];

    vec3 pos = p.posM.xyz;
    float mass = p.posM.w;
    vec3 vel = p.vel.xyz;
    float charge = p.vel.w;
    int ptype = int(p.color.w);  // Particle type

    // Consumed by a black hole: cull entirely
    if (mass <= 0.0) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        gl_PointSize = 0.0;
        fragColor = vec3(0.0);
        fragSize = 0.0;
        fragDepth = 0.0;
        return;
    }

    // Subtract camera position to get view-relative position
    vec3 viewPos = pos - vec3(camX, camY, camZ);

    // Camera rotation
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

    // Perspective projection with fixed FOV and zoom multiplier
    float nearPlane = 1.0;
    float farPlane = 20000.0;

    // Depth for clipping (must be positive for things in front of camera)
    float viewZ = max(final.z, nearPlane);

    // Fixed 30-degree half-FOV for natural view with less distortion
    float tanHalfFov = 0.577;  // tan(30°) ≈ 0.577, gives 60° total FOV
    float aspectRatio = screenWidth / screenHeight;

    // Standard perspective projection with zoom applied
    // camZoom > 1 = zoomed in (things appear bigger)
    // camZoom = 1 = normal view
    float projX = (final.x / viewZ) / (tanHalfFov * aspectRatio) * camZoom;
    float projY = (final.y / viewZ) / tanHalfFov * camZoom;

    // Gravitational lensing effect
    // Light bends around massive objects - calculate deflection from nearby masses
    if (lensingStrength > 0.01) {
        vec2 lensDeflection = vec2(0.0);

        // Sample nearby particles for lensing (simplified - sample a few heavy ones)
        // In reality we'd need all mass distribution, but this gives a visual approximation
        int numSamples = min(100, int(particles.length()));
        int stride = max(1, int(particles.length()) / numSamples);

        for (int s = 0; s < numSamples; s++) {
            int j = s * stride;
            if (j == gl_VertexIndex) continue;

            vec3 lensPos = particles[j].posM.xyz;
            float lensMass = particles[j].posM.w;

            // Transform lens position to view space
            vec3 lensView = lensPos - vec3(camX, camY, camZ);
            vec3 lensRotated;
            lensRotated.x = lensView.x * cy + lensView.z * sy;
            lensRotated.y = lensView.y;
            lensRotated.z = -lensView.x * sy + lensView.z * cy;
            vec3 lensFinal;
            lensFinal.x = lensRotated.x;
            lensFinal.y = lensRotated.y * cx - lensRotated.z * sx;
            lensFinal.z = lensRotated.y * sx + lensRotated.z * cx;

            // Only lens from objects between us and the particle, or nearby
            if (lensFinal.z > 0.0 && lensFinal.z < viewZ + 100.0) {
                // Project lens position
                float lensZ = max(lensFinal.z, nearPlane);
                float lensProjX = (lensFinal.x / lensZ) / (tanHalfFov * aspectRatio) * camZoom;
                float lensProjY = (lensFinal.y / lensZ) / tanHalfFov * camZoom;

                // Vector from lens to this particle in screen space
                vec2 toParticle = vec2(projX - lensProjX, projY - lensProjY);
                float screenDist = length(toParticle) + 0.001;

                // Einstein ring radius (simplified): theta_E ~ sqrt(4GM/c²/D)
                // Deflection angle ~ 4GM/(c²*b) where b is impact parameter
                // Divide out camZoom so impact parameter is in physical view-space units
                float impactParam = screenDist / camZoom * viewZ;
                float schwarzschildRadius = 2.0 * lensMass / (speedOfLight * speedOfLight) * 0.001;
                float deflection = schwarzschildRadius / (impactParam + 1.0) * lensingStrength;

                // Deflect away from the lens (light bends toward mass, image shifts away)
                lensDeflection += normalize(toParticle) * deflection * 50.0;
            }
        }

        // Apply accumulated lensing deflection (re-apply camZoom for zoom-invariant effect)
        projX += clamp(lensDeflection.x * camZoom, -0.5, 0.5);
        projY += clamp(lensDeflection.y * camZoom, -0.5, 0.5);
    }

    // Normalized depth for z-buffer
    float normalizedZ = clamp((viewZ - nearPlane) / (farPlane - nearPlane), 0.0, 1.0);

    // Cull particles behind camera
    if (final.z < nearPlane) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);  // Off screen
        gl_PointSize = 0.0;
    } else {
        gl_Position = vec4(projX, projY, normalizedZ, 1.0);
    }

    // Point size - scales with distance and zoom. Kept modest so dense
    // regions read as many resolved stars, not one fused glow.
    float massSize = pow(mass, 0.35) * 3.2;
    float speed = length(vel);
    float speedBoost = clamp(speed * 0.015, 0.0, 2.0);
    float distScale = 500.0 / viewZ * camZoom;  // Scale with distance and zoom
    float baseSize = (massSize + speedBoost) * distScale * 0.8;
    gl_PointSize = clamp(baseSize, 1.0, 150.0);

    // Normalized values for color calculation
    float dist = length(pos);
    float normDist = clamp(dist / 300.0, 0.0, 1.0);
    float normSpeed = clamp(speed / 50.0, 0.0, 1.0);
    float normMass = clamp(sqrt(mass) / 10.0, 0.0, 1.0);
    float angle = atan(pos.y, pos.x);
    float vertAngle = atan(pos.z, length(pos.xy));

    // ============================================
    // PHYSICS-FOCUSED COLOR MODES
    // ============================================
    switch (colorMode) {
        case 0: // Velocity - Speed-based coloring
            {
                vec3 slow = vec3(0.1, 0.0, 0.3);
                vec3 mid = vec3(0.0, 0.4, 1.0);
                vec3 fast = vec3(1.0, 1.0, 1.0);
                fragColor = mix(slow, mid, smoothstep(0.0, 0.4, normSpeed));
                fragColor = mix(fragColor, fast, smoothstep(0.4, 1.0, normSpeed));
            }
            break;

        case 1: // Mass - Stellar classification (red dwarf to blue giant)
            {
                if (normMass < 0.2) fragColor = mix(vec3(0.5, 0.1, 0.1), vec3(0.9, 0.2, 0.1), normMass * 5.0);
                else if (normMass < 0.4) fragColor = mix(vec3(0.9, 0.2, 0.1), vec3(1.0, 0.6, 0.2), (normMass - 0.2) * 5.0);
                else if (normMass < 0.6) fragColor = mix(vec3(1.0, 0.6, 0.2), vec3(1.0, 1.0, 0.8), (normMass - 0.4) * 5.0);
                else if (normMass < 0.8) fragColor = mix(vec3(1.0, 1.0, 0.8), vec3(0.7, 0.85, 1.0), (normMass - 0.6) * 5.0);
                else fragColor = mix(vec3(0.7, 0.85, 1.0), vec3(0.5, 0.6, 1.0), (normMass - 0.8) * 5.0);
            }
            break;

        case 2: // Particle Type - distinct colors for each particle type
            {
                if (ptype == PTYPE_ELECTRON) {
                    fragColor = vec3(0.2, 0.5, 1.0);    // Blue - electrons
                } else if (ptype == PTYPE_PROTON) {
                    fragColor = vec3(1.0, 0.3, 0.2);    // Red - protons
                } else if (ptype == PTYPE_NEUTRON) {
                    fragColor = vec3(0.6, 0.6, 0.6);    // Gray - neutrons
                } else if (ptype == PTYPE_UP_QUARK) {
                    fragColor = vec3(1.0, 0.2, 0.6);    // Magenta - up quarks
                } else if (ptype == PTYPE_DOWN_QUARK) {
                    fragColor = vec3(0.2, 1.0, 0.4);    // Green - down quarks
                } else if (ptype == PTYPE_POSITRON) {
                    fragColor = vec3(1.0, 0.7, 0.2);    // Gold - positrons (antimatter)
                } else {
                    fragColor = vec3(0.5, 0.5, 0.5);    // Gray - generic
                }
                fragColor *= (0.85 + normSpeed * 0.3);
            }
            break;

        case 3: // Charge - Positive/Negative/Neutral
            {
                vec3 positive = vec3(1.0, 0.2, 0.1);   // Red for positive
                vec3 negative = vec3(0.1, 0.4, 1.0);   // Blue for negative
                vec3 neutral = vec3(0.4, 0.4, 0.4);    // Gray for neutral

                if (charge > 0.01) {
                    float intensity = clamp(charge, 0.0, 1.0);
                    fragColor = mix(neutral, positive, intensity);
                } else if (charge < -0.01) {
                    float intensity = clamp(-charge, 0.0, 1.0);
                    fragColor = mix(neutral, negative, intensity);
                } else {
                    fragColor = neutral;
                }
                fragColor *= (0.9 + normSpeed * 0.2);
            }
            break;

        case 4: // EM Force - Electromagnetic interaction visualization
            {
                // Visualize EM potential: charged particles glow based on charge magnitude
                float chargeMag = abs(charge);
                vec3 emNeutral = vec3(0.2, 0.2, 0.2);
                vec3 emWeak = vec3(0.3, 0.5, 0.8);
                vec3 emStrong = vec3(0.8, 0.9, 1.0);

                // Color based on charge strength and type
                if (chargeMag < 0.1) {
                    fragColor = emNeutral;
                } else {
                    float t = clamp(chargeMag, 0.0, 1.0);
                    fragColor = mix(emWeak, emStrong, t);
                    // Tint by charge sign
                    if (charge > 0.0) fragColor *= vec3(1.2, 0.9, 0.8);
                    else fragColor *= vec3(0.8, 0.9, 1.2);
                }
                // Pulse effect based on speed (moving charges create fields)
                fragColor *= (0.8 + normSpeed * 0.4);
            }
            break;

        case 5: // Strong Force - Quarks and nucleons glow
            {
                vec3 noStrong = vec3(0.15, 0.15, 0.2);
                vec3 nucleon = vec3(0.6, 0.3, 0.8);     // Purple for nucleons (residual)
                vec3 quark = vec3(1.0, 0.4, 1.0);       // Bright magenta for quarks

                if (ptype == PTYPE_UP_QUARK || ptype == PTYPE_DOWN_QUARK) {
                    fragColor = quark;
                    // Color charge visualization (simplified)
                    float colorPhase = fract(angle / 6.28318 * 3.0);
                    if (colorPhase < 0.33) fragColor = vec3(1.0, 0.2, 0.2);      // Red
                    else if (colorPhase < 0.66) fragColor = vec3(0.2, 1.0, 0.2); // Green
                    else fragColor = vec3(0.2, 0.2, 1.0);                         // Blue
                } else if (ptype == PTYPE_PROTON || ptype == PTYPE_NEUTRON) {
                    fragColor = nucleon;
                } else {
                    fragColor = noStrong;  // Leptons don't feel strong force
                }
                fragColor *= (0.8 + normSpeed * 0.3);
            }
            break;

        case 6: // Net Force - Combined force magnitude (inferred from acceleration proxy)
            {
                // Use velocity direction vs position as proxy for force direction
                vec3 posDir = normalize(pos + vec3(0.001));
                vec3 velDir = normalize(vel + vec3(0.001));
                float forceDot = dot(posDir, velDir);

                // Inward force (falling in) vs outward (escaping)
                vec3 inward = vec3(0.2, 0.4, 1.0);   // Blue - attracted inward
                vec3 tangent = vec3(0.2, 0.8, 0.3);  // Green - orbiting
                vec3 outward = vec3(1.0, 0.3, 0.1);  // Red - escaping

                if (forceDot < -0.3) {
                    fragColor = mix(tangent, inward, clamp(-forceDot - 0.3, 0.0, 1.0) / 0.7);
                } else if (forceDot > 0.3) {
                    fragColor = mix(tangent, outward, clamp(forceDot - 0.3, 0.0, 1.0) / 0.7);
                } else {
                    fragColor = tangent;
                }
                fragColor *= (0.8 + normSpeed * 0.4);
            }
            break;

        case 7: // Potential - Gravitational potential well depth
            {
                // Deeper in potential well = closer to center with low velocity
                float potential = normDist;  // Simplified: distance from center
                float kineticRatio = normSpeed / (normDist + 0.1);

                vec3 deep = vec3(0.8, 0.2, 0.1);    // Red - deep in well
                vec3 mid = vec3(0.9, 0.7, 0.2);     // Yellow - moderate
                vec3 shallow = vec3(0.2, 0.6, 1.0); // Blue - near escape

                if (potential < 0.3) {
                    fragColor = mix(deep, mid, potential / 0.3);
                } else {
                    fragColor = mix(mid, shallow, (potential - 0.3) / 0.7);
                }
            }
            break;

        case 8: // Distance - Distance from center
            {
                vec3 close = vec3(1.0, 0.9, 0.7);
                vec3 mid = vec3(0.4, 0.6, 0.9);
                vec3 far = vec3(0.1, 0.15, 0.3);

                if (normDist < 0.5) {
                    fragColor = mix(close, mid, normDist * 2.0);
                } else {
                    fragColor = mix(mid, far, (normDist - 0.5) * 2.0);
                }
            }
            break;

        case 9: // Kinetic Energy - mass * velocity^2
            {
                float kinetic = normMass * normSpeed * normSpeed;
                kinetic = clamp(kinetic * 3.0, 0.0, 1.0);

                vec3 cold = vec3(0.1, 0.1, 0.3);
                vec3 warm = vec3(0.8, 0.4, 0.1);
                vec3 hot = vec3(1.0, 1.0, 0.8);

                if (kinetic < 0.5) {
                    fragColor = mix(cold, warm, kinetic * 2.0);
                } else {
                    fragColor = mix(warm, hot, (kinetic - 0.5) * 2.0);
                }
            }
            break;

        case 10: // Binding - Bound vs escaping (energy analysis)
            {
                // Simplified: compare kinetic proxy to potential proxy
                float escapeProxy = normSpeed * normSpeed - 1.0 / (normDist + 0.1) * 0.3;
                escapeProxy = clamp(escapeProxy * 2.0 + 0.5, 0.0, 1.0);

                vec3 bound = vec3(0.2, 0.5, 0.9);     // Blue - gravitationally bound
                vec3 marginal = vec3(0.7, 0.7, 0.3);  // Yellow - marginally bound
                vec3 escaping = vec3(1.0, 0.3, 0.2);  // Red - escaping

                if (escapeProxy < 0.4) {
                    fragColor = mix(bound, marginal, escapeProxy / 0.4);
                } else {
                    fragColor = mix(marginal, escaping, (escapeProxy - 0.4) / 0.6);
                }
            }
            break;

        case 11: // Temperature - Thermal velocity (speed as temperature proxy)
            {
                float temp = normSpeed;
                fragColor = firePalette(temp);
            }
            break;

        case 12: // Fire - Aesthetic fire palette
            {
                float heat = normSpeed * 0.6 + normMass * 0.4;
                fragColor = firePalette(heat) * (1.0 + heat * 0.3);
            }
            break;

        case 13: // Galaxy - Spiral galaxy colors
            fragColor = galaxyPalette(1.0 - normDist, angle) * (1.1 - normDist * 0.3);
            break;

        case 14: // Rainbow - Position-based hue
            {
                float hue = fract(angle / 6.28318 + 0.5 + pos.z * 0.002);
                fragColor = hsv2rgb(vec3(hue, 0.85, 0.95));
            }
            break;

        case 15: // Electric - Storm/lightning aesthetic
            {
                float bolt = pow(sin(angle * 8.0 + dist * 0.1) * 0.5 + 0.5, 3.0);
                fragColor = electricPalette(normSpeed + bolt * 0.3) * (1.0 + bolt * 0.8);
            }
            break;

        case 16: // Black Hole - Accretion disk
            {
                float r = normDist;
                vec3 outer = vec3(0.1, 0.15, 0.3);
                vec3 disk = vec3(1.0, 0.5, 0.1);
                vec3 inner = vec3(1.0, 1.0, 0.9);

                if (r > 0.5) fragColor = mix(disk, outer, (r - 0.5) * 2.0);
                else if (r > 0.1) fragColor = mix(inner, disk, (r - 0.1) / 0.4);
                else fragColor = inner * (1.0 + (0.1 - r) * 5.0);
                fragColor *= (0.9 + normSpeed * 0.4);
            }
            break;

        case 17: // Plasma - Hot ionized gas
            {
                float ionization = abs(charge) * 0.5 + normSpeed * 0.5;
                ionization = clamp(ionization, 0.0, 1.0);

                vec3 cool = vec3(0.2, 0.1, 0.4);
                vec3 warm = vec3(0.8, 0.3, 0.5);
                vec3 hot = vec3(1.0, 0.9, 0.7);

                if (ionization < 0.5) {
                    fragColor = mix(cool, warm, ionization * 2.0);
                } else {
                    fragColor = mix(warm, hot, (ionization - 0.5) * 2.0);
                }
            }
            break;

        case 18: // Nebula - Cosmic gas cloud
            {
                fragColor = cosmicPalette(normDist * 0.7 + normSpeed * 0.3);
                fragColor *= (1.1 - normDist * 0.3);
            }
            break;

        case 19: // Quantum - Wavefunction-inspired (probability density)
            {
                // Phase based on position, amplitude based on mass
                float phase = fract(length(pos) * 0.05 + angle * 0.5);
                float amplitude = normMass * 0.5 + 0.5;

                vec3 node = vec3(0.05, 0.05, 0.15);
                vec3 antinode = vec3(0.3, 0.5, 1.0);
                vec3 peak = vec3(1.0, 0.8, 1.0);

                float wave = sin(phase * 6.28318) * 0.5 + 0.5;
                wave *= amplitude;

                if (wave < 0.5) {
                    fragColor = mix(node, antinode, wave * 2.0);
                } else {
                    fragColor = mix(antinode, peak, (wave - 0.5) * 2.0);
                }
            }
            break;

        case 20: // Relativity - Lorentz gamma factor and time dilation
            {
                // Color based on relativistic gamma factor
                // gamma = 1/sqrt(1 - v²/c²), where c is speed of light
                float v = speed;
                float c = speedOfLight;
                float v_over_c = clamp(v / c, 0.0, 0.999);
                float gamma = 1.0 / sqrt(1.0 - v_over_c * v_over_c);

                // Normalize gamma: 1.0 = non-relativistic, higher = more relativistic
                // gamma of 2 = 87% speed of light, gamma of 7 = 99% speed of light
                float relFactor = clamp((gamma - 1.0) / 6.0, 0.0, 1.0);

                // Color gradient: blue (slow) -> white (moderate) -> red/orange (relativistic)
                vec3 slow = vec3(0.2, 0.3, 0.8);      // Blue - non-relativistic
                vec3 moderate = vec3(0.9, 0.9, 1.0);  // White - mildly relativistic
                vec3 fast = vec3(1.0, 0.4, 0.1);      // Orange/red - highly relativistic
                vec3 extreme = vec3(1.0, 1.0, 0.5);   // Yellow-white - near light speed

                if (relFactor < 0.3) {
                    fragColor = mix(slow, moderate, relFactor / 0.3);
                } else if (relFactor < 0.7) {
                    fragColor = mix(moderate, fast, (relFactor - 0.3) / 0.4);
                } else {
                    fragColor = mix(fast, extreme, (relFactor - 0.7) / 0.3);
                }

                // Add glow effect for very relativistic particles
                fragColor *= (1.0 + relFactor * 0.5);
            }
            break;

        default:
            fragColor = plasmaPalette(normDist + normSpeed * 0.5);
    }

    // Depth-based color shift
    float depthTint = clamp(final.z * 0.002, -0.1, 0.1);
    fragColor.b += depthTint;
    fragColor.r -= depthTint * 0.5;

    // Relativistic Doppler shift (redshift/blueshift)
    // Objects moving toward camera are blueshifted, away are redshifted
    if (speedOfLight > 1.0) {
        // Calculate radial velocity (velocity component toward/away from camera)
        vec3 camDir = normalize(vec3(camX, camY, camZ) - pos);  // Direction to camera
        float radialVel = dot(vel, camDir);  // Positive = toward camera

        // Doppler factor: sqrt((1+v/c)/(1-v/c)) for approaching
        // Simplified: shift = v/c for non-relativistic
        float v_over_c = clamp(radialVel / speedOfLight, -0.5, 0.5);

        // Apply color shift: blue for approaching, red for receding
        // Convert to HSV, shift hue, convert back
        vec3 hsv = rgb2hsv(fragColor);

        // Blueshift = decrease wavelength = increase frequency = shift hue toward blue (0.66)
        // Redshift = increase wavelength = decrease frequency = shift hue toward red (0.0)
        float hueShift = -v_over_c * 0.15;  // Approaching = positive v_over_c = negative shift toward blue
        hsv.x = fract(hsv.x + hueShift);

        // Also adjust brightness based on relativistic beaming
        // Approaching objects appear brighter
        float beamingFactor = 1.0 + v_over_c * 0.5;
        hsv.z *= beamingFactor;

        fragColor = hsv2rgb(hsv);
    }

    // Black holes emit nothing here - the composite pass renders them as a
    // gravitationally lensed shadow with a photon ring
    if (ptype == 7) fragColor = vec3(0.0);

    // Pulsars strobe: a hot blue-white beacon flashing at the spin rate
    if (ptype == 8) {
        float strobe = 1.3 + 1.1 * sin(simTime * 22.0);
        fragColor = vec3(0.72, 0.85, 1.0) * strobe;
    }

    fragColor = clamp(fragColor, 0.0, 1.5);
    fragSize = gl_PointSize;
    fragDepth = final.z;
}
