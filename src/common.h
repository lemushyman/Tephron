#pragma once

/*
 * Tephron - real-time Vulkan N-body simulation
 *
 * Features:
 * - Vulkan compute shaders for N-body physics
 * - Instanced particle rendering
 * - SDL2_ttf text overlay
 * - Multiple simulation presets
 * - Extensive customization options
 */

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_ttf.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <array>
#include <algorithm>
#include <fstream>
#include <set>
#include <string>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <thread>
#include <sys/stat.h>

#define MAX_PARTICLES 500000
#define DEFAULT_PARTICLES 30000
#define WORKGROUP_SIZE 256
#define MAX_TILES_TOTAL ((MAX_PARTICLES + 255) / 256)

// Gravity types
enum GravityType {
    GRAVITY_NEWTONIAN = 0,   // 1/r² - realistic
    GRAVITY_INV_LINEAR,      // 1/r - softer
    GRAVITY_LINEAR,          // r - spring-like
    GRAVITY_CONSTANT,        // constant attraction
    GRAVITY_REPULSIVE,       // -1/r² - push apart
    GRAVITY_OSCILLATING,     // sin modulated
    GRAVITY_INVERSE_CUBE,    // 1/r³ - extreme falloff
    GRAVITY_LOGARITHMIC,     // log(r) - very soft
    GRAVITY_YUKAWA,          // screened potential
    GRAVITY_LENNARD_JONES,   // molecular dynamics style
    GRAVITY_SPIRAL,          // tangential swirl force
    GRAVITY_PULSE,           // pulsing attraction/repulsion
    GRAVITY_QUANTIZED,       // gravity shells
    GRAVITY_FUNNEL,          // ring-forming
    GRAVITY_VORTEX,          // spiraling inward
    GRAVITY_RUBBER_BAND,     // snaps back at distance
    GRAVITY_COUNT
};

static const char* gravityNames[] = {
    "Newtonian (1/r^2)", "Inverse Linear (1/r)", "Linear (r)", "Constant",
    "Repulsive (-1/r^2)", "Oscillating", "Inverse Cube (1/r^3)",
    "Logarithmic", "Yukawa", "Lennard-Jones", "Spiral", "Pulse",
    "Quantized", "Funnel", "Vortex", "Rubber Band"
};

// Color modes - Physics-focused coloring
enum ColorMode {
    COLOR_VELOCITY = 0,      // Speed-based coloring
    COLOR_MASS,              // Mass-based stellar classification
    COLOR_PARTICLE_TYPE,     // Electron/proton/neutron/quark colors
    COLOR_CHARGE,            // Positive (red) / Negative (blue) / Neutral (gray)
    COLOR_EM_FORCE,          // Electromagnetic attraction/repulsion
    COLOR_STRONG_FORCE,      // Strong nuclear force (quarks glow)
    COLOR_NET_FORCE,         // Combined force magnitude
    COLOR_POTENTIAL,         // Gravitational potential energy
    COLOR_DISTANCE,          // Distance from center
    COLOR_KINETIC,           // Kinetic energy (mass * velocity^2)
    COLOR_BINDING,           // Bound vs escaping particles
    COLOR_TEMPERATURE,       // Thermal (velocity dispersion proxy)
    COLOR_FIRE,              // Aesthetic: fire palette
    COLOR_GALAXY,            // Aesthetic: galaxy spiral
    COLOR_RAINBOW,           // Aesthetic: position-based hue
    COLOR_ELECTRIC,          // Aesthetic: electric storm
    COLOR_BLACKHOLE,         // Aesthetic: accretion disk
    COLOR_PLASMA,            // Hot plasma colors
    COLOR_NEBULA,            // Cosmic nebula palette
    COLOR_QUANTUM,           // Quantum-inspired (wavefunction-like)
    COLOR_RELATIVITY,        // Relativistic gamma factor & Doppler
    COLOR_COUNT
};

static const char* colorNames[] = {
    "Velocity", "Mass", "Particle Type", "Charge", "EM Force",
    "Strong Force", "Net Force", "Potential", "Distance", "Kinetic Energy",
    "Binding", "Temperature", "Fire", "Galaxy", "Rainbow",
    "Electric", "Black Hole", "Plasma", "Nebula", "Quantum", "Relativity"
};

