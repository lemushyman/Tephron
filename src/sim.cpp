#include "common.h"

void applyPreset(int preset) {
    // Reset cosmological/relativistic effects to defaults (most presets don't use them)
    app.expansionEnabled = false;
    app.relativityEnabled = false;
    app.hubbleConstant = 0.01f;
    app.lensingStrength = 1.0f;
    app.mergeEnabled = false;
    app.currentPreset = preset;
    app.presetBHs.clear();

    switch (preset) {
        case PRESET_GALAXY:
            app.distribution = DIST_DISK;
            app.massMode = MASS_CENTRAL;
            app.gravityType = GRAVITY_NEWTONIAN;
            app.G = 50.0f;
            app.centralMass = 10000.0f;
            app.numParticles = 50000;
            app.damping = 0.0f;
            break;
        case PRESET_SOLAR_SYSTEM:
            app.distribution = DIST_SOLAR_SYSTEM;
            app.massMode = MASS_HIERARCHICAL;
            app.gravityType = GRAVITY_NEWTONIAN;
            app.G = 100.0f;
            app.numParticles = 10000;
            break;
        case PRESET_GALAXY_COLLISION:
            app.distribution = DIST_BINARY_GALAXIES;
            app.massMode = MASS_BINARY;
            app.gravityType = GRAVITY_NEWTONIAN;
            app.G = 40.0f;
            app.numParticles = 60000;
            break;
        case PRESET_GLOBULAR_CLUSTER:
            app.distribution = DIST_SPHERE;
            app.massMode = MASS_UNIFORM;
            app.gravityType = GRAVITY_NEWTONIAN;
            app.G = 30.0f;
            app.numParticles = 40000;
            break;
        case PRESET_BIG_BANG:
            app.distribution = DIST_EXPLOSION;
            app.massMode = MASS_RANDOM;
            app.gravityType = GRAVITY_NEWTONIAN;
            app.G = 20.0f;
            app.numParticles = 80000;
            app.darkMatter = true;
            // Enable cosmological effects for Big Bang simulation
            app.expansionEnabled = true;
            app.relativityEnabled = true;
            app.hubbleConstant = 0.02f;
            app.colorMode = COLOR_RELATIVITY;
            break;
        case PRESET_SATURN:
            app.distribution = DIST_RING;
            app.massMode = MASS_CENTRAL;
            app.gravityType = GRAVITY_NEWTONIAN;
            app.G = 80.0f;
            app.numParticles = 50000;
            break;
        case PRESET_BINARY_ORBIT:
            app.distribution = DIST_DISK;
            app.massMode = MASS_BINARY;
            app.gravityType = GRAVITY_NEWTONIAN;
            app.G = 60.0f;
            app.numParticles = 30000;
            break;
        case PRESET_CHAOS:
            app.distribution = DIST_RANDOM;
            app.massMode = MASS_RANDOM;
            app.gravityType = GRAVITY_OSCILLATING;
            app.G = 40.0f;
            app.numParticles = 40000;
            break;
        case PRESET_MOLECULAR:
            app.distribution = DIST_RANDOM;
            app.massMode = MASS_UNIFORM;
            app.gravityType = GRAVITY_LENNARD_JONES;
            app.G = 20.0f;
            app.numParticles = 20000;
            app.softening = 8.0f;
            break;
        case PRESET_EXPANSION:
            app.distribution = DIST_SPHERE;
            app.massMode = MASS_UNIFORM;
            app.gravityType = GRAVITY_NEWTONIAN;  // Normal gravity
            app.G = 30.0f;
            app.numParticles = 50000;
            // Enable dark energy expansion (competes with gravity)
            app.expansionEnabled = true;
            app.hubbleConstant = 0.03f;  // Strong expansion
            app.relativityEnabled = true;
            break;
        case PRESET_PLUMMER:
            // Self-gravitating cluster in virial equilibrium - a stability
            // showcase for the velocity Verlet integrator (watch dE on the
            // instruments panel stay flat)
            app.distribution = DIST_PLUMMER;
            app.massMode = MASS_UNIFORM;
            app.particleMode = 0;
            app.gravityType = GRAVITY_NEWTONIAN;
            app.G = 30.0f;
            app.softening = 5.0f;
            app.numParticles = 30000;
            app.damping = 0.0f;
            break;
        case PRESET_FIGURE8:
            // The Chenciner-Montgomery choreography: three equal bodies
            // eternally chasing each other around a figure-8
            app.distribution = DIST_FIGURE8;
            app.massMode = MASS_UNIFORM;
            app.particleMode = 0;
            app.gravityType = GRAVITY_NEWTONIAN;
            app.G = 30.0f;
            app.softening = 1.0f;
            app.numParticles = 20000;
            app.damping = 0.0f;
            app.timeScale = 2.0f;
            app.trailsEnabled = true;
            break;
        case PRESET_ACCRETION:
            // Planet formation: Keplerian rubble disk + bound-pair merging
            app.distribution = DIST_ACCRETION;
            app.massMode = MASS_UNIFORM;
            app.particleMode = 0;
            app.gravityType = GRAVITY_NEWTONIAN;
            app.G = 80.0f;
            app.softening = 2.0f;
            app.numParticles = 40000;
            app.damping = 0.0f;
            app.mergeEnabled = true;
            break;
        case PRESET_QUASAR:
            // A rapidly spinning supermassive hole devouring its gas disk:
            // frame dragging swirls the inflow, the shadow grows as it feeds
            app.distribution = DIST_QUASAR;
            app.massMode = MASS_UNIFORM;
            app.particleMode = 0;
            app.gravityType = GRAVITY_NEWTONIAN;
            app.G = 60.0f;
            app.softening = 2.0f;
            app.numParticles = 60000;
            app.damping = 0.0f;
            app.presetBHs.push_back({0, 0, 0, 0, 0, 0, 60000.0f, 0.9f, 0.0f});
            break;
        case PRESET_BINARY_BH:
            // Two heavy holes in mutual orbit inside a star cloud: dynamical
            // friction drags them together until one devours the other
            app.distribution = DIST_HALO;
            app.massMode = MASS_UNIFORM;
            app.particleMode = 0;
            app.gravityType = GRAVITY_NEWTONIAN;
            app.G = 60.0f;
            app.softening = 3.0f;
            app.numParticles = 30000;
            app.centralMass = 40000.0f;   // stars orbit the pair's combined mass
            app.damping = 0.0f;
            // Higher c shrinks the capture radii (~1/c^2) so the pair sweeps
            // through the halo without vacuuming it up before the inspiral
            app.speedOfLight = 600.0f;
            app.presetBHs.push_back({ 150, 0, 0,  0,  45, 0, 20000.0f,  0.5f, 0.0f});
            app.presetBHs.push_back({-150, 0, 0,  0, -45, 0, 20000.0f, -0.5f, 0.0f});
            break;
        case PRESET_TIDAL:
            // Tidal disruption event: a compact cluster plunges past a
            // massive hole and is shredded into accretion streams
            app.distribution = DIST_TIDAL;
            app.massMode = MASS_UNIFORM;
            app.particleMode = 0;
            app.gravityType = GRAVITY_NEWTONIAN;
            app.G = 50.0f;
            app.softening = 2.0f;
            app.numParticles = 30000;
            app.damping = 0.0f;
            app.trailsEnabled = true;
            app.presetBHs.push_back({0, 0, 0, 0, 0, 0, 100000.0f, 0.6f, 0.0f});
            break;
        case PRESET_BH_SWARM:
            // Six spinning holes loose in a star cloud: competitive accretion
            // and hole-eats-hole until a single survivor remains
            app.distribution = DIST_HALO;
            app.massMode = MASS_UNIFORM;
            app.particleMode = 0;
            app.gravityType = GRAVITY_NEWTONIAN;
            app.G = 50.0f;
            app.softening = 3.0f;
            app.numParticles = 50000;
            app.centralMass = 48000.0f;   // cloud orbits the swarm's total mass
            app.damping = 0.0f;
            for (int k = 0; k < 6; k++) {
                float a = (float)k / 6.0f * 6.2831853f;
                float r = 140.0f + 25.0f * (k % 3);
                float v = sqrtf(50.0f * 48000.0f / r) * 0.75f;
                app.presetBHs.push_back({r * cosf(a), r * sinf(a), (k % 2) ? 25.0f : -25.0f,
                                         -sinf(a) * v, cosf(a) * v, 0,
                                         8000.0f, (k % 2) ? 0.8f : -0.8f, 0.0f});
            }
            break;
        case PRESET_PLASMA_STORM:
            // A heavy neutral spinning hole anchors an ionized cloud (lens +
            // frame dragging + feeding), while two LIGHT charged holes orbit
            // through it. Their mass is tuned so Coulomb force ~ gravity at
            // every range (this engine's EM is mass-independent), so each
            // node openly sorts the storm: the -50 node gathers a red proton
            // swarm, the +50 node a blue electron one. Charge coloring makes
            // the segregation unmistakable.
            app.distribution = DIST_QUASAR;   // near-Keplerian disk: feeds
            app.massMode = MASS_UNIFORM;      // steadily instead of plunging
            app.particleMode = 2;             // ...and ionized (see initParticles)
            app.gravityType = GRAVITY_NEWTONIAN;
            app.emEnabled = true;
            app.emStrength = 1000.0f;         // node EM parity with node gravity
            app.colorMode = COLOR_CHARGE;     // red +, blue -
            app.G = 50.0f;
            app.softening = 5.0f;             // caps close-range Coulomb kicks
            app.numParticles = 30000;
            app.damping = 0.01f;              // bleeds off fluctuation heating
            app.trailsEnabled = true;
            app.presetBHs.push_back({0, 0, 0, 0, 0, 0, 25000.0f, 0.8f, 0.0f});
            // Charged nodes ride prograde inside the disk plane
            app.presetBHs.push_back({ 170, 0, 0,  0,  86, 0, 1000.0f, 0.0f,  50.0f});
            app.presetBHs.push_back({-170, 0, 0,  0, -86, 0, 1000.0f, 0.0f, -50.0f});
            break;
        case PRESET_PULSAR:
            // A strobing neutron star whose beamed wind sweeps twin lighthouse
            // cones through a surrounding cloud, carving rotating spiral
            // shells. It accretes what falls in - overfeed it past the mass
            // limit and it collapses into a black hole before your eyes.
            app.distribution = DIST_HALO;
            app.massMode = MASS_UNIFORM;
            app.particleMode = 0;
            app.gravityType = GRAVITY_NEWTONIAN;
            app.G = 50.0f;
            app.softening = 4.0f;
            app.numParticles = 40000;
            app.centralMass = 12000.0f;   // cloud orbits the pulsar
            app.damping = 0.0f;
            app.trailsEnabled = true;
            app.colorMode = COLOR_VELOCITY;  // beam-swept gas lights up
            app.presetBHs.push_back({0, 0, 0, 0, 0, 0, 12000.0f, 1.0f, 0.0f,
                                     PTYPE_PULSAR});
            break;
    }
    // Scale the preset's requested population to this device's tier
    // (flagship GPUs run full scale; integrated graphics get a lighter cut)
    app.numParticles = std::clamp((int)(app.numParticles * app.perfScale),
                                  1000, app.maxParticlesCap);
    app.menuDirty = true;
}

// Forward declaration
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

