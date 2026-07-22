#include "common.h"
#include "icon_data.h"

App app;

static void selftestStep(int frames, float dt) {
    for (int i = 0; i < frames; i++) {
        app.frameDeltaTime = dt;
        handleEvents();
        runCompute();
        readDiagnostics(true);
        runMergePass();
        runSortStateMachine();
        renderFrame();
    }
}

int runSelftest() {
    printf("\n=== SELFTEST ===\n");
    int failures = 0;
    auto report = [&](const char* name, bool pass, const char* detail) {
        printf("  [%s] %s\n         %s\n", pass ? "PASS" : "FAIL", name, detail);
        if (!pass) failures++;
    };
    char detail[256];

    app.showMenu = true;
    app.showInstruments = true;
    app.trailsEnabled = false;   // skip CPU trail readbacks for speed
    app.timeScale = 1.0f;
    app.displayMode = false;

    // --- Energy & momentum conservation: equilibrium Plummer sphere ---
    app.distribution = DIST_PLUMMER;
    app.massMode = MASS_UNIFORM;
    app.particleMode = 0;
    app.gravityType = GRAVITY_NEWTONIAN;
    app.G = 30.0f; app.softening = 5.0f; app.damping = 0.0f;
    app.numParticles = 8192;
    app.mergeEnabled = false;
    initParticles();
    selftestStep(5, 0.008f);   // prime the Verlet history
    double e0 = app.diag.ke + app.diag.pe;
    double p0x = app.diag.px, p0y = app.diag.py, p0z = app.diag.pz;
    double m0 = app.diag.mass;
    double vChar = app.diag.speedSum / std::max(app.diag.count, 1.0);
    selftestStep(500, 0.008f);
    double e1 = app.diag.ke + app.diag.pe;
    double drift = fabs((e1 - e0) / e0) * 100.0;
    snprintf(detail, sizeof(detail), "dE = %.3f%% over 500 steps (E %.5g -> %.5g)", drift, e0, e1);
    report("energy conservation (Plummer, velocity Verlet)",
           std::isfinite(drift) && drift < 2.0, detail);

    double dp = sqrt(pow(app.diag.px - p0x, 2) + pow(app.diag.py - p0y, 2) +
                     pow(app.diag.pz - p0z, 2));
    double pScale = std::max(m0 * vChar, 1e-9);
    snprintf(detail, sizeof(detail), "|dP| / (M * v_char) = %.6f", dp / pScale);
    report("momentum conservation", dp / pScale < 0.02, detail);

    // --- Far-field solver at large N: energy must still be conserved ---
    app.distribution = DIST_PLUMMER;
    app.massMode = MASS_UNIFORM;
    app.numParticles = 150000;
    initParticles();
    selftestStep(5, 0.008f);
    double eFar0 = app.diag.ke + app.diag.pe;
    uint32_t t0 = SDL_GetTicks();
    selftestStep(100, 0.008f);
    uint32_t t1 = SDL_GetTicks();
    double eFar1 = app.diag.ke + app.diag.pe;
    double farDrift = fabs((eFar1 - eFar0) / eFar0) * 100.0;
    bool farOK = app.diag.valid && (int)app.diag.count == app.numParticles &&
                 std::isfinite(farDrift) && farDrift < 2.0;
    snprintf(detail, sizeof(detail), "N=150000, dE = %.3f%% over 100 frames (%.1f ms/frame)",
             farDrift, (t1 - t0) / 100.0f);
    report("far-field solver at N=150k (energy conserved)", farOK, detail);

    // --- Accretion merging conserves mass, reduces particle count ---
    app.distribution = DIST_ACCRETION;
    app.massMode = MASS_UNIFORM;
    app.G = 80.0f; app.softening = 2.0f;
    app.numParticles = 6000;
    initParticles();
    app.mergeEnabled = true;
    selftestStep(5, 0.016f);
    double massBefore = app.diag.mass;
    int nBefore = app.numParticles;
    selftestStep(400, 0.016f);
    double massAfter = app.diag.mass;
    double massErr = fabs(massAfter - massBefore) / massBefore * 100.0;
    snprintf(detail, sizeof(detail), "merges=%d (N %d -> %d), mass drift %.4f%%",
             app.mergeCount, nBefore, app.numParticles, massErr);
    report("accretion merging (bound pairs merge, mass conserved)",
           app.mergeCount > 0 && massErr < 0.1, detail);

    printf("=== SELFTEST %s (%d failure%s) ===\n\n",
           failures ? "FAILED" : "PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}

int main(int argc, char** argv) {
    srand(time(nullptr));

    bool selftest = false;
    int spawnTestBH = 0;
    float camArgs[6] = {0, 0, 0, 0, 0, 1.0f};
    bool camSet = false;
    int startPreset = PRESET_GALAXY;
    int particleOverride = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--selftest") == 0) selftest = true;
        else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) app.maxFrames = atol(argv[++i]);
        else if (strcmp(argv[i], "--preset") == 0 && i + 1 < argc)
            startPreset = std::clamp(atoi(argv[++i]), 0, PRESET_COUNT - 1);
        else if (strcmp(argv[i], "--fixed-dt") == 0 && i + 1 < argc) app.fixedDt = atof(argv[++i]);
        else if (strcmp(argv[i], "--particles") == 0 && i + 1 < argc)
            particleOverride = std::clamp(atoi(argv[++i]), 100, MAX_PARTICLES);
        else if (strcmp(argv[i], "--screenshot") == 0) app.autoScreenshot = true;
        else if (strcmp(argv[i], "--blackhole") == 0) spawnTestBH++;
        else if (strcmp(argv[i], "--cam") == 0 && i + 1 < argc) {
            // --cam x,y,z,pitch,yaw[,zoom] - shot composition for headless runs
            if (sscanf(argv[++i], "%f,%f,%f,%f,%f,%f", &camArgs[0], &camArgs[1],
                       &camArgs[2], &camArgs[3], &camArgs[4], &camArgs[5]) >= 5)
                camSet = true;
        }
        else if (strcmp(argv[i], "--help") == 0) { printHelp(); return 0; }
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
    if (TTF_Init() < 0) return 1;

    // Dev convenience: GRAV_WINPOS="x,y" positions the window explicitly
    // (pair with SDL_VIDEODRIVER=x11 - Wayland ignores client positioning).
    // Used by automated test runs to keep windows off the working monitor.
    int winX = SDL_WINDOWPOS_CENTERED, winY = SDL_WINDOWPOS_CENTERED;
    if (const char* wp = getenv("GRAV_WINPOS")) {
        int wx, wy;
        if (sscanf(wp, "%d,%d", &wx, &wy) == 2) { winX = wx; winY = wy; }
    }
    // GRAV_WINSIZE="WxH" overrides the initial window size (testing)
    if (const char* ws = getenv("GRAV_WINSIZE")) {
        int ww, wh;
        if (sscanf(ws, "%dx%d", &ww, &wh) == 2) {
            app.width = std::clamp(ww, 320, 8192);
            app.height = std::clamp(wh, 240, 8192);
        }
    }
    app.window = SDL_CreateWindow("Tephron",
        winX, winY,
        app.width, app.height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    // Project icon (embedded 64x64 RGBA - taskbar/switcher/window decorations)
    SDL_Surface* iconSurf = SDL_CreateRGBSurfaceFrom((void*)gravityIconRGBA, 64, 64, 32, 64 * 4,
        0x000000ffu, 0x0000ff00u, 0x00ff0000u, 0xff000000u);
    if (iconSurf) {
        SDL_SetWindowIcon(app.window, iconSurf);
        SDL_FreeSurface(iconSurf);
    }

    if (!initVulkan()) return 1;

    if (selftest) {
        srand(12345);  // deterministic initial conditions
        int code = runSelftest();
        cleanup();
        return code;
    }

    // Low-fidelity devices also get a smaller default window (fill rate);
    // an explicit GRAV_WINSIZE takes precedence
    if (app.lowFidelity && !getenv("GRAV_WINSIZE")) {
        SDL_SetWindowSize(app.window, 1024, 640);
        app.needsResize = true;
    }

    applyPreset(startPreset);
    // Particle count set by preset (CLI --particles overrides, capped to tier)
    if (particleOverride > 0)
        app.numParticles = std::min(particleOverride, app.maxParticlesCap);
    initParticles();
    if (getenv("GRAV_MIN_PANELS")) { app.instrMinimized = true; app.gwMinimized = true; }
    if (spawnTestBH >= 1) spawnBlackHole(80.0f, 30.0f, 0.0f, 0.0f, 40.0f, 0.0f);
    if (spawnTestBH >= 2) spawnBlackHole(-80.0f, -30.0f, 0.0f, 0.0f, -40.0f, 0.0f);
    // Use good default camera position (particles spawn around origin with ~200 radius)
    // Camera at Z=-800 looking at origin should show them nicely
    app.camX = 0.0f; app.camY = 0.0f; app.camZ = -800.0f;
    app.camRotX = 0.0f; app.camRotY = 0.0f;
    app.camZoom = 1.0f;  // Default zoom level
    if (camSet) {
        app.camX = camArgs[0]; app.camY = camArgs[1]; app.camZ = camArgs[2];
        app.camRotX = camArgs[3]; app.camRotY = camArgs[4];
        app.camZoom = std::max(camArgs[5], 0.1f);
    }
    updateUniforms();  // Initialize uniforms before first frame

    printHelp();
    printf("GPU: %s\n", app.gpuName);
    printf("Starting with %d particles...\n\n", app.numParticles);

    app.fpsTimer = SDL_GetTicks();
    app.lastFrameTime = SDL_GetTicks();
    long frameIdx = 0;

    while (true) {
        // Smoothed simulation clock: clamp outliers toward the running
        // average, then blend. A driver hitch or background pass must slow
        // the FRAME, never lurch the PHYSICS - uniform motion reads far
        // smoother than exact wall-clock coupling.
        uint32_t frameNow = SDL_GetTicks();
        float rawDt = (frameNow - app.lastFrameTime) / 1000.0f;
        app.lastFrameTime = frameNow;
        if (rawDt <= 0.0f || rawDt > 0.1f) rawDt = app.frameDtSmooth;
        rawDt = std::clamp(rawDt, app.frameDtSmooth * 0.5f, app.frameDtSmooth * 1.5f);
        app.frameDtSmooth += (rawDt - app.frameDtSmooth) * 0.10f;
        app.frameDeltaTime = app.frameDtSmooth;
        // Deterministic timestep override (CLI --fixed-dt)
        if (app.fixedDt > 0.0f) app.frameDeltaTime = app.fixedDt;

        handleEvents();

        // Track display mode transitions to reset state on toggle-on
        static bool prevDisplayMode = false;
        bool displayModeJustEnabled = app.displayMode && !prevDisplayMode;
        prevDisplayMode = app.displayMode;

        // Display mode: auto-orbit camera around center of mass
        if (app.displayMode) {
            static uint32_t lastTime = 0;
            static uint32_t lastCOMUpdate = 0;
            // Start and end points for S-curve interpolation
            static float startCOMx = 0, startCOMy = 0, startCOMz = 0, startRadius = 250.0f;
            static float endCOMx = 0, endCOMy = 0, endCOMz = 0, endRadius = 250.0f;
            static bool comInitialized = false;

            // Smooth entrance transition state
            static bool transitionActive = false;
            static uint32_t transitionStartTime = 0;
            static float prevCamX = 0, prevCamY = 0, prevCamZ = 0;
            static float prevCamRotX = 0, prevCamRotY = 0;

            // Reset on fresh toggle-on so entrance transition fires each time
            if (displayModeJustEnabled) {
                comInitialized = false;
                lastTime = 0;
            }

            uint32_t now = SDL_GetTicks();
            float deltaTime = (now - lastTime) / 1000.0f;
            if (deltaTime > 0.1f || deltaTime < 0.001f) deltaTime = 0.016f;
            lastTime = now;

            // Update orbit angle smoothly
            app.displayOrbitAngle += app.displayOrbitSpeed * deltaTime;

            const float transitionDuration = 3000.0f;  // 3 seconds between updates

            // Only update center of mass and bounding radius every 3 seconds
            if (!comInitialized || (now - lastCOMUpdate > (uint32_t)transitionDuration)) {
                float newX, newY, newZ, newRadius;
                if (getCenterOfMass(newX, newY, newZ, &newRadius)) {
                    // Current interpolated position becomes the new start
                    if (comInitialized) {
                        float t = std::min((now - lastCOMUpdate) / transitionDuration, 1.0f);
                        // Smootherstep for current position
                        float s = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
                        startCOMx = startCOMx + (endCOMx - startCOMx) * s;
                        startCOMy = startCOMy + (endCOMy - startCOMy) * s;
                        startCOMz = startCOMz + (endCOMz - startCOMz) * s;
                        startRadius = startRadius + (endRadius - startRadius) * s;
                    } else {
                        startCOMx = newX;
                        startCOMy = newY;
                        startCOMz = newZ;
                        startRadius = newRadius;
                        // First successful COM — capture current camera for smooth entrance
                        prevCamX = app.camX;
                        prevCamY = app.camY;
                        prevCamZ = app.camZ;
                        prevCamRotX = app.camRotX;
                        prevCamRotY = app.camRotY;
                        transitionActive = true;
                        transitionStartTime = now;
                        comInitialized = true;
                    }
                    // Set new targets
                    endCOMx = newX;
                    endCOMy = newY;
                    endCOMz = newZ;
                    endRadius = newRadius;
                }
                lastCOMUpdate = now;
            }

            // S-curve interpolation (smootherstep: 6t^5 - 15t^4 + 10t^3)
            // Provides constant velocity in the middle, smooth ease-in/ease-out at ends
            float t = std::min((now - lastCOMUpdate) / transitionDuration, 1.0f);
            float s = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);

            float comX = startCOMx + (endCOMx - startCOMx) * s;
            float comY = startCOMy + (endCOMy - startCOMy) * s;
            float comZ = startCOMz + (endCOMz - startCOMz) * s;
            float interpRadius = startRadius + (endRadius - startRadius) * s;

            // Camera orbits around COM at distance based on particle cloud size
            float boundingRadius = std::max(interpRadius, 50.0f);
            float orbitDist = boundingRadius * 2.5f;  // Keep most particles in view
            float camAngle = app.displayOrbitAngle;
            float tilt = app.displayOrbitTilt;

            float targetCamX = comX + orbitDist * cosf(camAngle) * cosf(tilt);
            float targetCamY = comY + orbitDist * sinf(tilt);
            float targetCamZ = comZ + orbitDist * sinf(camAngle) * cosf(tilt);

            // Point camera at center of mass
            float dx = comX - targetCamX;
            float dy = comY - targetCamY;
            float dz = comZ - targetCamZ;
            float targetRotY = atan2f(-dx, dz);
            float targetRotX = atan2f(dy, sqrtf(dx*dx + dz*dz));

            // Smooth entrance transition from manual camera to orbit position
            if (transitionActive) {
                const float entranceDuration = 2000.0f;  // 2 seconds
                float elapsed = (float)(now - transitionStartTime);
                float et = std::min(elapsed / entranceDuration, 1.0f);
                // Smootherstep for entrance
                float es = et * et * et * (et * (et * 6.0f - 15.0f) + 10.0f);

                app.camX = prevCamX + (targetCamX - prevCamX) * es;
                app.camY = prevCamY + (targetCamY - prevCamY) * es;
                app.camZ = prevCamZ + (targetCamZ - prevCamZ) * es;
                app.camRotX = prevCamRotX + (targetRotX - prevCamRotX) * es;
                app.camRotY = prevCamRotY + (targetRotY - prevCamRotY) * es;

                if (et >= 1.0f) {
                    transitionActive = false;
                }
            } else {
                app.camX = targetCamX;
                app.camY = targetCamY;
                app.camZ = targetCamZ;
                app.camRotX = targetRotX;
                app.camRotY = targetRotY;
            }

            app.camZoom = 1.0f;
        }

        runCompute();
        readDiagnostics();
        runMergePass();
        runSortStateMachine();
        renderFrame();
        renderMenu();

        // Screenshot request (F12 or --screenshot on the final frame)
        if (app.screenshotRequested) {
            app.screenshotRequested = false;
            captureScreenshot();
        }

        // Headless-run frame limit (CLI --frames)
        frameIdx++;
        if (app.maxFrames > 0 && frameIdx >= app.maxFrames) {
            if (app.autoScreenshot) captureScreenshot();
            break;
        }

        // FPS
        app.frameCount++;
        uint32_t now = SDL_GetTicks();
        if (now - app.fpsTimer >= 1000) {
            app.fps = app.frameCount * 1000.0 / (now - app.fpsTimer);
            app.frameCount = 0;
            app.fpsTimer = now;
            app.menuDirty = true;
        }
    }

    cleanup();
    return 0;
}