// Distribution presets
enum Distribution {
    DIST_SPHERE = 0,
    DIST_DISK,
    DIST_SHELL,
    DIST_RANDOM,
    DIST_BINARY_GALAXIES,
    DIST_SOLAR_SYSTEM,
    DIST_RING,
    DIST_CLUSTER,
    DIST_FILAMENT,
    DIST_EXPLOSION,
    DIST_VORTEX,
    DIST_GRID,
    DIST_PLUMMER,     // Plummer sphere in virial equilibrium
    DIST_FIGURE8,     // Chenciner-Montgomery three-body choreography + tracer ring
    DIST_ACCRETION,   // Protoplanetary disk with near-Keplerian orbits
    DIST_QUASAR,      // Gas disk feeding a preset central black hole
    DIST_TIDAL,       // Compact cluster on a plunging flyby (tidal disruption)
    DIST_HALO,        // Spherical cloud on true circular orbits (random planes)
    DIST_COUNT
};

static const char* distNames[] = {
    "Sphere", "Disk/Galaxy", "Shell", "Random Cube", "Binary Galaxies",
    "Solar System", "Saturn Rings", "Star Cluster", "Cosmic Filament",
    "Big Bang", "Vortex", "Grid", "Plummer Sphere", "Figure-8 Orbit",
    "Accretion Disk", "Quasar Disk", "Tidal Flyby", "Rotating Halo"
};

// Mass modes
enum MassMode {
    MASS_UNIFORM = 0,
    MASS_RANDOM,
    MASS_CENTRAL,
    MASS_BINARY,
    MASS_HIERARCHICAL,
    MASS_POWER_LAW,      // Many tiny, few huge (realistic IMF)
    MASS_STARS_AND_GAS,  // Bimodal: heavy stars + light gas
    MASS_CLUSTERS,       // Random cluster centers with high mass
    MASS_COUNT
};

static const char* massNames[] = {
    "Uniform", "Random", "Central Star", "Binary Stars", "Hierarchical",
    "Power Law", "Stars+Gas", "Clusters"
};

// Simulation presets
enum Preset {
    PRESET_GALAXY = 0,
    PRESET_SOLAR_SYSTEM,
    PRESET_GALAXY_COLLISION,
    PRESET_GLOBULAR_CLUSTER,
    PRESET_BIG_BANG,
    PRESET_SATURN,
    PRESET_BINARY_ORBIT,
    PRESET_CHAOS,
    PRESET_MOLECULAR,
    PRESET_EXPANSION,
    PRESET_PLUMMER,
    PRESET_FIGURE8,
    PRESET_ACCRETION,
    PRESET_QUASAR,
    PRESET_BINARY_BH,
    PRESET_TIDAL,
    PRESET_BH_SWARM,
    PRESET_PLASMA_STORM,
    PRESET_PULSAR,
    PRESET_COUNT
};

static const char* presetNames[] = {
    "Spiral Galaxy", "Solar System", "Galaxy Collision", "Globular Cluster",
    "Big Bang", "Saturn Rings", "Binary Orbit", "Chaos", "Molecular Gas",
    "Universe Expansion", "Plummer Cluster", "Figure-8 Three-Body",
    "Planet Formation", "Quasar", "Binary Black Holes", "Tidal Disruption",
    "Black Hole Swarm", "Plasma Storm", "Pulsar"
};

// Particle types for physics simulation
enum ParticleType {
    PTYPE_GENERIC = 0,    // Generic matter (for gravity-only sims)
    PTYPE_ELECTRON = 1,   // Electron: charge -1, mass ~0.0005
    PTYPE_PROTON = 2,     // Proton: charge +1, mass 1.0
    PTYPE_NEUTRON = 3,    // Neutron: charge 0, mass 1.0
    PTYPE_UP_QUARK = 4,   // Up quark: charge +2/3, mass ~0.002
    PTYPE_DOWN_QUARK = 5, // Down quark: charge -1/3, mass ~0.005
    PTYPE_POSITRON = 6,   // Positron (anti-electron): charge +1, mass ~0.0005
    PTYPE_BLACKHOLE = 7,  // Black hole: emits no light; rendered as a lensed
                          // shadow with photon ring in the composite pass
    PTYPE_PULSAR = 8,     // Neutron star: pulsing beacon whose beamed wind
                          // sweeps twin cones along a precessing axis;
                          // collapses to a black hole past the mass limit
    PTYPE_COUNT
};