// Calculate center of mass of all particles (returns success)
// Uses persistent readback buffer — no per-call Vulkan alloc/free
bool getCenterOfMass(float& comX, float& comY, float& comZ, float* outBoundingRadius) {
    if (!app.particleBuffer || app.numParticles == 0 || !app.device) return false;
    if (!app.commandPool || !app.graphicsQueue || !app.readbackBuffer) return false;

    int sampleCount = std::min(app.numParticles, 50000);
    VkDeviceSize bufSize = sampleCount * sizeof(Particle);

    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo cmdAlloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmdAlloc.commandPool = app.commandPool;
    cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAlloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(app.device, &cmdAlloc, &cmd);

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);
    VkBufferCopy copy{0, 0, bufSize};
    vkCmdCopyBuffer(cmd, app.particleBuffer, app.readbackBuffer, 1, &copy);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(app.graphicsQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(app.graphicsQueue);

    void* data;
    vkMapMemory(app.device, app.readbackMemory, 0, bufSize, 0, &data);
    Particle* particles = (Particle*)data;

    float totalMass = 0.0f;
    comX = comY = comZ = 0.0f;
    for (int i = 0; i < sampleCount; i++) {
        float mass = particles[i].mass;
        comX += particles[i].x * mass;
        comY += particles[i].y * mass;
        comZ += particles[i].z * mass;
        totalMass += mass;
    }
    if (totalMass > 0.0f) {
        comX /= totalMass;
        comY /= totalMass;
        comZ /= totalMass;
    }

    if (outBoundingRadius) {
        std::vector<float> distances(sampleCount);
        for (int i = 0; i < sampleCount; i++) {
            float dx = particles[i].x - comX;
            float dy = particles[i].y - comY;
            float dz = particles[i].z - comZ;
            distances[i] = sqrtf(dx*dx + dy*dy + dz*dz);
        }
        std::sort(distances.begin(), distances.end());
        int idx95 = (int)(sampleCount * 0.95f);
        *outBoundingRadius = distances[std::min(idx95, sampleCount - 1)];
        *outBoundingRadius = std::max(*outBoundingRadius, 50.0f);
    }

    vkUnmapMemory(app.device, app.readbackMemory);
    vkFreeCommandBuffers(app.device, app.commandPool, 1, &cmd);

    return true;
}

