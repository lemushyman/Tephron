#include "common.h"

void handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT: exit(0); break;

            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_RESIZED ||
                    e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    app.needsResize = true;
                    app.menuDirty = true;
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    // Panel dock controls take priority over everything else
                    if (app.hoverPanelBtn >= 0) {
                        switch (app.hoverPanelBtn) {
                            case 1: app.instrMinimized = true; break;
                            case 2: app.instrMinimized = false; break;
                            case 3: app.gwMinimized = true; break;
                            case 4: app.gwMinimized = false; break;
                        }
                        app.menuDirty = true;
                    // Check if clicking on menu
                    } else if (app.showMenu && app.hoverMenu >= 0) {
                        // Menu indices: 0=Gravity, 1=Dist, 2=Mass, 3=Color, 4=Mat, 5=Preset,
                        // 6=EM, 7=Strong, 8=Relativ, 9=Expand, 10=Auto
                        // Items 6-10 are toggles (no dropdown, just toggle on click)
                        if (app.hoverMenu >= 6) {
                            // Toggle items - toggle value directly
                            switch (app.hoverMenu) {
                                case 6: app.emEnabled = !app.emEnabled; break;
                                case 7: app.strongForceEnabled = !app.strongForceEnabled; break;
                                case 8: app.relativityEnabled = !app.relativityEnabled; break;
                                case 9: app.expansionEnabled = !app.expansionEnabled; break;
                                case 10: app.displayMode = !app.displayMode; break;
                            }
                            app.openMenu = -1;
                        } else {
                            // Dropdown menus - toggle dropdown
                            if (app.openMenu == app.hoverMenu) {
                                app.openMenu = -1;
                            } else {
                                app.openMenu = app.hoverMenu;
                            }
                        }
                        app.menuDirty = true;
                    } else if (app.showMenu && app.openMenu >= 0 && app.hoverDropdownItem >= 0) {
                        // Select dropdown item (only for non-toggle menus 0-5)
                        switch (app.openMenu) {
                            case 0: app.gravityType = app.hoverDropdownItem; break;
                            case 1: app.distribution = app.hoverDropdownItem; initParticles(); break;
                            case 2: app.massMode = app.hoverDropdownItem; initParticles(); break;
                            case 3: app.colorMode = app.hoverDropdownItem; break;
                            case 4: app.particleMode = app.hoverDropdownItem; initParticles(); break;
                            case 5: applyPreset(app.hoverDropdownItem); initParticles(); break;
                        }
                        app.openMenu = -1;
                        app.menuDirty = true;
                    } else {
                        // Close menu and start dragging
                        app.openMenu = -1;
                        app.mouseDragging = true;
                        app.mouseLastX = e.button.x;
                        app.mouseLastY = e.button.y;
                        app.menuDirty = true;
                    }
                } else if (e.button.button == SDL_BUTTON_MIDDLE) {
                    // Middle-click: Spawn black hole at world position under cursor
                    float ndcX = (float)e.button.x / app.swapchainExtent.width * 2.0f - 1.0f;
                    float ndcY = (float)e.button.y / app.swapchainExtent.height * 2.0f - 1.0f;
                    float aspect = (float)app.swapchainExtent.width / app.swapchainExtent.height;
                    float tanHalfFov = tanf(30.0f * M_PI / 180.0f);

                    // View-space ray direction
                    float rvx = ndcX * aspect * tanHalfFov / app.camZoom;
                    float rvy = ndcY * tanHalfFov / app.camZoom;
                    float rvz = 1.0f;

                    // Inverse camera rotation (RotY(-yaw) * RotX(-pitch))
                    float cx = cosf(-app.camRotX), sx = sinf(-app.camRotX);
                    float cy = cosf(-app.camRotY), sy = sinf(-app.camRotY);

                    // RotX(-pitch)
                    float ry = cx * rvy - sx * rvz;
                    float rz = sx * rvy + cx * rvz;
                    float rx = rvx;

                    // RotY(-yaw)
                    float wx = cy * rx + sy * rz;
                    float wy = ry;
                    float wz = -sy * rx + cy * rz;

                    // Normalize
                    float len = sqrtf(wx*wx + wy*wy + wz*wz);
                    wx /= len; wy /= len; wz /= len;

                    // Place at distance = distance to center of mass (or 500 if unavailable)
                    float comX, comY, comZ;
                    float spawnDist = 500.0f;
                    if (getCenterOfMass(comX, comY, comZ)) {
                        float dx = comX - app.camX, dy = comY - app.camY, dz = comZ - app.camZ;
                        spawnDist = sqrtf(dx*dx + dy*dy + dz*dz);
                    }

                    spawnBlackHole(app.camX + wx * spawnDist,
                                   app.camY + wy * spawnDist,
                                   app.camZ + wz * spawnDist);
                } else if (e.button.button == SDL_BUTTON_RIGHT) {
                    // Right-click: Start orbit mode - lock current center of mass as pivot
                    float comX, comY, comZ;
                    if (getCenterOfMass(comX, comY, comZ)) {
                        app.orbitTargetX = comX;
                        app.orbitTargetY = comY;
                        app.orbitTargetZ = comZ;

                        // Calculate current distance from camera to the pivot
                        float dx = app.camX - comX;
                        float dy = app.camY - comY;
                        float dz = app.camZ - comZ;
                        app.orbitDistance = sqrtf(dx*dx + dy*dy + dz*dz);
                        if (app.orbitDistance < 10.0f) app.orbitDistance = 800.0f;

                        app.orbitMode = true;
                        app.rightMouseDragging = true;
                        app.mouseLastX = e.button.x;
                        app.mouseLastY = e.button.y;
                    }
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    app.mouseDragging = false;
                } else if (e.button.button == SDL_BUTTON_RIGHT) {
                    app.rightMouseDragging = false;
                    app.orbitMode = false;
                }
                break;

            case SDL_MOUSEMOTION:
                // Always track mouse position for menu hover
                app.mouseX = e.motion.x;
                app.mouseY = e.motion.y;
                if (app.showMenu) app.menuDirty = true;

                if (app.mouseDragging) {
                    // Normal camera rotation (left-click drag)
                    app.camRotY += (e.motion.x - app.mouseLastX) * 0.005f;
                    app.camRotX += (e.motion.y - app.mouseLastY) * 0.005f;
                    app.mouseLastX = e.motion.x;
                    app.mouseLastY = e.motion.y;
                }

                if (app.rightMouseDragging && app.orbitMode) {
                    // Orbit mode - rotate camera around fixed pivot point
                    float dx = (e.motion.x - app.mouseLastX) * 0.005f;
                    float dy = (e.motion.y - app.mouseLastY) * 0.005f;
                    app.mouseLastX = e.motion.x;
                    app.mouseLastY = e.motion.y;

                    // Update orbit angles (camera rotation)
                    app.camRotY += dx;
                    app.camRotX += dy;

                    // Recalculate camera position based on orbit around the fixed pivot
                    float pitch = app.camRotX;
                    float yaw = app.camRotY;
                    float cp = cosf(pitch), sp = sinf(pitch);
                    float cy = cosf(yaw), sy = sinf(yaw);

                    // Camera position is orbitDistance behind the pivot point
                    // Camera looks at pivot, so forward direction points from camera to pivot
                    // Camera position = pivot - forward * orbitDistance
                    float forwardX = -cp * sy;
                    float forwardY = sp;
                    float forwardZ = cp * cy;

                    app.camX = app.orbitTargetX - forwardX * app.orbitDistance;
                    app.camY = app.orbitTargetY - forwardY * app.orbitDistance;
                    app.camZ = app.orbitTargetZ - forwardZ * app.orbitDistance;
                }
                break;

            case SDL_MOUSEWHEEL:
                if (e.wheel.y != 0 && app.showMenu && app.hoverMenu >= 0) {
                    // Scroll over any top-bar item cycles its value
                    int dir = (e.wheel.y > 0) ? 1 : -1;
                    switch (app.hoverMenu) {
                        case 0: app.gravityType = (app.gravityType + dir + GRAVITY_COUNT) % GRAVITY_COUNT; break;
                        case 1: app.distribution = (app.distribution + dir + DIST_COUNT) % DIST_COUNT; initParticles(); break;
                        case 2: app.massMode = (app.massMode + dir + MASS_COUNT) % MASS_COUNT; initParticles(); break;
                        case 3: app.colorMode = (app.colorMode + dir + COLOR_COUNT) % COLOR_COUNT; break;
                        case 4: app.particleMode = (app.particleMode + dir + MATTER_COUNT) % MATTER_COUNT; initParticles(); break;
                        case 5: applyPreset((app.currentPreset + dir + PRESET_COUNT) % PRESET_COUNT); initParticles(); break;
                        case 6: app.emEnabled = !app.emEnabled; break;
                        case 7: app.strongForceEnabled = !app.strongForceEnabled; break;
                        case 8: app.relativityEnabled = !app.relativityEnabled; break;
                        case 9: app.expansionEnabled = !app.expansionEnabled; break;
                        case 10: app.displayMode = !app.displayMode; break;
                    }
                    app.menuDirty = true;
                    break;
                }
                if (e.wheel.y != 0) {
                    // Check if hovering over a row2 variable (scroll to adjust)
                    // 0=G, 1=Soft, 2=Time, 3=Part, 4=Hubble, 5=c, 6=Lens, 7=Bound, 8=Trail, 9=Dark, 10=Glow, 11=BH
                    if (app.showMenu && app.hoverRow2Var >= 0) {
                        int dir = (e.wheel.y > 0) ? 1 : -1;
                        app.menuDirty = true;
                        switch (app.hoverRow2Var) {
                            case 0: // G (gravity strength)
                                app.G = std::clamp(app.G + dir * 5.0f, 1.0f, 200.0f);
                                break;
                            case 1: // Softening
                                app.softening = std::clamp(app.softening + dir * 0.5f, 0.1f, 50.0f);
                                break;
                            case 2: // Time scale
                                app.timeScale = std::clamp(app.timeScale + dir * 0.1f, 0.1f, 5.0f);
                                break;
                            case 3: // Particle count
                                app.numParticles = std::clamp(app.numParticles + dir * 5000, 1000, app.maxParticlesCap);
                                initParticles();
                                break;
                            case 4: // Hubble constant
                                app.hubbleConstant = std::clamp(app.hubbleConstant + dir * 0.005f, 0.001f, 0.1f);
                                break;
                            case 5: // Speed of light
                                app.speedOfLight = std::clamp(app.speedOfLight + dir * 50.0f, 100.0f, 2000.0f);
                                break;
                            case 6: // Lensing strength
                                app.lensingStrength = std::fmod(app.lensingStrength + dir * 0.5f + 3.0f, 3.0f);
                                break;
                            case 7: // Boundary mode (cycle)
                                app.boundaryMode = (app.boundaryMode + dir + 3) % 3;
                                break;
                            case 8: // Trails toggle
                                app.trailsEnabled = !app.trailsEnabled;
                                break;
                            case 9: // Dark matter toggle
                                app.darkMatter = !app.darkMatter;
                                break;
                            case 10: // Glow intensity
                                app.glowIntensity = std::clamp(app.glowIntensity + dir * 0.05f, 0.01f, 2.0f);
                                break;
                            case 11: // Black hole mass
                                app.blackHoleMass = std::clamp(app.blackHoleMass * (dir > 0 ? 2.0f : 0.5f), 1000.0f, 1000000.0f);
                                break;
                            case 12: // Accretion merging toggle
                                app.mergeEnabled = !app.mergeEnabled;
                                break;
                            case 13: // Starfield toggle
                                app.starfieldEnabled = !app.starfieldEnabled;
                                break;
                            case 14: // Black hole charge (units of e)
                                app.blackHoleCharge = std::clamp(app.blackHoleCharge + dir * 5.0f,
                                                                 -50.0f, 50.0f);
                                break;
                            case 15: // Black hole spin (frame dragging)
                                app.blackHoleSpin = std::clamp(app.blackHoleSpin + dir * 0.1f,
                                                               -1.0f, 1.0f);
                                break;
                        }
                    } else {
                        // Normal zoom behavior
                        float factor = (e.wheel.y > 0) ? 1.15f : 1.0f / 1.15f;
                        app.camZoom *= powf(factor, fabsf((float)e.wheel.y));
                        // camZoom >= 1.0 always (can't zoom out past default), no upper limit for zoom in
                        app.camZoom = std::max(app.camZoom, 1.0f);
                    }
                }
                break;

            case SDL_KEYDOWN:
                app.menuDirty = true;
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: exit(0); break;
                    case SDLK_SPACE: app.paused = !app.paused; break;
                    case SDLK_TAB: app.menuPage = (app.menuPage + 1) % 2; break;
                    case SDLK_h: app.showMenu = !app.showMenu; break;

                    // Cycle modes
                    case SDLK_g: app.gravityType = (app.gravityType + 1) % GRAVITY_COUNT; break;
                    case SDLK_c: app.colorMode = (app.colorMode + 1) % COLOR_COUNT; break;
                    case SDLK_f: app.distribution = (app.distribution + 1) % DIST_COUNT; initParticles(); break;  // F for distribution (was D)
                    case SDLK_m: app.massMode = (app.massMode + 1) % MASS_COUNT; initParticles(); break;
                    case SDLK_p: app.particleMode = (app.particleMode + 1) % MATTER_COUNT; initParticles(); break;
                    case SDLK_r:
                        // Reset particles and camera to default viewing position
                        app.camX = 0.0f; app.camY = 0.0f; app.camZ = -800.0f;
                        app.camRotX = 0.0f; app.camRotY = 0.0f;
                        app.camZoom = 1.0f;
                        app.orbitMode = false;
                        initParticles();
                        break;

                    case SDLK_HOME:
                        // Center camera on particles and auto-zoom to frame them
                        centerCameraOnParticles();
                        break;

                    // Particle count
                    case SDLK_EQUALS: case SDLK_PLUS:
                        app.numParticles = std::min(app.numParticles + 5000, app.maxParticlesCap);
                        initParticles(); break;
                    case SDLK_MINUS:
                        app.numParticles = std::max(app.numParticles - 5000, 1000);
                        initParticles(); break;

                    // Time scale
                    case SDLK_COMMA: case SDLK_LESS:
                        app.timeScale = std::max(0.1f, app.timeScale - 0.1f); break;
                    case SDLK_PERIOD: case SDLK_GREATER:
                        app.timeScale = std::min(5.0f, app.timeScale + 0.1f); break;

                    // Physics parameters (use U/O for gravity, Z/X for softening)
                    case SDLK_u: app.G = std::max(1.0f, app.G - 5.0f); break;
                    case SDLK_o: app.G = std::min(200.0f, app.G + 5.0f); break;
                    case SDLK_z: app.softening = std::max(0.1f, app.softening - 0.5f); break;
                    case SDLK_x: app.softening = std::min(50.0f, app.softening + 0.5f); break;

                    // Instruments / accretion / starfield
                    case SDLK_i: app.showInstruments = !app.showInstruments; break;
                    case SDLK_v: app.mergeEnabled = !app.mergeEnabled; break;
                    case SDLK_n: app.starfieldEnabled = !app.starfieldEnabled; break;

                    // Save / load / screenshot
                    case SDLK_F10:
                        if (SDL_GetModState() & KMOD_SHIFT) loadState();
                        else saveState();
                        break;
                    case SDLK_F12: app.screenshotRequested = true; break;

                    // Trail controls
                    case SDLK_t: app.trailsEnabled = !app.trailsEnabled; break;
                    case SDLK_LEFTBRACKET:
                        app.trailUpdateFreq = std::min(10, app.trailUpdateFreq + 1); break;
                    case SDLK_RIGHTBRACKET:
                        app.trailUpdateFreq = std::max(1, app.trailUpdateFreq - 1); break;

                    // Force toggles
                    case SDLK_F2: app.emEnabled = !app.emEnabled; break;
                    case SDLK_F3: app.strongForceEnabled = !app.strongForceEnabled; break;
                    case SDLK_F4: app.gravityEnabled = !app.gravityEnabled; break;
                    case SDLK_F5: app.relativityEnabled = !app.relativityEnabled; break;
                    case SDLK_F6: app.expansionEnabled = !app.expansionEnabled; break;

                    // Relativistic/cosmological parameter adjustments
                    case SDLK_F7: app.hubbleConstant = std::max(0.001f, app.hubbleConstant - 0.005f); break;
                    case SDLK_F8: app.hubbleConstant = std::min(0.1f, app.hubbleConstant + 0.005f); break;
                    case SDLK_F9: app.displayMode = !app.displayMode; break;
                    case SDLK_l: app.lensingStrength = std::fmod(app.lensingStrength + 0.5f, 3.0f); break;

                    case SDLK_b: app.boundaryMode = (app.boundaryMode + 1) % 3; break;
                    case SDLK_F1: app.darkMatter = !app.darkMatter; break;

                    // Arrow keys for camera rotation (track state for smooth rotation)
                    case SDLK_LEFT: app.keyLeft = true; break;
                    case SDLK_RIGHT: app.keyRight = true; break;
                    case SDLK_UP: app.keyUp = true; break;
                    case SDLK_DOWN: app.keyDown = true; break;
                    case SDLK_F11:
                        app.fullscreen = !app.fullscreen;
                        SDL_SetWindowFullscreen(app.window, app.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                        app.needsResize = true;
                        break;

                    // Presets
                    case SDLK_1: applyPreset(0); initParticles(); break;
                    case SDLK_2: applyPreset(1); initParticles(); break;
                    case SDLK_3: applyPreset(2); initParticles(); break;
                    case SDLK_4: applyPreset(3); initParticles(); break;
                    case SDLK_5: applyPreset(4); initParticles(); break;
                    case SDLK_6: applyPreset(5); initParticles(); break;
                    case SDLK_7: applyPreset(6); initParticles(); break;
                    case SDLK_8: applyPreset(7); initParticles(); break;
                    case SDLK_9: applyPreset(8); initParticles(); break;
                    case SDLK_0: applyPreset(9); initParticles(); break;

                    // WASDQE camera movement (track key state)
                    case SDLK_w: app.keyW = true; break;
                    case SDLK_a: app.keyA = true; break;
                    case SDLK_s: app.keyS = true; break;
                    case SDLK_d: app.keyD = true; break;
                    case SDLK_q: app.keyQ = true; break;
                    case SDLK_e: app.keyE = true; break;

                    default: break;
                }
                break;

            case SDL_KEYUP:
                switch (e.key.keysym.sym) {
                    case SDLK_w: app.keyW = false; break;
                    case SDLK_a: app.keyA = false; break;
                    case SDLK_s: app.keyS = false; break;
                    case SDLK_d: app.keyD = false; break;
                    case SDLK_q: app.keyQ = false; break;
                    case SDLK_e: app.keyE = false; break;
                    case SDLK_LEFT: app.keyLeft = false; break;
                    case SDLK_RIGHT: app.keyRight = false; break;
                    case SDLK_UP: app.keyUp = false; break;
                    case SDLK_DOWN: app.keyDown = false; break;
                    default: break;
                }
                break;
        }
    }

    // WASD camera movement - FPS-style fly camera
    // The shader transforms: final = RotX(pitch) * RotY(yaw) * (worldPos - camPos)
    // Center of screen is final = (0, 0, Z). Working backwards to find world direction:
    //   rotated = RotX(-pitch) * (0, 0, Z) = (0, sin(p)*Z, cos(p)*Z)
    //   viewPos = RotY(-yaw) * rotated = (-cos(p)*sin(y)*Z, sin(p)*Z, cos(p)*cos(y)*Z)
    // Direction from camera to center of view: (-cp*sy, +sp, cp*cy)

    float pitch = app.camRotX;
    float yaw = app.camRotY;
    float cp = cosf(pitch), sp = sinf(pitch);
    float cy = cosf(yaw), sy = sinf(yaw);

    // Forward = direction camera is looking (toward center of screen)
    float forwardX = -cp * sy;
    float forwardY = sp;    // Positive pitch = camera tilted up = forward points up
    float forwardZ = cp * cy;

    // Right = perpendicular to forward in the horizontal plane
    float rightX = cy;
    float rightZ = sy;

    // Fast movement speed to match zoom speed
    float moveSpeed = 80.0f;

    if (app.keyW) {
        app.camX += forwardX * moveSpeed;
        app.camY += forwardY * moveSpeed;
        app.camZ += forwardZ * moveSpeed;
    }
    if (app.keyS) {
        app.camX -= forwardX * moveSpeed;
        app.camY -= forwardY * moveSpeed;
        app.camZ -= forwardZ * moveSpeed;
    }
    if (app.keyA) {
        app.camX -= rightX * moveSpeed;
        app.camZ -= rightZ * moveSpeed;
    }
    if (app.keyD) {
        app.camX += rightX * moveSpeed;
        app.camZ += rightZ * moveSpeed;
    }
    if (app.keyQ) {
        app.camY += moveSpeed;  // Q = strafe up
    }
    if (app.keyE) {
        app.camY -= moveSpeed;  // E = strafe down
    }

    // Smooth arrow key rotation
    float rotSpeed = 0.03f;
    if (app.keyLeft) app.camRotY -= rotSpeed;
    if (app.keyRight) app.camRotY += rotSpeed;
    if (app.keyUp) app.camRotX -= rotSpeed;
    if (app.keyDown) app.camRotX += rotSpeed;
}