static const char* particleTypeNames[] = {
    "Generic", "Electron", "Proton", "Neutron",
    "Up Quark", "Down Quark", "Positron", "Black Hole", "Pulsar"
};

// Particle structure for GPU
struct Particle {
    float x, y, z, mass;
    float vx, vy, vz, charge;  // charge in units of e (-1, +1, +2/3, -1/3, etc)
    float r, g, b, ptype;      // ptype: see ParticleType enum
};

// Simulation UBO
struct SimUBO {
    float G;
    float softening;
    float dt;
    int numParticles;
    int gravityType;
    float darkMatterStrength;
    int darkMatterEnabled;
    float simTime;
    float damping;
    float centralMass;
    float boundaryRadius;
    int boundaryMode;       // 0=none, 1=reflect, 2=wrap
    // New force parameters
    int gravityEnabled;
    int emEnabled;          // Electromagnetism enabled
    int strongForceEnabled; // Strong nuclear force enabled
    float emStrength;       // Electromagnetic coupling constant
    float strongStrength;   // Strong force coupling
    float strongRange;      // Strong force range
    // Relativistic and cosmological parameters
    int relativityEnabled;  // Enable relativistic effects
    int expansionEnabled;   // Enable spacetime expansion
    float speedOfLight;     // Speed of light limit
    float hubbleConstant;   // Hubble expansion rate
};

// CPU mirror of the physics shader's BHSim SSBO (std430)
#define MAX_SIM_BLACK_HOLES 16
struct BHSimCPU {
    int32_t count; int32_t pad[3];
    float posRad[MAX_SIM_BLACK_HOLES][4];   // xyz pos, w capture radius
    int32_t gid[MAX_SIM_BLACK_HOLES][4];
    float growth[MAX_SIM_BLACK_HOLES][4];   // dMass, dP.xyz
    int32_t eatAcc[MAX_SIM_BLACK_HOLES][4]; // fixed-point atomics
};

// CPU mirror of the physics shader's BHOut SSBO
struct BHOutCPU {
    int32_t count; int32_t pad[3];
    float data[2 * MAX_SIM_BLACK_HOLES][4];
};

// Composite pass UBO: black holes projected to screen space (std140)
#define MAX_BLACK_HOLES 8
struct CompositeUBO {
    float bh[MAX_BLACK_HOLES][4];  // u, v, Einstein radius (v units), shadow radius
    int32_t count;
    int32_t pad[3];
};

// Render UBO
struct RenderUBO {
    float camRotX, camRotY;
    float camZoom;          // Zoom level (1.0 = normal, >1 = zoomed in)
    float camX, camY, camZ; // Camera position
    float screenWidth, screenHeight;
    float glowIntensity;
    int colorMode;
    float pointScale;
    float lensingStrength;  // Gravitational lensing intensity
    float speedOfLight;     // For relativistic visual effects (redshift)
    float simTime;          // For pulsar beacon strobing
};

// Trail point for world-space trails
struct TrailPoint {
    float x, y, z, age;  // position + age (0=newest, 1=oldest, -1=invalid)
};

// Trail UBO
struct TrailUBO {
    float camRotX, camRotY;
    float camZoom;           // Zoom level (1.0 = normal, >1 = zoomed in)
    float camX, camY, camZ;  // Camera position
    float screenWidth, screenHeight;
    float pointScale;
    int trailLength;
    int numParticles;
    int trailHead;           // ring-buffer newest slot
};

#define TRAIL_LENGTH 32  // Number of positions stored per particle

// Text vertex for overlay
struct TextVertex {
    float x, y;      // position in clip space [-1, 1]
    float u, v;      // texture coordinates
    float r, g, b, a; // color
};

struct App {
    SDL_Window* window = nullptr;
    int width = 1600;
    int height = 1000;
    bool fullscreen = false;
    bool needsResize = false;

    // Vulkan handles
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
    uint32_t graphicsFamily = 0;
    uint32_t computeFamily = 0;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkFormat swapchainFormat;
    VkExtent2D swapchainExtent;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;

    VkDescriptorSetLayout computeDescLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout graphicsDescLayout = VK_NULL_HANDLE;
    VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout graphicsPipelineLayout = VK_NULL_HANDLE;
    VkPipeline computePipeline = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet computeDescSet = VK_NULL_HANDLE;
    VkDescriptorSet graphicsDescSet = VK_NULL_HANDLE;