void spawnBlackHole(float worldX, float worldY, float worldZ,
                    float velX, float velY, float velZ) {
    if (app.numParticles >= app.maxParticlesCap) return;

    vkDeviceWaitIdle(app.device);

    Particle bh;
    bh.x = worldX; bh.y = worldY; bh.z = worldZ;
    bh.mass = app.blackHoleMass;
    bh.vx = velX; bh.vy = velY; bh.vz = velZ;
    bh.charge = app.blackHoleCharge;
    bh.ptype = (float)PTYPE_BLACKHOLE;
    bh.r = app.blackHoleSpin;   // spin rides in color.r (holes emit no light)
    bh.g = 0; bh.b = 0;
    app.bhMaybePresent = true;
    // Immediate render entry; the per-frame GPU channel takes over next frame
    app.blackHoles.push_back({worldX, worldY, worldZ, bh.mass,
                              app.numParticles, bh.charge, app.blackHoleSpin,
                              PTYPE_BLACKHOLE});

    // Give the new particle a fresh trail row (recycle one freed by merges)
    int32_t row;
    if (!app.freeTrailRows.empty()) {
        row = app.freeTrailRows.back();
        app.freeTrailRows.pop_back();
    } else {
        row = (app.trailRowsAllocated < app.maxParticlesCap) ? app.trailRowsAllocated++ : 0;
    }
    if (app.trailMemory) {
        void* tdata;
        vkMapMemory(app.device, app.trailMemory,
                    (VkDeviceSize)row * app.trailLength * sizeof(TrailPoint),
                    (VkDeviceSize)app.trailLength * sizeof(TrailPoint), 0, &tdata);
        TrailPoint* tp = (TrailPoint*)tdata;
        for (int j = 0; j < app.trailLength; j++) tp[j] = {0.0f, 0.0f, 0.0f, -1.0f};
        vkUnmapMemory(app.device, app.trailMemory);
    }
    app.trailRowMap.push_back(row);
    syncTrailRowMap();

    // Write to staging buffer at the new particle offset
    void* data;
    VkDeviceSize offset = app.numParticles * sizeof(Particle);
    vkMapMemory(app.device, app.stagingMemory, offset, sizeof(Particle), 0, &data);
    memcpy(data, &bh, sizeof(Particle));
    vkUnmapMemory(app.device, app.stagingMemory);

    // Copy from staging to particle buffer
    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = app.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(app.device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);
    VkBufferCopy copy{offset, offset, sizeof(Particle)};
    vkCmdCopyBuffer(cmd, app.stagingBuffer, app.particleBuffer, 1, &copy);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(app.graphicsQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(app.graphicsQueue);
    vkFreeCommandBuffers(app.device, app.commandPool, 1, &cmd);

    app.numParticles++;
}

// Center camera on particle center of mass with auto-zoom to frame all particles
// Uses persistent readback buffer — no per-call Vulkan alloc/free
void centerCameraOnParticles() {
    if (!app.particleBuffer || app.numParticles == 0 || !app.device) return;
    if (!app.commandPool || !app.graphicsQueue || !app.readbackBuffer) return;

    VkDeviceSize bufSize = app.numParticles * sizeof(Particle);

    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo cmdAlloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmdAlloc.commandPool = app.commandPool;
    cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAlloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(app.device, &cmdAlloc, &cmd);

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);
    VkBufferCopy copy{0, 0, bufSize};
    vkCmdCopyBuffer(cmd, app.particleBuffer, app.readbackBuffer, 1, &copy);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(app.graphicsQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(app.graphicsQueue);

    void* data;
    vkMapMemory(app.device, app.readbackMemory, 0, bufSize, 0, &data);
    Particle* particles = (Particle*)data;

    float totalMass = 0.0f;
    float comX = 0.0f, comY = 0.0f, comZ = 0.0f;
    for (int i = 0; i < app.numParticles; i++) {
        float mass = particles[i].mass;
        comX += particles[i].x * mass;
        comY += particles[i].y * mass;
        comZ += particles[i].z * mass;
        totalMass += mass;
    }
    if (totalMass > 0.0f) {
        comX /= totalMass;
        comY /= totalMass;
        comZ /= totalMass;
    }

    std::vector<float> distances(app.numParticles);
    for (int i = 0; i < app.numParticles; i++) {
        float dx = particles[i].x - comX;
        float dy = particles[i].y - comY;
        float dz = particles[i].z - comZ;
        distances[i] = sqrtf(dx*dx + dy*dy + dz*dz);
    }

    vkUnmapMemory(app.device, app.readbackMemory);
    vkFreeCommandBuffers(app.device, app.commandPool, 1, &cmd);

    std::sort(distances.begin(), distances.end());
    int idx95 = (int)(app.numParticles * 0.95f);
    float boundingRadius = distances[std::min(idx95, app.numParticles - 1)];
    boundingRadius = std::max(boundingRadius, 50.0f);

    float viewDistance = boundingRadius * 2.5f;
    app.camX = comX;
    app.camY = comY;
    app.camZ = comZ - viewDistance;
    app.camRotX = 0.0f;
    app.camRotY = 0.0f;
    app.camZoom = 0.7f * 0.577f * viewDistance / boundingRadius;
    app.camZoom = std::max(app.camZoom, 1.0f);

    printf("Centered on particles: COM=(%.1f, %.1f, %.1f), radius=%.1f, zoom=%.2f\n",
           comX, comY, comZ, boundingRadius, app.camZoom);
}

void mortonSort(std::vector<Particle>& parts, std::vector<int>* outPerm);  // defined with the spatial sort pass

// Copy the first `count` particles from the GPU into `out`.
// Uses the persistent readback buffer; waits for the transfer.
bool readbackParticles(std::vector<Particle>& out, int count) {
    if (!app.particleBuffer || count <= 0 || !app.readbackBuffer) return false;
    out.resize(count);
    VkDeviceSize bufSize = (VkDeviceSize)count * sizeof(Particle);

    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo cmdAlloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmdAlloc.commandPool = app.commandPool;
    cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAlloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(app.device, &cmdAlloc, &cmd);

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);
    VkBufferCopy copy{0, 0, bufSize};
    vkCmdCopyBuffer(cmd, app.particleBuffer, app.readbackBuffer, 1, &copy);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(app.graphicsQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(app.graphicsQueue);

    void* data;
    vkMapMemory(app.device, app.readbackMemory, 0, bufSize, 0, &data);
    memcpy(out.data(), data, bufSize);
    vkUnmapMemory(app.device, app.readbackMemory);
    vkFreeCommandBuffers(app.device, app.commandPool, 1, &cmd);
    return true;
}

// Upload a full particle array to the GPU and reset the Verlet acceleration
// history (indices changed, so the stored accelerations are meaningless).
void uploadParticles(const std::vector<Particle>& particles) {
    // Wait for ALL GPU work to complete before modifying buffers
    vkDeviceWaitIdle(app.device);

    size_t count = particles.size();
    void* data;
    vkMapMemory(app.device, app.stagingMemory, 0, count * sizeof(Particle), 0, &data);
    memcpy(data, particles.data(), count * sizeof(Particle));
    vkUnmapMemory(app.device, app.stagingMemory);

    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = app.computeCommandPool;  // Use compute pool for compute queue
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(app.device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferCopy copy{0, 0, (VkDeviceSize)(count * sizeof(Particle))};
    vkCmdCopyBuffer(cmd, app.stagingBuffer, app.particleBuffer, 1, &copy);
    vkCmdFillBuffer(cmd, app.accBuffer, 0, (VkDeviceSize)app.maxParticlesCap * 4 * sizeof(float), 0);

    // Memory barrier to ensure the copy is visible to compute and graphics
    VkMemoryBarrier memBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        0, 1, &memBarrier, 0, nullptr, 0, nullptr);

    vkEndCommandBuffer(cmd);
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    // Submit to compute queue (ensures visibility for compute shader)
    vkQueueSubmit(app.computeQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(app.computeQueue);

    // Also ensure graphics queue sees the update (for shared buffer)
    if (app.graphicsQueue != app.computeQueue) {
        vkQueueWaitIdle(app.graphicsQueue);
    }

    vkFreeCommandBuffers(app.device, app.computeCommandPool, 1, &cmd);
}

// Push the CPU trail row map to its GPU buffer
void syncTrailRowMap() {
    if (!app.trailRowMapMemory || app.trailRowMap.empty()) return;
    void* data;
    vkMapMemory(app.device, app.trailRowMapMemory, 0,
                app.trailRowMap.size() * sizeof(int32_t), 0, &data);
    memcpy(data, app.trailRowMap.data(), app.trailRowMap.size() * sizeof(int32_t));
    vkUnmapMemory(app.device, app.trailRowMapMemory);
}

// Full trail reset: identity row map, every row marked unwritten
void resetTrails(const std::vector<Particle>& particles) {
    if (!app.trailMemory) return;
    size_t n = particles.size();
    size_t count = n * app.trailLength;
    void* data;
    vkMapMemory(app.device, app.trailMemory, 0, count * sizeof(TrailPoint), 0, &data);
    TrailPoint* tp = (TrailPoint*)data;
    for (size_t i = 0; i < count; i++) tp[i] = {0.0f, 0.0f, 0.0f, -1.0f};
    vkUnmapMemory(app.device, app.trailMemory);
    app.trailHead = 0;

    app.trailRowMap.resize(n);
    for (size_t i = 0; i < n; i++) app.trailRowMap[i] = (int32_t)i;
    app.freeTrailRows.clear();
    app.trailRowsAllocated = (int)n;
    syncTrailRowMap();
}

void initParticles() {
    std::vector<Particle> particles(app.numParticles);
    float radius = 200.0f;

    // Scale central mass proportional to particle count
    // Base: 5000 mass for 30000 particles = 0.167 per particle
    const int baseParticleCount = 30000;
    app.centralMass = app.baseCentralMass * ((float)app.numParticles / (float)baseParticleCount);

    // These distributions fully determine positions, velocities AND masses;
    // the generic mass-mode / orbital-velocity / perturbation stages would
    // destroy their carefully constructed initial conditions.
    bool distSetsEverything = (app.distribution == DIST_PLUMMER ||
                               app.distribution == DIST_FIGURE8 ||
                               app.distribution == DIST_ACCRETION ||
                               app.distribution == DIST_QUASAR ||
                               app.distribution == DIST_TIDAL ||
                               app.distribution == DIST_HALO);

    for (int i = 0; i < app.numParticles; i++) {
        Particle& p = particles[i];
        p.charge = 0.0f;  // Will be set later based on chargeRatio
        p.ptype = 0.0f;   // Normal matter by default

        float px = 0, py = 0, pz = 0;
        float vx = 0, vy = 0, vz = 0;
        float mass = 1.0f;

        switch (app.distribution) {
            case DIST_SPHERE: {
                float r = radius * cbrtf(randFloat());
                float ux, uy, uz; randUnitVector(ux, uy, uz);
                px = r * ux; py = r * uy; pz = r * uz;
                break;
            }
            case DIST_DISK: {
                float r = radius * sqrtf(randFloat());
                float theta = randFloat() * 2.0f * M_PI;
                px = r * cosf(theta); py = r * sinf(theta);
                pz = randGaussian() * 3.0f;
                break;
            }
            case DIST_SHELL: {
                float ux, uy, uz; randUnitVector(ux, uy, uz);
                px = radius * ux; py = radius * uy; pz = radius * uz;
                break;
            }
            case DIST_RANDOM:
                px = randRange(-radius, radius);
                py = randRange(-radius, radius);
                pz = randRange(-radius, radius);
                break;
            case DIST_BINARY_GALAXIES: {
                float offset = (i < app.numParticles / 2) ? -120.0f : 120.0f;
                float r = 80.0f * sqrtf(randFloat());
                float theta = randFloat() * 2.0f * M_PI;
                px = offset + r * cosf(theta);
                py = r * sinf(theta);
                pz = randGaussian() * 2.0f;
                break;
            }
            case DIST_SOLAR_SYSTEM: {
                if (i == 0) {
                    px = py = pz = 0;
                    mass = app.centralMass;
                } else {
                    float r = 30.0f + sqrtf(randFloat()) * 200.0f;
                    float theta = randFloat() * 2.0f * M_PI;
                    px = r * cosf(theta); py = r * sinf(theta);
                    pz = randGaussian() * 1.0f;
                }
                break;
            }
            case DIST_RING: {
                float r = 120.0f + randGaussian() * 20.0f;
                float theta = randFloat() * 2.0f * M_PI;
                px = r * cosf(theta); py = r * sinf(theta);
                pz = randGaussian() * 2.0f;
                break;
            }
            case DIST_CLUSTER: {
                int cluster = rand() % 5;
                float cx = (cluster % 3 - 1) * 150.0f;
                float cy = (cluster / 3 - 0.5f) * 150.0f;
                float r = 40.0f * cbrtf(randFloat());
                float ux, uy, uz; randUnitVector(ux, uy, uz);
                px = cx + r * ux; py = cy + r * uy; pz = r * uz;
                break;
            }
            case DIST_FILAMENT: {
                float t = randFloat();
                float spine = (t - 0.5f) * 400.0f;
                float angle = t * 4.0f * M_PI;
                float r = 30.0f * sqrtf(randFloat());
                px = spine;
                py = cosf(angle) * 50.0f + randGaussian() * r;
                pz = sinf(angle) * 50.0f + randGaussian() * r;
                break;
            }
            case DIST_EXPLOSION: {
                float r = 10.0f + randFloat() * 5.0f;
                float ux, uy, uz; randUnitVector(ux, uy, uz);
                px = r * ux; py = r * uy; pz = r * uz;
                float speed = 50.0f + randFloat() * 100.0f;
                vx = ux * speed; vy = uy * speed; vz = uz * speed;
                break;
            }
            case DIST_VORTEX: {
                float r = radius * sqrtf(randFloat());
                float theta = randFloat() * 2.0f * M_PI;
                px = r * cosf(theta); py = r * sinf(theta);
                pz = randGaussian() * 10.0f;
                float tangent = sqrtf(app.G * 5000.0f / (r + 10.0f));
                vx = -sinf(theta) * tangent;
                vy = cosf(theta) * tangent;
                vz = randGaussian() * 2.0f;
                break;
            }
            case DIST_GRID: {
                int side = (int)cbrtf(app.numParticles) + 1;
                int ix = i % side;
                int iy = (i / side) % side;
                int iz = i / (side * side);
                float spacing = 400.0f / side;
                px = (ix - side/2) * spacing;
                py = (iy - side/2) * spacing;
                pz = (iz - side/2) * spacing;
                break;
            }
            case DIST_PLUMMER: {
                // Plummer sphere in virial equilibrium: radius from the exact
                // inverse CDF, speed rejection-sampled from the isotropic
                // distribution function g(q) ~ q^2 (1-q^2)^(7/2), q = v/v_esc.
                float a = radius * 0.35f;  // Plummer scale radius
                float u = std::min(std::max(randFloat(), 1e-4f), 0.999f);
                float r = a / sqrtf(powf(u, -2.0f / 3.0f) - 1.0f);
                r = std::min(r, a * 8.0f);
                float ux, uy, uz; randUnitVector(ux, uy, uz);
                px = r * ux; py = r * uy; pz = r * uz;

                float M = (float)app.numParticles;  // unit masses
                float psi = app.G * M / sqrtf(r * r + a * a);
                float vesc = sqrtf(2.0f * psi);
                float q = 0.0f;
                for (int tries = 0; tries < 100; tries++) {
                    float qq = randFloat();
                    float g = qq * qq * powf(std::max(1.0f - qq * qq, 0.0f), 3.5f);
                    if (randFloat() * 0.093f < g) { q = qq; break; }  // 0.093 > max(g)
                }
                float speed = q * vesc;
                float vux, vuy, vuz; randUnitVector(vux, vuy, vuz);
                vx = speed * vux; vy = speed * vuy; vz = speed * vuz;
                mass = 1.0f;
                break;
            }
            case DIST_FIGURE8: {
                // Chenciner-Montgomery figure-8 choreography (G=1, m=1 units,
                // rescaled): three equal bodies chase each other around a
                // figure-8, plus a far tracer dust ring that feels the chaos.
                const float L = 130.0f;    // length scale
                const float M8 = 1000.0f;  // per-body mass (closest approach
                                           // stays below the shader force clamp)
                float vscale = sqrtf(app.G * M8 / L);
                if (i < 3) {
                    const float x1 = 0.97000436f, y1 = -0.24308753f;
                    const float v3x = 0.93240737f, v3y = 0.86473146f;
                    if (i == 0) { px =  x1 * L; py =  y1 * L; vx = -0.5f * v3x * vscale; vy = -0.5f * v3y * vscale; }
                    if (i == 1) { px = -x1 * L; py = -y1 * L; vx = -0.5f * v3x * vscale; vy = -0.5f * v3y * vscale; }
                    if (i == 2) { px = 0.0f;    py = 0.0f;    vx =  v3x * vscale;        vy =  v3y * vscale; }
                    pz = 0.0f; vz = 0.0f;
                    mass = M8;
                } else {
                    float r = L * (3.0f + 3.0f * sqrtf(randFloat()));
                    float theta = randFloat() * 2.0f * (float)M_PI;
                    px = r * cosf(theta); py = r * sinf(theta);
                    pz = randGaussian() * L * 0.05f;
                    float v = sqrtf(app.G * 3.0f * M8 / r);
                    vx = -sinf(theta) * v; vy = cosf(theta) * v; vz = 0.0f;
                    mass = 0.005f;
                }
                break;
            }
            case DIST_ACCRETION: {
                // Protoplanetary disk: central star + planetesimals on
                // near-circular Keplerian orbits with a log-uniform radial
                // profile (surface density ~ 1/r). Pairs beautifully with
                // accretion merging (V) - watch planets form.
                if (i == 0) {
                    px = py = pz = 0.0f; vx = vy = vz = 0.0f;
                    // Star dominates the disk (~25% disk-to-star mass ratio)
                    // so accretion proceeds by local clumping, not violent
                    // disk fragmentation
                    mass = app.centralMass * 2.0f;
                } else {
                    float rin = 35.0f, rout = 280.0f;
                    float r = rin * powf(rout / rin, randFloat());
                    float theta = randFloat() * 2.0f * (float)M_PI;
                    px = r * cosf(theta); py = r * sinf(theta);
                    pz = randGaussian() * (0.5f + r * 0.01f);
                    float v = sqrtf(app.G * app.centralMass * 2.0f / r);
                    vx = -sinf(theta) * v * (1.0f + randGaussian() * 0.01f);
                    vy =  cosf(theta) * v * (1.0f + randGaussian() * 0.01f);
                    vz = randGaussian() * v * 0.005f;
                    mass = 0.02f + powf(randFloat(), 2.0f) * 0.2f;
                }
                break;
            }
            case DIST_QUASAR: {
                // Gas disk feeding a preset central black hole: log-uniform
                // radii, Keplerian speeds for the hole's mass, thin and hot
                float bhM = app.presetBHs.empty() ? app.centralMass * 3.0f
                                                  : app.presetBHs[0].mass;
                // Inner edge sits safely outside the hole's capture radius
                float rcap = 2.0f * app.G * bhM /
                             std::max(app.speedOfLight * app.speedOfLight, 1.0f);
                float rin = std::max(45.0f, rcap * 1.25f), rout = 340.0f;
                float r = rin * powf(rout / rin, randFloat());
                float theta = randFloat() * 2.0f * (float)M_PI;
                px = r * cosf(theta); py = r * sinf(theta);
                pz = randGaussian() * (0.6f + r * 0.012f);
                // Slightly sub-Keplerian: the disk steadily drains inward,
                // so the hole visibly feeds and its shadow grows
                float v = sqrtf(app.G * bhM / r) * 0.93f;
                vx = -sinf(theta) * v * (1.0f + randGaussian() * 0.012f);
                vy =  cosf(theta) * v * (1.0f + randGaussian() * 0.012f);
                vz = randGaussian() * v * 0.006f;
                mass = 0.03f + powf(randFloat(), 2.0f) * 0.15f;
                break;
            }
            case DIST_TIDAL: {
                // Compact star cluster on a plunging trajectory past a preset
                // massive hole at the origin - it gets shredded into streams
                float r = 30.0f * cbrtf(randFloat());
                float ux, uy, uz; randUnitVector(ux, uy, uz);
                px = 380.0f + r * ux; py = 60.0f + r * uy; pz = r * uz;
                float vdisp = 8.0f;
                vx = -60.0f + randGaussian() * vdisp;
                vy = -58.0f + randGaussian() * vdisp;
                vz = randGaussian() * vdisp * 0.5f;
                mass = 0.05f;
                break;
            }
            case DIST_HALO: {
                // Spherical cloud on TRUE circular orbits in randomized
                // planes around app.centralMass (the compact objects a preset
                // places at the center). Unlike the legacy sphere's 0.8x
                // fudge, nothing plunges - central holes nibble instead of
                // devouring the whole cloud.
                float r = std::max(45.0f, radius * cbrtf(randFloat()));
                float ux, uy, uz; randUnitVector(ux, uy, uz);
                px = r * ux; py = r * uy; pz = r * uz;
                mass = 0.1f;
                float mEnc = app.centralMass +
                             0.1f * (float)app.numParticles * (r / radius) * (r / radius) * (r / radius);
                float vc = sqrtf(app.G * mEnc / r);
                float tx, ty, tz; randUnitVector(tx, ty, tz);
                // tangential direction: component of the random vector
                // perpendicular to the radius
                float dp = tx * ux + ty * uy + tz * uz;
                tx -= dp * ux; ty -= dp * uy; tz -= dp * uz;
                float tl = sqrtf(tx * tx + ty * ty + tz * tz);
                if (tl < 1e-4f) { tx = -uy; ty = ux; tz = 0.0f; tl = sqrtf(tx * tx + ty * ty) + 1e-6f; }
                vx = vc * tx / tl; vy = vc * ty / tl; vz = vc * tz / tl;
                break;
            }
        }

        // Mass assignment based on mode (skipped when the distribution
        // fully specifies masses)
        if (distSetsEverything) {
            // keep the distribution-assigned mass
        } else if (app.massMode == MASS_CENTRAL && i == 0) {
            px = py = pz = 0; mass = app.centralMass;
        } else if (app.massMode == MASS_BINARY && i < 2) {
            px = (i == 0) ? -100.0f : 100.0f; py = pz = 0;
            mass = app.centralMass / 2.0f;
            vy = (i == 0) ? sqrtf(app.G * mass / 200.0f) * 0.7f : -sqrtf(app.G * mass / 200.0f) * 0.7f;
        }

        // Prevent particles from spawning inside central massive bodies
        // Push them outward to a minimum radius
        float minRadius = 0.0f;
        if (!distSetsEverything && app.massMode == MASS_CENTRAL && i > 0) {
            // Minimum radius from central mass (scales with mass)
            minRadius = 15.0f + sqrtf(app.centralMass) * 0.5f;
            float d = sqrtf(px*px + py*py + pz*pz);
            if (d < minRadius && d > 0.01f) {
                float scale = minRadius / d;
                px *= scale; py *= scale; pz *= scale;
            } else if (d < 0.01f) {
                // Particle at exact center - push outward randomly
                float ux, uy, uz; randUnitVector(ux, uy, uz);
                px = ux * minRadius; py = uy * minRadius; pz = uz * minRadius;
            }
        } else if (!distSetsEverything && app.massMode == MASS_BINARY && i >= 2) {
            // Minimum radius from each binary star
            minRadius = 10.0f + sqrtf(app.centralMass / 2.0f) * 0.3f;
            float d0 = sqrtf((px+100)*(px+100) + py*py + pz*pz);
            float d1 = sqrtf((px-100)*(px-100) + py*py + pz*pz);
            if (d0 < minRadius) {
                float scale = minRadius / d0;
                px = -100.0f + (px + 100.0f) * scale;
                py *= scale; pz *= scale;
            }
            if (d1 < minRadius) {
                float scale = minRadius / d1;
                px = 100.0f + (px - 100.0f) * scale;
                py *= scale; pz *= scale;
            }
        }

        if (distSetsEverything) {
            // keep the distribution-assigned mass
        } else if ((app.massMode == MASS_CENTRAL && i == 0) ||
                   (app.massMode == MASS_BINARY && i < 2) ||
                   (app.distribution == DIST_SOLAR_SYSTEM && i == 0)) {
            // Keep the central mass assigned above. (Historically this fell
            // through to the generic switch below, which overwrote the central
            // body back to ~0.1 mass - every preset was orbiting a phantom
            // star that did not actually exist.)
        } else if (app.massMode == MASS_HIERARCHICAL && i < 10 && i > 0) {
            mass = app.centralMass / (i + 1);
        } else if (app.massMode == MASS_CLUSTERS && (i % 500 == 0)) {
            // Every 500th particle is a cluster center
            mass = 50.0f + randFloat() * 200.0f;
        } else {
            switch (app.massMode) {
                case MASS_UNIFORM:
                    mass = 1.0f;
                    break;
                case MASS_RANDOM:
                    // Much wider range: 0.1 to 50
                    mass = 0.1f + powf(randFloat(), 2.0f) * 50.0f;
                    break;
                case MASS_CENTRAL:
                    mass = 0.1f;
                    break;
                case MASS_BINARY:
                    mass = 0.1f;
                    break;
                case MASS_HIERARCHICAL:
                    mass = 0.5f + randFloat() * 1.5f;
                    break;
                case MASS_POWER_LAW: {
                    // Salpeter-like IMF: many small masses, few large
                    // P(m) ~ m^(-2.35), so m ~ u^(-1/1.35) where u is uniform
                    float u = randFloat();
                    if (u < 0.001f) u = 0.001f; // Prevent division issues
                    mass = 0.1f * powf(u, -0.74f); // Inverse power law
                    mass = fminf(mass, 500.0f);    // Cap at 500
                    break;
                }
                case MASS_STARS_AND_GAS: {
                    // Bimodal: 80% gas (small), 20% stars (large)
                    if (randFloat() < 0.2f) {
                        // Star: 5 to 100
                        mass = 5.0f + randFloat() * 95.0f;
                    } else {
                        // Gas: 0.05 to 0.5
                        mass = 0.05f + randFloat() * 0.45f;
                    }
                    break;
                }
                case MASS_CLUSTERS:
                    // Regular particles between cluster centers
                    mass = 0.2f + randFloat() * 1.8f;
                    break;
                default:
                    mass = 1.0f;
            }
        }

        // Orbital velocity calculation
        if (!distSetsEverything &&
            app.distribution != DIST_EXPLOSION && app.distribution != DIST_VORTEX &&
            app.distribution != DIST_GRID && vx == 0 && vy == 0 && vz == 0) {

            // Determine the center we're orbiting around based on ACTUAL massive particle locations
            float centerX = 0, centerY = 0, centerZ = 0;
            float centerMass = app.centralMass;

            // For binary galaxies distribution: orbit around galaxy center
            // The galaxy centers ARE the gravity sources (implicit central mass)
            if (app.distribution == DIST_BINARY_GALAXIES) {
                centerX = (i < app.numParticles/2) ? -120.0f : 120.0f;
                centerMass = 5000.0f;
            }
            // For binary stars mass mode: orbit around nearest ACTUAL star at ±100
            else if (app.massMode == MASS_BINARY && i >= 2) {
                float d0 = sqrtf((px+100)*(px+100) + py*py + pz*pz);
                float d1 = sqrtf((px-100)*(px-100) + py*py + pz*pz);
                centerX = (d0 < d1) ? -100.0f : 100.0f;
                centerMass = app.centralMass / 2.0f;
            }

            // Calculate position relative to orbit center
            float relX = px - centerX;
            float relY = py - centerY;
            float relZ = pz - centerZ;

            float distXY = sqrtf(relX*relX + relY*relY);
            float orbitDist = sqrtf(relX*relX + relY*relY + relZ*relZ);

            if (orbitDist > 0.5f) {
                float orbSpeed;
                if (app.distribution == DIST_DISK && app.massMode == MASS_CENTRAL) {
                    // True rotation curve: the disk's own enclosed mass
                    // (uniform surface density: M_enc ~ r^2) adds to the
                    // central body. The old 0.8 fudge left the disk
                    // sub-circular, so it slowly collapsed and fragmented.
                    float diskMass = 0.1f * (float)app.numParticles;
                    float rNorm = std::min(orbitDist / radius, 1.0f);
                    float mEnc = centerMass + diskMass * rNorm * rNorm * 0.6f;
                    orbSpeed = sqrtf(app.G * mEnc / (orbitDist + 5.0f));
                } else {
                    orbSpeed = sqrtf(app.G * centerMass / (orbitDist + 5.0f)) * 0.8f;
                }

                // Calculate tangential velocity (perpendicular to radius in x-y plane)
                if (distXY < 1.0f) {
                    // Particle is on z-axis relative to center - random direction
                    float randAngle = randFloat() * 2.0f * M_PI;
                    vx = orbSpeed * cosf(randAngle);
                    vy = orbSpeed * sinf(randAngle);
                } else {
                    // Tangential velocity: perpendicular to line from center to particle
                    float angle = atan2f(relY, relX) + M_PI / 2.0f;
                    vx = orbSpeed * cosf(angle);
                    vy = orbSpeed * sinf(angle);
                }

                // Add small z-velocity for 3D motion
                vz = randGaussian() * orbSpeed * 0.1f;
            }
        }

        // ALWAYS add small random velocity perturbation to ALL particles (except central masses)
        // This prevents any particle from being perfectly static
        bool isCentralBody = (app.massMode == MASS_CENTRAL && i == 0) ||
                             (app.massMode == MASS_BINARY && i < 2) ||
                             (app.distribution == DIST_FIGURE8 && i < 3) ||
                             (app.distribution == DIST_ACCRETION && i == 0);
        if (!isCentralBody && !distSetsEverything) {
            vx += randGaussian() * 2.0f;
            vy += randGaussian() * 2.0f;
            vz += randGaussian() * 1.0f;
        }

        p.x = px; p.y = py; p.z = pz;
        p.vx = vx; p.vy = vy; p.vz = vz;

        // Assign particle type, charge, and mass based on particleMode
        // Central bodies (massive stars) remain generic with their assigned mass,
        // as do self-contained distributions (Plummer / Figure-8 / Accretion).
        // Exception: a quasar disk in Plasma matter mode is ionized - species
        // charges assigned, dynamics (positions/velocities) kept from the case.
        if (app.distribution == DIST_QUASAR && app.particleMode == 2) {
            if (randFloat() < 0.5f) {
                p.ptype = (float)PTYPE_ELECTRON;
                p.charge = -1.0f;
                p.mass = mass * 0.2f;    // lighter species, same disk dynamics
            } else {
                p.ptype = (float)PTYPE_PROTON;
                p.charge = 1.0f;
                p.mass = mass;
            }
        } else if (isCentralBody || distSetsEverything) {
            p.charge = 0.0f;
            p.ptype = (float)PTYPE_GENERIC;
            p.mass = mass;  // Keep the central mass
        } else {
            float r = randFloat();
            switch (app.particleMode) {
                case 0:  // Generic - all neutral, equal mass
                    p.ptype = (float)PTYPE_GENERIC;
                    p.charge = 0.0f;
                    p.mass = mass;
                    break;

                case 1:  // Atoms - electrons, protons, neutrons (equal parts)
                    if (r < 0.333f) {
                        p.ptype = (float)PTYPE_ELECTRON;
                        p.charge = -1.0f;
                        p.mass = 0.0005f;  // Much lighter than protons
                    } else if (r < 0.666f) {
                        p.ptype = (float)PTYPE_PROTON;
                        p.charge = 1.0f;
                        p.mass = 1.0f;
                    } else {
                        p.ptype = (float)PTYPE_NEUTRON;
                        p.charge = 0.0f;
                        p.mass = 1.0f;
                    }
                    break;

                case 2:  // Plasma - electrons and protons only (ionized gas)
                    if (r < 0.5f) {
                        p.ptype = (float)PTYPE_ELECTRON;
                        p.charge = -1.0f;
                        p.mass = 0.0005f;
                    } else {
                        p.ptype = (float)PTYPE_PROTON;
                        p.charge = 1.0f;
                        p.mass = 1.0f;
                    }
                    break;

                case 3:  // Quarks - up and down quarks (QCD simulation)
                    if (r < 0.5f) {
                        p.ptype = (float)PTYPE_UP_QUARK;
                        p.charge = 2.0f / 3.0f;   // +2/3 e
                        p.mass = 0.002f;
                    } else {
                        p.ptype = (float)PTYPE_DOWN_QUARK;
                        p.charge = -1.0f / 3.0f;  // -1/3 e
                        p.mass = 0.005f;
                    }
                    break;

                case 4:  // Mixed - configurable ratios
                    {
                        float cumulative = 0.0f;
                        cumulative += app.electronRatio;
                        if (r < cumulative) {
                            p.ptype = (float)PTYPE_ELECTRON;
                            p.charge = -1.0f;
                            p.mass = 0.0005f;
                        } else {
                            cumulative += app.protonRatio;
                            if (r < cumulative) {
                                p.ptype = (float)PTYPE_PROTON;
                                p.charge = 1.0f;
                                p.mass = 1.0f;
                            } else {
                                cumulative += app.neutronRatio;
                                if (r < cumulative) {
                                    p.ptype = (float)PTYPE_NEUTRON;
                                    p.charge = 0.0f;
                                    p.mass = 1.0f;
                                } else {
                                    // Remaining is quarks, split evenly
                                    if (randFloat() < 0.5f) {
                                        p.ptype = (float)PTYPE_UP_QUARK;
                                        p.charge = 2.0f / 3.0f;
                                        p.mass = 0.002f;
                                    } else {
                                        p.ptype = (float)PTYPE_DOWN_QUARK;
                                        p.charge = -1.0f / 3.0f;
                                        p.mass = 0.005f;
                                    }
                                }
                            }
                        }
                    }
                    break;

                case 5:  // Fundamental - leptons + quarks (equal parts)
                    if (r < 0.25f) {
                        p.ptype = (float)PTYPE_ELECTRON;
                        p.charge = -1.0f;
                        p.mass = 0.0005f;
                    } else if (r < 0.5f) {
                        p.ptype = (float)PTYPE_POSITRON;
                        p.charge = 1.0f;
                        p.mass = 0.0005f;
                    } else if (r < 0.75f) {
                        p.ptype = (float)PTYPE_UP_QUARK;
                        p.charge = 2.0f / 3.0f;
                        p.mass = 0.002f;
                    } else {
                        p.ptype = (float)PTYPE_DOWN_QUARK;
                        p.charge = -1.0f / 3.0f;
                        p.mass = 0.005f;
                    }
                    break;

                default:
                    p.ptype = (float)PTYPE_GENERIC;
                    p.charge = 0.0f;
                    p.mass = mass;
                    break;
            }
        }

        // Color (will be overridden by shader based on color mode)
        hsv2rgb(randFloat(), 0.8f, 1.0f, p.r, p.g, p.b);
    }

    // Black holes placed by the active preset join the population
    int firstBHIndex = (int)particles.size();
    for (const App::PresetBH& pb : app.presetBHs) {
        if ((int)particles.size() >= app.maxParticlesCap) break;
        Particle b{};
        b.x = pb.x; b.y = pb.y; b.z = pb.z; b.mass = pb.mass;
        b.vx = pb.vx; b.vy = pb.vy; b.vz = pb.vz;
        b.charge = pb.charge;
        b.ptype = (float)pb.ptype;
        b.r = pb.spin; b.g = 0.0f; b.b = 0.0f;
        particles.push_back(b);
    }
    app.numParticles = (int)particles.size();

    // Large sims lean on the far-field solver, which needs spatially
    // compact tiles from the very first frame. Preset holes are appended
    // AFTER the sort so their gids stay known for immediate rendering.
    if (app.numParticles > 128 * WORKGROUP_SIZE) {
        // sort only the non-hole prefix, keep holes at the tail
        std::vector<Particle> holes(particles.begin() + firstBHIndex, particles.end());
        particles.resize(firstBHIndex);
        mortonSort(particles);
        particles.insert(particles.end(), holes.begin(), holes.end());
    }

    uploadParticles(particles);
    resetTrails(particles);

    // Fresh run: reset diagnostics baseline, accretion counters, black holes
    app.energyBaselineSet = false;
    app.energyHistory.clear();
    app.diag.valid = false;
    app.mergeCount = 0;
    app.gwSamples = 0;
    app.gwSmoothP = app.gwSmoothX = 0;
    app.gwPlus.clear();
    app.gwCross.clear();
    app.blackHoles.clear();
    for (size_t i = 0; i < app.presetBHs.size(); i++) {
        const App::PresetBH& pb = app.presetBHs[i];
        app.blackHoles.push_back({pb.x, pb.y, pb.z, pb.mass,
                                  firstBHIndex + (int)i, pb.charge, pb.spin,
                                  pb.ptype});
    }
    app.bhMaybePresent = !app.blackHoles.empty();

    app.simTime = 0.0f;
    app.menuDirty = true;
}

// Write the simulation UBO with an explicit timestep (runCompute passes the
// substep dt; the render path passes the full frame dt for shader consistency)
void writeSimUBO(float stepDt) {
    SimUBO sim{};
    sim.G = app.G;
    sim.softening = app.softening;
    sim.dt = stepDt;
    sim.numParticles = app.numParticles;
    sim.gravityType = app.gravityType;
    sim.darkMatterStrength = app.darkMatterStrength;
    sim.darkMatterEnabled = app.darkMatter ? 1 : 0;
    sim.simTime = app.simTime;
    sim.damping = app.damping;
    sim.centralMass = app.centralMass;
    sim.boundaryRadius = app.boundaryRadius;
    sim.boundaryMode = app.boundaryMode;
    // New force parameters
    sim.gravityEnabled = app.gravityEnabled ? 1 : 0;
    sim.emEnabled = app.emEnabled ? 1 : 0;
    sim.strongForceEnabled = app.strongForceEnabled ? 1 : 0;
    sim.emStrength = app.emStrength;
    sim.strongStrength = app.strongStrength;
    sim.strongRange = app.strongRange;
    // Relativistic and cosmological parameters
    sim.relativityEnabled = app.relativityEnabled ? 1 : 0;
    sim.expansionEnabled = app.expansionEnabled ? 1 : 0;
    sim.speedOfLight = app.speedOfLight;
    sim.hubbleConstant = app.hubbleConstant;

    void* data;
    vkMapMemory(app.device, app.simUboMemory, 0, sizeof(SimUBO), 0, &data);
    memcpy(data, &sim, sizeof(SimUBO));
    vkUnmapMemory(app.device, app.simUboMemory);
}

void updateUniforms() {
    writeSimUBO(app.frameDeltaTime * app.timeScale);
    void* data;

    RenderUBO render{};
    render.camRotX = app.camRotX;
    render.camRotY = app.camRotY;
    render.camZoom = app.camZoom;
    render.camX = app.camX;
    render.camY = app.camY;
    render.camZ = app.camZ;
    render.screenWidth = (float)app.swapchainExtent.width;
    render.screenHeight = (float)app.swapchainExtent.height;
    render.glowIntensity = app.glowIntensity;
    render.colorMode = app.colorMode;
    render.pointScale = app.pointScale;
    render.lensingStrength = app.lensingStrength;
    render.speedOfLight = app.speedOfLight;
    render.simTime = app.simTime;

    vkMapMemory(app.device, app.renderUboMemory, 0, sizeof(RenderUBO), 0, &data);
    memcpy(data, &render, sizeof(RenderUBO));
    vkUnmapMemory(app.device, app.renderUboMemory);

    // Project tracked black holes to screen space for the composite
    // lensing/shadow pass (mirrors the particle vertex shader's camera math)
    if (app.compositeUboMemory) {
        CompositeUBO cu{};
        float cxr = cosf(app.camRotX), sxr = sinf(app.camRotX);
        float cyr = cosf(app.camRotY), syr = sinf(app.camRotY);
        float aspect = (float)app.swapchainExtent.width / (float)app.swapchainExtent.height;
        const float tanHalfFov = 0.577f;

        // Positions come straight from this frame's physics dispatch - exact
        for (const App::BHInfo& b : app.blackHoles) {
            if (cu.count >= MAX_BLACK_HOLES) break;
            if (b.type != PTYPE_BLACKHOLE) continue;   // pulsars shine, not lens
            float wx = b.x - app.camX;
            float wy = b.y - app.camY;
            float wz = b.z - app.camZ;
            float rx = wx * cyr + wz * syr;
            float ry = wy;
            float rz = -wx * syr + wz * cyr;
            float fx = rx;
            float fy = ry * cxr - rz * sxr;
            float fz = ry * sxr + rz * cxr;
            if (fz < 1.0f) continue;   // behind the camera

            float u = ((fx / fz) / (tanHalfFov * aspect) * app.camZoom) * 0.5f + 0.5f;
            float v = ((fy / fz) / tanHalfFov * app.camZoom) * 0.5f + 0.5f;
            if (u < -0.6f || u > 1.6f || v < -0.6f || v > 1.6f) continue;

            // Visual Einstein radius (in v units): scaled-down sqrt(4GM/(c^2 D)),
            // modulated by the lensing-strength control
            float thetaE = 0.25f * sqrtf(4.0f * app.G * b.mass /
                                         (app.speedOfLight * app.speedOfLight * fz));
            float rE = thetaE * app.camZoom / (2.0f * tanHalfFov)
                     * std::max(app.lensingStrength, 0.05f);
            rE = std::min(rE, 0.35f);
            cu.bh[cu.count][0] = u;
            cu.bh[cu.count][1] = v;
            cu.bh[cu.count][2] = rE;
            cu.bh[cu.count][3] = rE * 0.42f;   // capture (shadow) radius
            cu.count++;
        }

        vkMapMemory(app.device, app.compositeUboMemory, 0, sizeof(CompositeUBO), 0, &data);
        memcpy(data, &cu, sizeof(CompositeUBO));
        vkUnmapMemory(app.device, app.compositeUboMemory);
    }

    // Update trail UBO
    if (app.trailUboBuffer) {
        TrailUBO trail{};
        trail.camRotX = app.camRotX;
        trail.camRotY = app.camRotY;
        trail.camZoom = app.camZoom;
        trail.camX = app.camX;
        trail.camY = app.camY;
        trail.camZ = app.camZ;
        trail.screenWidth = (float)app.swapchainExtent.width;
        trail.screenHeight = (float)app.swapchainExtent.height;
        trail.pointScale = app.pointScale;
        trail.trailLength = app.trailLength;
        trail.numParticles = app.numParticles;
        trail.trailHead = app.trailHead;

        vkMapMemory(app.device, app.trailUboMemory, 0, sizeof(TrailUBO), 0, &data);
        memcpy(data, &trail, sizeof(TrailUBO));
        vkUnmapMemory(app.device, app.trailUboMemory);
    }
}


void runCompute() {
    if (app.paused) return;

    // Adaptive substepping: split large effective timesteps so the integrator
    // stays in its accurate regime even at high time-scale settings
    float simDt = app.frameDeltaTime * app.timeScale;
    int substeps = std::clamp((int)std::ceil(simDt / 0.025f), 1, 4);
    float stepDt = simDt / substeps;

    vkWaitForFences(app.device, 1, &app.computeFence, VK_TRUE, UINT64_MAX);
    vkResetFences(app.device, 1, &app.computeFence);
    vkResetCommandBuffer(app.computeCommandBuffer, 0);
    // GPU compute is idle from here until submit: host writes are safe

    // ------------------------------------------------------------
    // CPU half of the async sort apply. The permutation may also be
    // a compaction (consumed particles dropped), shrinking the count,
    // so this must run before the UBO write and dispatch sizing.
    // ------------------------------------------------------------
    bool applyPerm = false;
    if (app.sortReady) {
        if (app.sortReady->n == app.numParticles && app.permutePipeline &&
            !app.sortReady->perm.empty()) {
            const std::vector<int32_t>& perm = app.sortReady->perm;
            int newN = (int)perm.size();

            void* pdata;
            vkMapMemory(app.device, app.permMemory, 0,
                        (VkDeviceSize)newN * sizeof(int32_t), 0, &pdata);
            memcpy(pdata, perm.data(), (size_t)newN * sizeof(int32_t));
            vkUnmapMemory(app.device, app.permMemory);

            // Trail row map follows: CPU shadow + free-list only - the GPU
            // gather below is the sole writer of the device buffer
            permuteTrailRows(std::vector<int>(perm.begin(), perm.end()), false);

            // Tracked black hole gids move with the permutation
            if (!app.blackHoles.empty()) {
                std::vector<int32_t> inv(app.numParticles, -1);
                for (int i = 0; i < newN; i++) inv[perm[i]] = i;
                for (auto& b : app.blackHoles)
                    if (b.gid >= 0 && b.gid < (int)inv.size()) b.gid = inv[b.gid];
            }

            app.numParticles = newN;
            applyPerm = true;
            app.lastSortApply = SDL_GetTicks();
            if (getenv("GRAV_DEBUG")) printf("[sort] perm applied (n=%d)\n", newN);
        }
        app.sortReady.reset();   // consumed, or dropped because the count changed
    }

    // ------------------------------------------------------------
    // Black hole channel maintenance: fold last frame's consumption
    // into growth entries, upload the current hole list (position,
    // capture radius, gid), zero the accumulators and the output.
    // ------------------------------------------------------------
    if (app.bhMaybePresent && app.bhSimMemory) {
        void* data;
        vkMapMemory(app.device, app.bhSimMemory, 0, sizeof(BHSimCPU), 0, &data);
        BHSimCPU* simBuf = (BHSimCPU*)data;

        // Harvest fixed-point meal accumulators keyed by last upload's gids
        std::vector<std::pair<int32_t, std::array<float, 4>>> meals;
        for (size_t k = 0; k < app.lastBHSimGids.size() && k < MAX_SIM_BLACK_HOLES; k++) {
            int32_t* a = simBuf->eatAcc[k];
            if (a[0] != 0 || a[1] != 0 || a[2] != 0 || a[3] != 0) {
                meals.push_back({app.lastBHSimGids[k],
                    {a[0] / 256.0f, a[1] / 16.0f, a[2] / 16.0f, a[3] / 16.0f}});
            }
        }

        int n = std::min((int)app.blackHoles.size(), (int)MAX_SIM_BLACK_HOLES);
        memset(simBuf, 0, sizeof(BHSimCPU));
        simBuf->count = n;
        app.lastBHSimGids.assign(n, -1);
        float c2 = std::max(app.speedOfLight * app.speedOfLight, 1.0f);
        for (int k = 0; k < n; k++) {
            const App::BHInfo& b = app.blackHoles[k];
            simBuf->posRad[k][0] = b.x;
            simBuf->posRad[k][1] = b.y;
            simBuf->posRad[k][2] = b.z;
            // Schwarzschild capture radius (matches the rendered shadow scale)
            simBuf->posRad[k][3] = std::max(2.0f * app.G * b.mass / c2, 1.0f);
            simBuf->gid[k][0] = b.gid;
            simBuf->gid[k][1] = b.type;
            simBuf->gid[k][2] = (int32_t)lroundf(b.spin * 1024.0f);
            for (const auto& m : meals) {
                if (m.first == b.gid) {
                    simBuf->growth[k][0] = m.second[0];
                    simBuf->growth[k][1] = m.second[1];
                    simBuf->growth[k][2] = m.second[2];
                    simBuf->growth[k][3] = m.second[3];
                    break;
                }
            }
            app.lastBHSimGids[k] = b.gid;
        }
        vkUnmapMemory(app.device, app.bhSimMemory);

        vkMapMemory(app.device, app.bhOutMemory, 0, sizeof(int32_t), 0, &data);
        *(int32_t*)data = 0;
        vkUnmapMemory(app.device, app.bhOutMemory);
    }

    // Compute is idle here (fence waited), safe to write the UBO
    writeSimUBO(stepDt);

    // Advance the trail ring buffer every trailUpdateFreq-th simulated frame
    bool writeTrail = false;
    if (app.trailsEnabled) {
        app.trailUpdateCounter++;
        if (app.trailUpdateCounter >= app.trailUpdateFreq) {
            app.trailUpdateCounter = 0;
            app.trailHead = (app.trailHead + 1) % app.trailLength;
            writeTrail = true;
        }
    }

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(app.computeCommandBuffer, &beginInfo);

    // Barrier to ensure any prior transfer writes are visible to compute reads
    VkMemoryBarrier preBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    preBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    preBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(app.computeCommandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0, 1, &preBarrier, 0, nullptr, 0, nullptr);

    vkCmdBindDescriptorSets(app.computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            app.computePipelineLayout, 0, 1, &app.computeDescSet, 0, nullptr);

    uint32_t groups = (app.numParticles + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;

    VkMemoryBarrier compBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    compBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    compBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

    // GPU half of the sort apply: gather particles, Verlet history, and the
    // trail row map through the permutation, then copy back. The acceleration
    // history survives the reorder (no half-kick glitch).
    if (applyPerm) {
        vkCmdBindPipeline(app.computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          app.permutePipeline);
        vkCmdDispatch(app.computeCommandBuffer, groups, 1, 1);

        VkMemoryBarrier toXfer{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        toXfer.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        toXfer.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(app.computeCommandBuffer,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 1, &toXfer, 0, nullptr, 0, nullptr);

        VkBufferCopy cp{0, 0, (VkDeviceSize)app.numParticles * sizeof(Particle)};
        vkCmdCopyBuffer(app.computeCommandBuffer, app.sortScratchBuffer, app.particleBuffer, 1, &cp);
        VkBufferCopy ca{app.scratchAccOff, 0, (VkDeviceSize)app.numParticles * 4 * sizeof(float)};
        vkCmdCopyBuffer(app.computeCommandBuffer, app.sortScratchBuffer, app.accBuffer, 1, &ca);
        VkBufferCopy cr{app.scratchRowOff, 0, (VkDeviceSize)app.numParticles * sizeof(int32_t)};
        vkCmdCopyBuffer(app.computeCommandBuffer, app.sortScratchBuffer, app.trailRowMapBuffer, 1, &cr);

        VkMemoryBarrier fromXfer{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        fromXfer.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        fromXfer.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(app.computeCommandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                             0, 1, &fromXfer, 0, nullptr, 0, nullptr);
    }

    for (int s = 0; s < substeps; s++) {
        // Pass 1: reduce each 256-particle tile to a monopole (COM + mass)
        vkCmdBindPipeline(app.computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, app.tileComPipeline);
        vkCmdDispatch(app.computeCommandBuffer, groups, 1, 1);
        vkCmdPipelineBarrier(app.computeCommandBuffer,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 1, &compBarrier, 0, nullptr, 0, nullptr);

        // Pass 2: forces + velocity Verlet integration. Push constants tell
        // the shader when to apply queued black hole growth (first substep)
        // and when to emit the end-of-frame hole list (last substep).
        int32_t physPush[2] = {(s == substeps - 1) ? 1 : 0, (s == 0) ? 1 : 0};
        vkCmdPushConstants(app.computeCommandBuffer, app.computePipelineLayout,
                           VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(physPush), physPush);
        vkCmdBindPipeline(app.computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, app.computePipeline);
        vkCmdDispatch(app.computeCommandBuffer, groups, 1, 1);
        if (s < substeps - 1) {
            vkCmdPipelineBarrier(app.computeCommandBuffer,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 0, 1, &compBarrier, 0, nullptr, 0, nullptr);
        }
    }

    // Stamp current positions into the trail ring buffer (GPU-side; no readback)
    if (writeTrail && app.trailUpdatePipeline) {
        vkCmdPipelineBarrier(app.computeCommandBuffer,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 1, &compBarrier, 0, nullptr, 0, nullptr);
        int32_t trailPush[2] = {app.trailHead, app.trailLength};
        vkCmdPushConstants(app.computeCommandBuffer, app.computePipelineLayout,
                           VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(trailPush), trailPush);
        vkCmdBindPipeline(app.computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          app.trailUpdatePipeline);
        vkCmdDispatch(app.computeCommandBuffer, groups, 1, 1);
    }

    // Memory barrier: ensure compute writes are visible to vertex shader reads
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(app.computeCommandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                         0, 1, &barrier, 0, nullptr, 0, nullptr);

    vkEndCommandBuffer(app.computeCommandBuffer);

    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &app.computeCommandBuffer;
    vkQueueSubmit(app.computeQueue, 1, &submit, app.computeFence);

    // Wait for compute to finish before rendering
    vkWaitForFences(app.device, 1, &app.computeFence, VK_TRUE, UINT64_MAX);

    // Exact end-of-frame black hole states from the GPU channel: this is what
    // the composite renderer draws, so hole motion is per-frame accurate
    // (no scans, no extrapolation, no jumping)
    if (app.bhMaybePresent && app.bhOutMemory) {
        void* data;
        vkMapMemory(app.device, app.bhOutMemory, 0, sizeof(BHOutCPU), 0, &data);
        const BHOutCPU* out = (const BHOutCPU*)data;
        int cnt = std::clamp(out->count, 0, (int)MAX_SIM_BLACK_HOLES);
        app.blackHoles.clear();
        for (int s2 = 0; s2 < cnt; s2++) {
            const float* d0 = out->data[2 * s2];
            const float* d1 = out->data[2 * s2 + 1];
            app.blackHoles.push_back({d0[0], d0[1], d0[2], d0[3],
                                      (int)d1[0], d1[1], d1[2], (int)d1[3]});
        }
        vkUnmapMemory(app.device, app.bhOutMemory);
        std::sort(app.blackHoles.begin(), app.blackHoles.end(),
                  [](const App::BHInfo& a, const App::BHInfo& b) { return a.mass > b.mass; });
        if (cnt == 0) app.bhMaybePresent = false;
    }

    app.simTime += simDt;
}

// Sum the per-workgroup diagnostic partial sums written by the physics shader.
// Safe to call right after runCompute (the compute fence has been waited on).
void readDiagnostics(bool force) {
    if (app.paused || !app.diagMemory || app.numParticles <= 0) return;
    app.diagFrameCounter++;

    uint32_t groups = (app.numParticles + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
    void* data;
    vkMapMemory(app.device, app.diagMemory, 0, (VkDeviceSize)groups * 16 * sizeof(float), 0, &data);
    const float* v = (const float*)data;
    double ke = 0, pe = 0, mass = 0, count = 0, px = 0, py = 0, pz = 0, spd = 0;
    double q[6] = {0, 0, 0, 0, 0, 0};
    for (uint32_t g = 0; g < groups; g++) {
        const float* e = v + g * 16;
        ke += e[0]; pe += e[1]; mass += e[2]; count += e[3];
        px += e[4]; py += e[5]; pz += e[6]; spd += e[7];
        q[0] += e[8]; q[1] += e[9]; q[2] += e[10];
        q[3] += e[11]; q[4] += e[12]; q[5] += e[13];
    }
    vkUnmapMemory(app.device, app.diagMemory);

    app.diag.ke = ke; app.diag.pe = pe; app.diag.mass = mass; app.diag.count = count;
    app.diag.px = px; app.diag.py = py; app.diag.pz = pz; app.diag.speedSum = spd;
    app.diag.valid = std::isfinite(ke) && std::isfinite(pe);

    // ------------------------------------------------------------
    // Gravitational wave observatory (quadrupole formula):
    // h+ ~ (d2Qxx - d2Qyy)/2 and hx ~ d2Qxy for an observer on +Z.
    // Second derivative by central finite difference over sim time.
    // ------------------------------------------------------------
    double dtSim = (double)app.frameDeltaTime * app.timeScale;
    memcpy(app.gwQ[0], app.gwQ[1], sizeof(app.gwQ[0]));
    memcpy(app.gwQ[1], app.gwQ[2], sizeof(app.gwQ[1]));
    memcpy(app.gwQ[2], q, sizeof(app.gwQ[2]));
    app.gwDtHist[0] = app.gwDtHist[1];
    app.gwDtHist[1] = dtSim;
    app.gwSamples++;
    if (app.gwSamples >= 3 && app.gwDtHist[0] > 0 && app.gwDtHist[1] > 0) {
        double dtm = 0.5 * (app.gwDtHist[0] + app.gwDtHist[1]);
        auto dd = [&](int i) {
            return (app.gwQ[2][i] - 2.0 * app.gwQ[1][i] + app.gwQ[0][i]) / (dtm * dtm);
        };
        double hp = 0.5 * (dd(0) - dd(1));
        double hx = dd(3);
        if (std::isfinite(hp) && std::isfinite(hx)) {
            // light smoothing knocks down frame-to-frame finite-difference noise
            app.gwSmoothP += (hp - app.gwSmoothP) * 0.35;
            app.gwSmoothX += (hx - app.gwSmoothX) * 0.35;
            app.gwPlus.push_back((float)app.gwSmoothP);
            app.gwCross.push_back((float)app.gwSmoothX);
            if (app.gwPlus.size() > 260) {
                app.gwPlus.erase(app.gwPlus.begin());
                app.gwCross.erase(app.gwCross.begin());
            }
        }
    }

    double E = ke + pe;
    static bool diagDebug = getenv("GRAV_DEBUG") != nullptr;
    if (diagDebug && (force || (app.diagFrameCounter % 10) == 0))
        printf("[diag] N=%d KE=%.5g PE=%.5g E=%.5g P=(%.3g,%.3g,%.3g) w=%.1f\n",
               (int)count, ke, pe, E, px, py, pz, spd / groups);
    if (!app.energyBaselineSet && app.diag.valid && count > 0) {
        app.energyBaseline = E;
        app.momentumBaseline[0] = px;
        app.momentumBaseline[1] = py;
        app.momentumBaseline[2] = pz;
        app.energyBaselineSet = true;
    }
    if (force || (app.diagFrameCounter % 10) == 0) {
        app.energyHistory.push_back((float)E);
        if (app.energyHistory.size() > 150) app.energyHistory.erase(app.energyHistory.begin());
        app.menuDirty = true;
    }
}

// ============================================================
// ACCRETION: merge gravitationally bound close pairs
//
// CPU pass every 400 ms: spatial-hash all particles, and for any
// pair closer than the merge radius whose relative kinetic energy
// is below its mutual binding energy, replace both with a single
// momentum-conserving body at their barycenter. Mass and momentum
// are conserved exactly; kinetic energy drops (inelastic), which
// is what lets planetesimals actually grow.
// ============================================================
void runMergePass() {
    if (!app.mergeEnabled || app.paused || app.numParticles < 2) return;
    uint32_t now = SDL_GetTicks();
    if (now - app.lastMergePass < 400) return;
    app.lastMergePass = now;

    int n = app.numParticles;
    std::vector<Particle> parts;
    if (!readbackParticles(parts, n)) return;

    float mergeR = std::max(app.softening * 0.5f, 1.0f);
    float invCell = 1.0f / mergeR;
    auto cellKey = [&](float x, float y, float z) -> long long {
        long long ix = (long long)floorf(x * invCell) + (1LL << 20);
        long long iy = (long long)floorf(y * invCell) + (1LL << 20);
        long long iz = (long long)floorf(z * invCell) + (1LL << 20);
        return (ix << 42) | (iy << 21) | iz;
    };

    std::unordered_map<long long, std::vector<int>> grid;
    grid.reserve(n * 2);
    for (int i = 0; i < n; i++)
        grid[cellKey(parts[i].x, parts[i].y, parts[i].z)].push_back(i);

    std::vector<char> dead(n, 0);
    int merges = 0;
    for (int i = 0; i < n; i++) {
        if (dead[i]) continue;
        Particle& a = parts[i];
        for (int dx = -1; dx <= 1; dx++)
        for (int dy = -1; dy <= 1; dy++)
        for (int dz = -1; dz <= 1; dz++) {
            auto it = grid.find(cellKey(a.x + dx * mergeR, a.y + dy * mergeR, a.z + dz * mergeR));
            if (it == grid.end()) continue;
            for (int j : it->second) {
                if (j <= i || dead[j] || dead[i]) continue;
                Particle& b = parts[j];
                float ddx = a.x - b.x, ddy = a.y - b.y, ddz = a.z - b.z;
                float dist2 = ddx * ddx + ddy * ddy + ddz * ddz;
                if (dist2 > mergeR * mergeR) continue;
                float dist = std::max(sqrtf(dist2), 0.1f);
                float dvx = a.vx - b.vx, dvy = a.vy - b.vy, dvz = a.vz - b.vz;
                float relv2 = dvx * dvx + dvy * dvy + dvz * dvz;
                float mu = a.mass * b.mass / (a.mass + b.mass);
                // Only merge bound pairs: relative KE below mutual binding energy
                if (0.5f * mu * relv2 >= app.G * a.mass * b.mass / dist) continue;

                float total = a.mass + b.mass;
                a.x = (a.mass * a.x + b.mass * b.x) / total;
                a.y = (a.mass * a.y + b.mass * b.y) / total;
                a.z = (a.mass * a.z + b.mass * b.z) / total;
                a.vx = (a.mass * a.vx + b.mass * b.vx) / total;
                a.vy = (a.mass * a.vy + b.mass * b.vy) / total;
                a.vz = (a.mass * a.vz + b.mass * b.vz) / total;
                if (b.mass > a.mass) { a.r = b.r; a.g = b.g; a.b = b.b; a.ptype = b.ptype; }
                a.charge += b.charge;
                a.mass = total;
                dead[j] = 1;
                merges++;
            }
        }
    }

    if (merges == 0) return;

    std::vector<Particle> alive;
    std::vector<int> aliveIdx;
    alive.reserve(n - merges);
    aliveIdx.reserve(n - merges);
    for (int i = 0; i < n; i++) {
        if (!dead[i]) {
            alive.push_back(parts[i]);
            aliveIdx.push_back(i);
        }
    }

    app.numParticles = (int)alive.size();
    app.mergeCount += merges;
    uploadParticles(alive);
    permuteTrailRows(aliveIdx);   // survivors keep their trails
    app.menuDirty = true;
}

// ============================================================
// SPATIAL SORT (Morton / Z-order)
//
// The physics shader partitions particles into 256-wide tiles by
// INDEX. Sorting by Morton code every few seconds makes those
// tiles spatially compact, which is what makes the hybrid
// far-field honest: a compact tile really is well-approximated
// by its monopole, so the unsampled-tile error nearly vanishes.
// Runs only when tile sampling is active (N > 32768).
// ============================================================
static inline uint64_t expandBits21(uint64_t v) {
    v &= 0x1FFFFF;
    v = (v | v << 32) & 0x1F00000000FFFFULL;
    v = (v | v << 16) & 0x1F0000FF0000FFULL;
    v = (v | v << 8)  & 0x100F00F00F00F00FULL;
    v = (v | v << 4)  & 0x10C30C30C30C30C3ULL;
    v = (v | v << 2)  & 0x1249249249249249ULL;
    return v;
}

// In-place Morton sort of a particle array
void mortonSort(std::vector<Particle>& parts, std::vector<int>* outPerm) {
    int n = (int)parts.size();
    if (n < 2) return;
    float mn[3] = {parts[0].x, parts[0].y, parts[0].z};
    float mx[3] = {parts[0].x, parts[0].y, parts[0].z};
    for (const Particle& p : parts) {
        mn[0] = std::min(mn[0], p.x); mx[0] = std::max(mx[0], p.x);
        mn[1] = std::min(mn[1], p.y); mx[1] = std::max(mx[1], p.y);
        mn[2] = std::min(mn[2], p.z); mx[2] = std::max(mx[2], p.z);
    }
    float span = std::max({mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2], 1e-3f});
    float scale = 2097151.0f / span;   // 21 bits per axis

    std::vector<std::pair<uint64_t, int>> keys(n);
    for (int i = 0; i < n; i++) {
        uint64_t cx = (uint64_t)std::clamp((parts[i].x - mn[0]) * scale, 0.0f, 2097151.0f);
        uint64_t cy = (uint64_t)std::clamp((parts[i].y - mn[1]) * scale, 0.0f, 2097151.0f);
        uint64_t cz = (uint64_t)std::clamp((parts[i].z - mn[2]) * scale, 0.0f, 2097151.0f);
        keys[i] = {expandBits21(cx) | (expandBits21(cy) << 1) | (expandBits21(cz) << 2), i};
    }
    std::sort(keys.begin(), keys.end());

    std::vector<Particle> sorted(n);
    for (int i = 0; i < n; i++) sorted[i] = parts[keys[i].second];
    parts.swap(sorted);

    if (outPerm) {
        outPerm->resize(n);
        for (int i = 0; i < n; i++) (*outPerm)[i] = keys[i].second;
    }
}

// Follow a particle permutation or compaction (srcIndex[i] = old particle
// index that becomes new index i) by permuting the trail ROW MAP - the trail
// data itself never moves. The old approach shuffled 25+ MB through uncached
// host-visible memory and caused a visible frame hitch every sort pass; this
// is a few hundred KB of cached CPU work. Rows dropped by a compaction are
// recycled through the free list (black hole spawns reuse them).
void permuteTrailRows(const std::vector<int>& srcIndex, bool syncGpu) {
    size_t oldN = app.trailRowMap.size();
    size_t newN = srcIndex.size();
    if (oldN == 0) return;

    std::vector<int32_t> newMap(newN);
    std::vector<char> carried(oldN, 0);
    for (size_t i = 0; i < newN; i++) {
        newMap[i] = app.trailRowMap[srcIndex[i]];
        carried[srcIndex[i]] = 1;
    }
    for (size_t j = 0; j < oldN; j++)
        if (!carried[j]) app.freeTrailRows.push_back(app.trailRowMap[j]);
    app.trailRowMap.swap(newMap);
    // During an async-sort apply the GPU permute pass is the sole writer of
    // the device buffer: it gathers rowMap[perm[i]] from the OLD contents.
    // Syncing the CPU-permuted map first would make the gather permute an
    // already-permuted map - every particle would display its neighbour's
    // trail, and horizon wipes would hit the wrong rows (the cause of a
    // ghost ring that flashed around black holes every sweep).
    if (syncGpu) syncTrailRowMap();
}

// Morton permutation of the LIVING particles (consumed ones - mass <= 0 -
// are dropped, which is how black hole victims leave the population).
// perm[i] = old index of the particle that should land at new position i;
// perm.size() may be smaller than parts.size(). Runs on a worker thread.
void computeMortonPerm(const std::vector<Particle>& parts, std::vector<int32_t>& perm) {
    int n = (int)parts.size();
    perm.clear();
    perm.reserve(n);
    for (int i = 0; i < n; i++)
        if (parts[i].mass > 0.0f) perm.push_back(i);
    if (perm.size() < 2) return;
    float mn[3] = {parts[perm[0]].x, parts[perm[0]].y, parts[perm[0]].z};
    float mx[3] = {mn[0], mn[1], mn[2]};
    for (int i : perm) {
        const Particle& p = parts[i];
        mn[0] = std::min(mn[0], p.x); mx[0] = std::max(mx[0], p.x);
        mn[1] = std::min(mn[1], p.y); mx[1] = std::max(mx[1], p.y);
        mn[2] = std::min(mn[2], p.z); mx[2] = std::max(mx[2], p.z);
    }
    float span = std::max({mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2], 1e-3f});
    float scale = 2097151.0f / span;
    std::vector<std::pair<uint64_t, int32_t>> keys(perm.size());
    for (size_t k = 0; k < perm.size(); k++) {
        const Particle& p = parts[perm[k]];
        uint64_t cx = (uint64_t)std::clamp((p.x - mn[0]) * scale, 0.0f, 2097151.0f);
        uint64_t cy = (uint64_t)std::clamp((p.y - mn[1]) * scale, 0.0f, 2097151.0f);
        uint64_t cz = (uint64_t)std::clamp((p.z - mn[2]) * scale, 0.0f, 2097151.0f);
        keys[k] = {expandBits21(cx) | (expandBits21(cy) << 1) | (expandBits21(cz) << 2), perm[k]};
    }
    std::sort(keys.begin(), keys.end());
    for (size_t k = 0; k < perm.size(); k++) perm[k] = keys[k].second;
}

// ============================================================
// ASYNC SPATIAL SORT STATE MACHINE (never blocks the frame loop)
//
//   kick:    submit particle-buffer -> snapshot copy, fence attached
//   poll:    fence signaled -> memcpy snapshot, scan black holes,
//            hand it to a detached worker thread for Morton sorting
//   promote: worker done -> stage the permutation for runCompute,
//            which applies it GPU-side (permute.comp + copy-back)
//
// The permutation is computed from a slightly stale snapshot; applying
// a pure reordering to live data is always correct - only the spatial
// coherence is stale, and that drifts over seconds, not frames. If the
// particle count changed meanwhile (merge/spawn), the perm is dropped.
// ============================================================
void runSortStateMachine() {
    if (app.paused) return;
    uint32_t now = SDL_GetTicks();

    // Poll an in-flight snapshot copy
    if (app.snapshotInFlight &&
        vkGetFenceStatus(app.device, app.snapshotFence) == VK_SUCCESS) {
        app.snapshotInFlight = false;
        int n = app.snapshotN;
        auto job = std::make_shared<App::SortJob>();
        job->n = n;
        job->snap.resize(n);
        void* data;
        vkMapMemory(app.device, app.snapshotMemory, 0, (VkDeviceSize)n * sizeof(Particle), 0, &data);
        memcpy(job->snap.data(), data, (size_t)n * sizeof(Particle));
        vkUnmapMemory(app.device, app.snapshotMemory);

        // Sort for far-field coherence at large N; also run whenever
        // consumed (mass 0) particles need sweeping out of the population
        bool hasDead = false;
        for (const Particle& p : job->snap)
            if (p.mass <= 0.0f) { hasDead = true; break; }
        bool wantSort = (n > 128 * WORKGROUP_SIZE || hasDead) &&
                        !app.sortJob && !app.sortReady &&
                        (now - app.lastSortApply > 2500);
        if (wantSort) {
            app.sortJob = job;
            std::thread([job]() {
                computeMortonPerm(job->snap, job->perm);
                job->done.store(true, std::memory_order_release);
            }).detach();
        }
    }

    // Promote a finished worker; runCompute picks it up next frame
    if (app.sortJob && app.sortJob->done.load(std::memory_order_acquire)) {
        app.sortReady = app.sortJob;
        app.sortJob.reset();
    }

    // Kick a new snapshot when idle (faster cadence while black holes need
    // tracking; none at all for small sims without black holes)
    uint32_t interval = 1200;
    bool needSnapshot = (app.numParticles > 128 * WORKGROUP_SIZE) || app.bhMaybePresent;
    if (!app.snapshotInFlight && needSnapshot && now - app.lastSnapshotKick >= interval) {
        app.lastSnapshotKick = now;
        app.snapshotN = app.numParticles;
        vkResetFences(app.device, 1, &app.snapshotFence);
        vkResetCommandBuffer(app.snapshotCmdBuf, 0);
        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(app.snapshotCmdBuf, &bi);
        VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        mb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        mb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(app.snapshotCmdBuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
        VkBufferCopy copy{0, 0, (VkDeviceSize)app.snapshotN * sizeof(Particle)};
        vkCmdCopyBuffer(app.snapshotCmdBuf, app.particleBuffer, app.snapshotBuffer, 1, &copy);
        vkEndCommandBuffer(app.snapshotCmdBuf);
        VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        si.commandBufferCount = 1;
        si.pCommandBuffers = &app.snapshotCmdBuf;
        vkQueueSubmit(app.graphicsQueue, 1, &si, app.snapshotFence);
        app.snapshotInFlight = true;
        // no wait: the fence is polled on later frames
    }
}

// ============================================================
// QUICK SAVE / LOAD (F10 / Shift+F10)
// ============================================================
struct SaveHeader {
    char magic[8];        // "GRAVSV01"
    int32_t numParticles;
    int32_t gravityType, colorMode, distribution, massMode, particleMode, boundaryMode;
    int32_t flags;        // bit0 darkMatter, bit1 em, bit2 strong, bit3 relativity,
                          // bit4 expansion, bit5 gravity, bit6 trails, bit7 merge
    float G, softening, damping, timeScale, centralMass, baseCentralMass;
    float boundaryRadius, darkMatterStrength, emStrength, strongStrength, strongRange;
    float speedOfLight, hubbleConstant, lensingStrength, glowIntensity, pointScale;
    float blackHoleMass, simTime;
    float camX, camY, camZ, camRotX, camRotY, camZoom;
};

void saveState() {
    std::vector<Particle> parts;
    if (!readbackParticles(parts, app.numParticles)) return;

    mkdir("saves", 0755);
    SaveHeader h{};
    memcpy(h.magic, "GRAVSV01", 8);
    h.numParticles = app.numParticles;
    h.gravityType = app.gravityType; h.colorMode = app.colorMode;
    h.distribution = app.distribution; h.massMode = app.massMode;
    h.particleMode = app.particleMode; h.boundaryMode = app.boundaryMode;
    h.flags = (app.darkMatter ? 1 : 0) | (app.emEnabled ? 2 : 0) |
              (app.strongForceEnabled ? 4 : 0) | (app.relativityEnabled ? 8 : 0) |
              (app.expansionEnabled ? 16 : 0) | (app.gravityEnabled ? 32 : 0) |
              (app.trailsEnabled ? 64 : 0) | (app.mergeEnabled ? 128 : 0);
    h.G = app.G; h.softening = app.softening; h.damping = app.damping;
    h.timeScale = app.timeScale; h.centralMass = app.centralMass;
    h.baseCentralMass = app.baseCentralMass; h.boundaryRadius = app.boundaryRadius;
    h.darkMatterStrength = app.darkMatterStrength; h.emStrength = app.emStrength;
    h.strongStrength = app.strongStrength; h.strongRange = app.strongRange;
    h.speedOfLight = app.speedOfLight; h.hubbleConstant = app.hubbleConstant;
    h.lensingStrength = app.lensingStrength; h.glowIntensity = app.glowIntensity;
    h.pointScale = app.pointScale; h.blackHoleMass = app.blackHoleMass;
    h.simTime = app.simTime;
    h.camX = app.camX; h.camY = app.camY; h.camZ = app.camZ;
    h.camRotX = app.camRotX; h.camRotY = app.camRotY; h.camZoom = app.camZoom;

    FILE* f = fopen("saves/quicksave.grav", "wb");
    if (!f) { fprintf(stderr, "Save failed: cannot open saves/quicksave.grav\n"); return; }
    fwrite(&h, sizeof(h), 1, f);
    fwrite(parts.data(), sizeof(Particle), parts.size(), f);
    fclose(f);
    printf("Saved %d particles to saves/quicksave.grav\n", app.numParticles);
}

void loadState() {
    FILE* f = fopen("saves/quicksave.grav", "rb");
    if (!f) { fprintf(stderr, "Load failed: saves/quicksave.grav not found\n"); return; }
    SaveHeader h{};
    if (fread(&h, sizeof(h), 1, f) != 1 || memcmp(h.magic, "GRAVSV01", 8) != 0 ||
        h.numParticles < 1 || h.numParticles > MAX_PARTICLES) {
        fprintf(stderr, "Load failed: invalid save file\n");
        fclose(f);
        return;
    }
    std::vector<Particle> parts(h.numParticles);
    if (fread(parts.data(), sizeof(Particle), parts.size(), f) != parts.size()) {
        fprintf(stderr, "Load failed: truncated save file\n");
        fclose(f);
        return;
    }
    fclose(f);

    app.numParticles = h.numParticles;
    app.gravityType = h.gravityType; app.colorMode = h.colorMode;
    app.distribution = h.distribution; app.massMode = h.massMode;
    app.particleMode = h.particleMode; app.boundaryMode = h.boundaryMode;
    app.darkMatter = h.flags & 1; app.emEnabled = h.flags & 2;
    app.strongForceEnabled = h.flags & 4; app.relativityEnabled = h.flags & 8;
    app.expansionEnabled = h.flags & 16; app.gravityEnabled = h.flags & 32;
    app.trailsEnabled = h.flags & 64; app.mergeEnabled = h.flags & 128;
    app.G = h.G; app.softening = h.softening; app.damping = h.damping;
    app.timeScale = h.timeScale; app.centralMass = h.centralMass;
    app.baseCentralMass = h.baseCentralMass; app.boundaryRadius = h.boundaryRadius;
    app.darkMatterStrength = h.darkMatterStrength; app.emStrength = h.emStrength;
    app.strongStrength = h.strongStrength; app.strongRange = h.strongRange;
    app.speedOfLight = h.speedOfLight; app.hubbleConstant = h.hubbleConstant;
    app.lensingStrength = h.lensingStrength; app.glowIntensity = h.glowIntensity;
    app.pointScale = h.pointScale; app.blackHoleMass = h.blackHoleMass;
    app.simTime = h.simTime;
    app.camX = h.camX; app.camY = h.camY; app.camZ = h.camZ;
    app.camRotX = h.camRotX; app.camRotY = h.camRotY; app.camZoom = h.camZoom;

    uploadParticles(parts);
    resetTrails(parts);
    app.energyBaselineSet = false;
    app.energyHistory.clear();
    app.diag.valid = false;
    app.bhMaybePresent = true;   // rescan will confirm or clear
    app.lastBHScan = 0;
    app.menuDirty = true;
    printf("Loaded %d particles from saves/quicksave.grav\n", app.numParticles);
}

// ============================================================
// SCREENSHOT (F12): copy the last presented swapchain image to
// a host buffer and write it as a 32-bit BMP
// ============================================================