    VkBuffer particleBuffer = VK_NULL_HANDLE;
    VkDeviceMemory particleMemory = VK_NULL_HANDLE;
    VkBuffer simUboBuffer = VK_NULL_HANDLE;
    VkDeviceMemory simUboMemory = VK_NULL_HANDLE;
    VkBuffer renderUboBuffer = VK_NULL_HANDLE;
    VkDeviceMemory renderUboMemory = VK_NULL_HANDLE;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandPool computeCommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    VkCommandBuffer computeCommandBuffer = VK_NULL_HANDLE;

    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkFence renderFence = VK_NULL_HANDLE;
    VkFence computeFence = VK_NULL_HANDLE;

    // Simulation state
    int numParticles = DEFAULT_PARTICLES;
    int gravityType = GRAVITY_NEWTONIAN;
    int colorMode = COLOR_VELOCITY;
    int distribution = DIST_DISK;
    int massMode = MASS_CENTRAL;
    float G = 50.0f;
    float softening = 5.0f;
    float damping = 0.0f;
    float simTime = 0.0f;
    float timeScale = 1.0f;
    float glowIntensity = 0.15f;
    float pointScale = 2.0f;
    bool paused = false;
    bool darkMatter = false;
    float darkMatterStrength = 1.0f;
    float baseCentralMass = 5000.0f;  // Base mass, scales with particle count
    float centralMass = 10000.0f;     // Actual computed central mass
    float boundaryRadius = 1000.0f;
    int boundaryMode = 0;
    bool showMenu = true;
    int menuPage = 0;

    // Camera state (WASD navigation)
    float camX = 0.0f, camY = 0.0f, camZ = -800.0f;  // Camera position
    float camRotX = 0.0f;    // Pitch (up/down look)
    float camRotY = 0.0f;    // Yaw (left/right look)
    float camZoom = 1.0f;    // Zoom level (1.0 = normal, >1 = zoomed in, affects projection scale)

    // Orbit mode (right-click)
    float orbitTargetX = 0.0f, orbitTargetY = 0.0f, orbitTargetZ = 0.0f;  // Orbit pivot point
    float orbitDistance = 800.0f;  // Distance from orbit target
    bool orbitMode = false;  // Whether we're in orbit mode

    // New physics forces
    bool gravityEnabled = true;
    bool emEnabled = false;           // Electromagnetism
    bool strongForceEnabled = false;  // Strong nuclear force
    float emStrength = 50.0f;         // Coulomb constant
    float strongStrength = 500.0f;    // Strong force coupling
    float strongRange = 10.0f;        // Strong force range (very short)

    // Relativistic and cosmological effects
    bool relativityEnabled = false;    // Enable relativistic effects
    bool expansionEnabled = false;     // Enable spacetime expansion (dark energy)
    float speedOfLight = 300.0f;       // Speed of light limit (simulation units)
    float hubbleConstant = 0.01f;      // Expansion rate (higher = faster expansion)
    float lensingStrength = 1.0f;      // Gravitational lensing intensity

    // Particle type distribution (percentages, should sum to 100)
    int particleMode = 0;  // 0=generic, 1=atoms (e/p/n), 2=plasma (e/p), 3=quarks, 4=mixed, 5=fundamental
    float blackHoleMass = 50000.0f;  // Mass of spawned black holes
    // Distribution ratios for "mixed" mode
    float electronRatio = 0.3f;    // 30% electrons
    float protonRatio = 0.3f;      // 30% protons
    float neutronRatio = 0.2f;     // 20% neutrons
    float quarkRatio = 0.2f;       // 20% quarks (split between up/down)

    // Input
    bool mouseDragging = false;
    int mouseLastX = 0, mouseLastY = 0;
    int mouseX = 0, mouseY = 0;

    // WASDQE key states for continuous movement
    bool keyW = false, keyA = false, keyS = false, keyD = false;
    bool keyQ = false, keyE = false;  // Q = up, E = down
    bool keyLeft = false, keyRight = false, keyUp = false, keyDown = false;  // Arrow keys for rotation

    // Mouse state for orbit
    bool rightMouseDragging = false;

    // Graphical menu state
    int openMenu = -1;          // Which dropdown is open (-1 = none)
    int hoverMenu = -1;         // Which menu item is hovered
    int hoverDropdownItem = -1; // Which dropdown item is hovered
    int hoverRow2Var = -1;      // Which row2 variable is hovered (for scroll adjust)

    // World-space trail effect
    bool trailsEnabled = true;
    int trailLength = TRAIL_LENGTH;
    int trailUpdateCounter = 0;
    int trailHead = 0;                  // ring-buffer write slot (GPU trails)
    VkPipeline trailUpdatePipeline = VK_NULL_HANDLE;
    // Trail row indirection: rowMap[particle] -> trail buffer row. Sorts and
    // merges permute this tiny table instead of moving 25+ MB of trail data.
    std::vector<int32_t> trailRowMap;
    std::vector<int32_t> freeTrailRows;
    int trailRowsAllocated = 0;
    VkBuffer trailRowMapBuffer = VK_NULL_HANDLE;
    VkDeviceMemory trailRowMapMemory = VK_NULL_HANDLE;

    // Async spatial sort pipeline: fence-polled GPU snapshot -> worker-thread
    // Morton sort -> GPU-side gather permutation. Nothing in the frame loop
    // ever blocks on it. The snapshot doubles as the black hole scan source.
    struct SortJob {
        std::vector<Particle> snap;
        std::vector<int32_t> perm;
        std::atomic<bool> done{false};
        int n = 0;
    };
    std::shared_ptr<SortJob> sortJob;     // worker running (perm not ready yet)
    std::shared_ptr<SortJob> sortReady;   // perm ready, awaiting GPU apply
    bool snapshotInFlight = false;
    int snapshotN = 0;
    uint32_t lastSnapshotKick = 0;
    uint32_t lastSortApply = 0;
    VkBuffer snapshotBuffer = VK_NULL_HANDLE;
    VkDeviceMemory snapshotMemory = VK_NULL_HANDLE;
    VkCommandBuffer snapshotCmdBuf = VK_NULL_HANDLE;
    VkFence snapshotFence = VK_NULL_HANDLE;
    VkBuffer permBuffer = VK_NULL_HANDLE;
    VkDeviceMemory permMemory = VK_NULL_HANDLE;
    VkBuffer sortScratchBuffer = VK_NULL_HANDLE;   // regions: particles | acc | rowMap
    VkDeviceMemory sortScratchMemory = VK_NULL_HANDLE;
    VkPipeline permutePipeline = VK_NULL_HANDLE;

    // Smoothed simulation clock: frame-time jitter must not modulate motion
    float frameDtSmooth = 0.016f;

    // Device-tier adaptation: per-particle buffers are sized to this cap and
    // preset particle counts scale by perfScale, so the sim fits and runs on
    // anything from integrated laptop graphics to a flagship discrete GPU.
    int maxParticlesCap = MAX_PARTICLES;   // always a multiple of 16 so the
    float perfScale = 1.0f;                // scratch offsets stay 256-aligned
    bool lowFidelity = false;   // 1 HDR mip, no bloom stack, starfield off
    VkDeviceSize scratchAccOff = 0;   // sort-scratch region offsets, set once
    VkDeviceSize scratchRowOff = 0;   // in initVulkan, used by descriptors+copies
    int trailUpdateFreq = 2;  // Update trails every N frames
    VkBuffer trailBuffer = VK_NULL_HANDLE;
    VkDeviceMemory trailMemory = VK_NULL_HANDLE;
    VkBuffer trailUboBuffer = VK_NULL_HANDLE;
    VkDeviceMemory trailUboMemory = VK_NULL_HANDLE;
    VkDescriptorSetLayout trailDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet trailDescSet = VK_NULL_HANDLE;
    VkPipelineLayout trailPipelineLayout = VK_NULL_HANDLE;
    VkPipeline trailPipeline = VK_NULL_HANDLE;

    // HDR rendering pipeline
    VkImage hdrImage = VK_NULL_HANDLE;
    VkDeviceMemory hdrMemory = VK_NULL_HANDLE;
    VkImageView hdrFbView = VK_NULL_HANDLE;       // mip 0 only (for framebuffer)
    VkImageView hdrSamplerView = VK_NULL_HANDLE;   // all mips (for sampling)
    VkSampler hdrSampler = VK_NULL_HANDLE;
    VkRenderPass hdrRenderPass = VK_NULL_HANDLE;
    VkFramebuffer hdrFramebuffer = VK_NULL_HANDLE;
    uint32_t hdrMipLevels = 1;
    VkDescriptorSetLayout compositeDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet compositeDescSet = VK_NULL_HANDLE;
    VkPipelineLayout compositePipelineLayout = VK_NULL_HANDLE;
    VkPipeline compositePipeline = VK_NULL_HANDLE;

    // Display mode - auto-orbit camera around center of mass
    bool displayMode = false;
    float displayOrbitAngle = 0.0f;
    float displayOrbitSpeed = 0.3f;  // Radians per second
    float displayOrbitTilt = 0.3f;   // Camera tilt angle

    // Persistent GPU readback buffer (reused by trails, COM calc, etc.)
    // Eliminates per-frame VkBuffer/VkDeviceMemory alloc/free overhead
    VkBuffer readbackBuffer = VK_NULL_HANDLE;
    VkDeviceMemory readbackMemory = VK_NULL_HANDLE;

    // Velocity Verlet acceleration history (vec4 per particle: acc.xyz + potential)
    VkBuffer accBuffer = VK_NULL_HANDLE;
    VkDeviceMemory accMemory = VK_NULL_HANDLE;
    // Per-tile monopoles for the hybrid far-field solver (vec4: COM.xyz + mass)
    VkBuffer tileComBuffer = VK_NULL_HANDLE;
    VkDeviceMemory tileComMemory = VK_NULL_HANDLE;
    VkPipeline tileComPipeline = VK_NULL_HANDLE;
    // Per-workgroup diagnostic partial sums (host-visible, 2 vec4 per group)
    VkBuffer diagBuffer = VK_NULL_HANDLE;
    VkDeviceMemory diagMemory = VK_NULL_HANDLE;

    // Diagnostics (energy / momentum instruments)
    bool showInstruments = true;
    bool instrMinimized = false;   // instruments docked as a top-bar chip
    bool gwMinimized = false;      // GW scope docked as a bottom-bar chip
    // Hovered panel control: 1=instr minimize, 2=instr restore chip,
    // 3=gw minimize, 4=gw restore chip (-1 = none)
    int hoverPanelBtn = -1;
    struct DiagStats {
        double ke = 0, pe = 0, mass = 0, count = 0;
        double px = 0, py = 0, pz = 0, speedSum = 0;
        bool valid = false;
    } diag;
    double energyBaseline = 0.0;
    double momentumBaseline[3] = {0, 0, 0};
    bool energyBaselineSet = false;
    std::vector<float> energyHistory;   // recent total-energy samples for the sparkline
    int diagFrameCounter = 0;

    // Gravitational wave observatory: quadrupole formula h ~ d^2 Q / dt^2.
    // Three-sample ring for the finite-difference second derivative.
    double gwQ[3][6] = {{0}};
    double gwDtHist[2] = {0, 0};
    int gwSamples = 0;
    double gwSmoothP = 0, gwSmoothX = 0;
    std::vector<float> gwPlus, gwCross;   // strain traces for the scope

    // Accretion: bound close pairs merge into one body (momentum-conserving)
    bool mergeEnabled = false;
    int mergeCount = 0;
    uint32_t lastMergePass = 0;

    // Spatial (Morton) sort keeps the 256-particle compute tiles spatially
    // compact so the far-field tile monopoles are accurate
    uint32_t lastSortPass = 0;

    // Black hole tracking for the composite lensing/shadow renderer.
    // Positions are rescanned periodically and extrapolated per frame.
    struct BHInfo { float x, y, z, mass; int gid; float charge, spin; int type; };
    std::vector<BHInfo> blackHoles;
    std::vector<int32_t> lastBHSimGids;   // gid order of the last BHSim upload
    bool bhMaybePresent = false;
    uint32_t lastBHScan = 0;
    VkBuffer bhOutBuffer = VK_NULL_HANDLE;
    VkDeviceMemory bhOutMemory = VK_NULL_HANDLE;
    VkBuffer bhSimBuffer = VK_NULL_HANDLE;
    VkDeviceMemory bhSimMemory = VK_NULL_HANDLE;

    // Spawn tuning for middle-click black holes
    float blackHoleCharge = 0.0f;   // in units of e
    float blackHoleSpin = 0.0f;     // -1..1, frame-dragging strength/direction

    // Black holes placed by presets (appended by initParticles)
    struct PresetBH { float x, y, z, vx, vy, vz, mass, spin, charge;
                     int ptype = PTYPE_BLACKHOLE; };
    std::vector<PresetBH> presetBHs;
    VkBuffer compositeUboBuffer = VK_NULL_HANDLE;
    VkDeviceMemory compositeUboMemory = VK_NULL_HANDLE;

    // Deep-sky starfield background
    bool starfieldEnabled = true;

    // UI state for the preset dropdown
    int currentPreset = 0;

    // Screenshot / headless-run control
    bool screenshotRequested = false;
    uint32_t lastPresentedImage = 0;
    long maxFrames = -1;          // exit after this many frames (CLI --frames)
    float fixedDt = -1.0f;        // deterministic timestep override (CLI --fixed-dt)
    bool autoScreenshot = false;  // capture a frame right before a --frames exit

    // Frame timing
    float frameDeltaTime = 0.016f;  // Actual frame delta time (smoothed)
    uint32_t lastFrameTime = 0;     // SDL_GetTicks of last frame

    // Stats
    double fps = 0.0;
    uint32_t frameCount = 0;
    uint32_t fpsTimer = 0;
    char gpuName[256] = "Unknown GPU";

    // Text rendering
    TTF_Font* font = nullptr;
    bool menuDirty = true;

    // Vulkan text overlay
    VkImage fontImage = VK_NULL_HANDLE;
    VkDeviceMemory fontMemory = VK_NULL_HANDLE;
    VkImageView fontImageView = VK_NULL_HANDLE;
    VkSampler fontSampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout textDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet textDescSet = VK_NULL_HANDLE;
    VkPipelineLayout textPipelineLayout = VK_NULL_HANDLE;
    VkPipeline textPipeline = VK_NULL_HANDLE;
    VkBuffer textVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory textVertexMemory = VK_NULL_HANDLE;
    int textVertexCount = 0;
    int fontTexWidth = 0, fontTexHeight = 0;
    int fontCharWidth = 0, fontCharHeight = 0;
};

extern App app;

#define MATTER_COUNT 6

// util.cpp
float randFloat();
float randRange(float min, float max);
float randGaussian();
void randUnitVector(float& x, float& y, float& z);
void hsv2rgb(float h, float s, float v, float& r, float& g, float& b);
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props,
                  VkBuffer& buffer, VkDeviceMemory& memory, bool concurrent = false);
std::vector<char> readFile(const std::string& filename);
VkShaderModule createShaderModule(const std::vector<char>& code);

// font.cpp
bool createFontTexture();
bool createTextPipeline();

// ui.cpp
void buildMenuText(std::vector<TextVertex>& vertices);
void renderMenu();
void printHelp();

// vulkan_setup.cpp
void cleanupHDRResources();
void createHDRResources();
void updateCompositeDescriptor();
void cleanupSwapchain();
void createSwapchain();
void recreateSwapchain();
bool initVulkan();
void cleanup();

// sim.cpp
void applyPreset(int preset);
bool getCenterOfMass(float& comX, float& comY, float& comZ, float* outBoundingRadius = nullptr);
void spawnBlackHole(float worldX, float worldY, float worldZ,
                    float velX = 0.0f, float velY = 0.0f, float velZ = 0.0f);
void centerCameraOnParticles();
bool readbackParticles(std::vector<Particle>& out, int count);
void uploadParticles(const std::vector<Particle>& particles);
void resetTrails(const std::vector<Particle>& particles);
void mortonSort(std::vector<Particle>& parts, std::vector<int>* outPerm = nullptr);
void permuteTrailRows(const std::vector<int>& srcIndex, bool syncGpu = true);
void syncTrailRowMap();
void initParticles();
void writeSimUBO(float stepDt);
void updateUniforms();
void runCompute();
void readDiagnostics(bool force = false);
void runMergePass();
void runSortStateMachine();
void computeMortonPerm(const std::vector<Particle>& parts, std::vector<int32_t>& perm);
void saveState();
void loadState();

// render.cpp
void renderFrame();
void captureScreenshot();

// input.cpp
void handleEvents();
