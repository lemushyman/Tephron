#include "common.h"

float screenToNdcX(int x) { return (float)x / app.swapchainExtent.width * 2.0f - 1.0f; }
float screenToNdcY(int y) { return (float)y / app.swapchainExtent.height * 2.0f - 1.0f; }

// ============================================================
// UI THEME - "observatory HUD"
// Glassy blue-black gradient panels, crisp 1px hairlines, dim
// labels with bright cyan values, photon-ring amber for active
// and selected states. All rects are hard-edged (no AA) to stay
// true to the bitmap-font aesthetic.
// ============================================================
namespace theme {
    // panel gradient (top -> bottom)
    constexpr float panelTop[4] = {0.045f, 0.075f, 0.13f, 0.90f};
    constexpr float panelBot[4] = {0.010f, 0.016f, 0.035f, 0.90f};
    // accents
    constexpr float cyan[3]     = {0.30f, 0.85f, 1.00f};
    constexpr float amber[3]    = {1.00f, 0.65f, 0.28f};
    // text
    constexpr float bright[3]   = {0.93f, 0.97f, 1.00f};
    constexpr float value[3]    = {0.72f, 0.92f, 1.00f};
    constexpr float dim[3]      = {0.42f, 0.52f, 0.66f};
    // status
    constexpr float good[3]     = {0.38f, 0.95f, 0.55f};
    constexpr float warn[3]     = {1.00f, 0.75f, 0.30f};
    constexpr float bad[3]      = {1.00f, 0.40f, 0.35f};
}

void buildMenuText(std::vector<TextVertex>& vertices) {
    vertices.clear();
    if ((!app.showMenu && !app.showInstruments) || app.fontCharWidth == 0) return;

    // ---- Menu content (defined up front so the scale fits it exactly) ----
    struct MenuDef {
        const char* title;
        const char* shortcut;  // Keyboard shortcut to display
        int itemCount;
        const char** items;
        int* value;
        bool* boolValue;  // For toggle items
        bool isToggle;
    };
    static const char* gravityItems[] = {"Newtonian", "Inv Linear", "Linear", "Constant",
                                   "Repulsive", "Oscillating", "Inv Cube", "Logarithmic",
                                   "Yukawa", "Lennard-Jones", "Spiral", "Pulse",
                                   "Quantized", "Funnel", "Vortex", "Rubber Band"};
    static const char* distItems[] = {"Sphere", "Disk", "Shell", "Random", "Binary", "Solar",
                               "Ring", "Cluster", "Filament", "Explosion", "Vortex", "Grid",
                               "Plummer", "Figure-8", "Accretion", "Quasar", "Tidal",
                               "Halo"};
    static const char* massItems[] = {"Uniform", "Random", "Central", "Binary", "Hierarchical",
                               "Power Law", "Stars+Gas", "Clusters"};
    static const char* forceItems[] = {"OFF", "ON"};
    static const char* matterItems[] = {"Generic", "Atoms", "Plasma", "Quarks", "Mixed", "Fundamental"};

    MenuDef menus[] = {
        {"Grav", "G", GRAVITY_COUNT, gravityItems, &app.gravityType, nullptr, false},
        {"Dist", "F", DIST_COUNT, distItems, &app.distribution, nullptr, false},
        {"Mass", "M", MASS_COUNT, massItems, &app.massMode, nullptr, false},
        {"Col", "C", COLOR_COUNT, colorNames, &app.colorMode, nullptr, false},
        {"Mat", "P", MATTER_COUNT, matterItems, &app.particleMode, nullptr, false},
        {"Pre", "1-0", PRESET_COUNT, presetNames, &app.currentPreset, nullptr, false},
        {"EM", "F2", 2, forceItems, nullptr, &app.emEnabled, true},
        {"Str", "F3", 2, forceItems, nullptr, &app.strongForceEnabled, true},
        {"Rel", "F5", 2, forceItems, nullptr, &app.relativityEnabled, true},
        {"Exp", "F6", 2, forceItems, nullptr, &app.expansionEnabled, true},
        {"Auto", "F9", 2, forceItems, nullptr, &app.displayMode, true},
    };
    int numMenus = 11;

    static const char* row2Names[] = {
        "G", "Soft", "Time", "Part", "H", "c", "Lens", "Bnd", "Trail", "Dark",
        "Glow", "BHm", "Mrg", "Sky", "BHq", "BHs"
    };
    // Reserved value widths (worst case per entry) keep the bar static
    // while scrolling: G 200 / Soft 50.0 / Time 5.0x / Part 500.0k /
    // H 0.100 / c 2000 / Lens 3.0 / Bnd Reflect / ON-OFF / Glow 2.00 /
    // BHm 1000k / Mrg ON(999999) / Sky / BHq +50 / BHs +1.0
    static const int row2MaxVal[16] = {3, 4, 4, 6, 5, 4, 3, 7, 3, 3,
                                       4, 5, 10, 3, 3, 4};

    // Per-menu longest option (full length - values are never truncated)
    int menuMaxVal[16];
    for (int m = 0; m < numMenus; m++) {
        int mv = 3;   // "OFF"
        if (!menus[m].isToggle)
            for (int i = 0; i < menus[m].itemCount; i++)
                mv = std::max(mv, (int)strlen(menus[m].items[i]));
        menuMaxVal[m] = mv;
    }

    // ---- Font size + bar layout: quantum steps through Spleen ----
    // Pick the largest native size whose bars fit in ONE row each; if
    // even the smallest can't, allow TWO rows per bar (wrap, never hide)
    // and again take the largest that fits. Native bitmaps only.
    float topWc[16], botWc[16];
    for (int m = 0; m < numMenus; m++) {
        char lbuf[32];
        snprintf(lbuf, sizeof(lbuf), "%s(%s):", menus[m].title, menus[m].shortcut);
        topWc[m] = (int)strlen(lbuf) + menuMaxVal[m] + 0.8f;   // + pad
    }
    for (int v = 0; v < 16; v++)
        botWc[v] = (float)((int)strlen(row2Names[v]) + 1 + row2MaxVal[v]);
    auto rowsFor = [&](const float* wArr, int n, float gap, float first, float rest) {
        int rows = 1;
        float x = 0, avail = first;
        for (int i = 0; i < n; i++) {
            float need = wArr[i] + (x > 0 ? gap : 0.0f);
            if (x > 0 && x + need > avail) { rows++; avail = rest; x = wArr[i]; }
            else x += need;
        }
        return rows;
    };
    int sw = (int)app.swapchainExtent.width, sh = (int)app.swapchainExtent.height;
    int pick = 0, topRows = 2, botRows = 2;
    for (int maxRows = 1; maxRows <= 2; maxRows++) {
        bool found = false;
        for (int s = SPLEEN_SIZES - 1; s >= 0 && !found; s--) {
            float ca = (float)sw / spleenW[s];   // window width in chars
            int tr = rowsFor(topWc, numMenus, 0.3f, ca - 30.0f, ca - 2.0f);
            int br = rowsFor(botWc, 16, 0.8f, ca - 17.0f, ca - 17.0f);
            bool heightOK = (1.5f * tr + 1.3f * br) * spleenH[s] <= 0.30f * sh;
            if (tr <= maxRows && br <= maxRows && heightOK) {
                pick = s; topRows = tr; botRows = br; found = true;
            }
        }
        if (found) break;
        if (maxRows == 2) { pick = 0; topRows = 2; botRows = 2; }  // elide fallback
    }
    app.fontSizeIdx = pick;
    app.fontCharWidth = spleenW[pick];
    app.fontCharHeight = spleenH[pick];
    float charW = 2.0f * spleenW[pick] / sw;
    float charH = 2.0f * spleenH[pick] / sh;
    float px = std::max(1, spleenH[pick] / 12) * 2.0f / sh;  // hairline
    float topRowHt = charH * 1.5f;   // menu-bar row height
    float botRowHt = charH * 1.3f;   // variable-bar row height
    float row0Bot = -1.0f + topRowHt;

    // Convert mouse position to NDC
    float mouseNdcX = screenToNdcX(app.mouseX);
    float mouseNdcY = screenToNdcY(app.mouseY);
    app.hoverPanelBtn = -1;

    auto addChar = [&](float x, float y, char c, float r, float g, float b, float a) {
        if (c < 32 || c > 126) c = '?';
        int gi = c - 32;
        int col = gi % 16, grow = gi / 16;
        int fw = spleenW[app.fontSizeIdx], fh = spleenH[app.fontSizeIdx];
        float u0 = (float)(col * fw) / app.fontTexWidth;
        float v0 = (float)(app.fontSizeYOff[app.fontSizeIdx] + grow * fh) / app.fontTexHeight;
        float u1 = u0 + (float)fw / app.fontTexWidth;
        float v1 = v0 + (float)fh / app.fontTexHeight;

        // Snap the glyph origin to the pixel grid so identical characters
        // always sample the atlas identically (crisp at any scale)
        x = floorf((x + 1.0f) * 0.5f * app.swapchainExtent.width + 0.5f)
            / app.swapchainExtent.width * 2.0f - 1.0f;
        y = floorf((y + 1.0f) * 0.5f * app.swapchainExtent.height + 0.5f)
            / app.swapchainExtent.height * 2.0f - 1.0f;
        vertices.push_back({x, y, u0, v0, r, g, b, a});
        vertices.push_back({x + charW, y, u1, v0, r, g, b, a});
        vertices.push_back({x, y + charH, u0, v1, r, g, b, a});
        vertices.push_back({x + charW, y, u1, v0, r, g, b, a});
        vertices.push_back({x + charW, y + charH, u1, v1, r, g, b, a});
        vertices.push_back({x, y + charH, u0, v1, r, g, b, a});
    };

    auto addText = [&](float x, float y, const char* text, const float* c, float a) {
        while (*text) {
            addChar(x, y, *text, c[0], c[1], c[2], a);
            x += charW;
            text++;
        }
    };

    // Vertical gradient rectangle (top color -> bottom color)
    auto addRectGrad = [&](float x1, float y1, float x2, float y2,
                           const float* top, const float* bot) {
        vertices.push_back({x1, y1, 0, 0, top[0], top[1], top[2], top[3]});
        vertices.push_back({x2, y1, 0, 0, top[0], top[1], top[2], top[3]});
        vertices.push_back({x1, y2, 0, 0, bot[0], bot[1], bot[2], bot[3]});
        vertices.push_back({x2, y1, 0, 0, top[0], top[1], top[2], top[3]});
        vertices.push_back({x2, y2, 0, 0, bot[0], bot[1], bot[2], bot[3]});
        vertices.push_back({x1, y2, 0, 0, bot[0], bot[1], bot[2], bot[3]});
    };

    auto addRect = [&](float x1, float y1, float x2, float y2,
                       float r, float g, float b, float a) {
        float c[4] = {r, g, b, a};
        addRectGrad(x1, y1, x2, y2, c, c);
    };

    // 1px horizontal hairline
    auto addHLine = [&](float x1, float x2, float y, const float* c, float a) {
        addRect(x1, y, x2, y + px, c[0], c[1], c[2], a);
    };

    // Soft vertical drop shadow (fades to transparent toward yFar)
    auto addShadow = [&](float x1, float x2, float yNear, float yFar) {
        float s0[4] = {0.0f, 0.0f, 0.0f, 0.45f};
        float s1[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        if (yFar > yNear) addRectGrad(x1, yNear, x2, yFar, s0, s1);
        else              addRectGrad(x1, yFar, x2, yNear, s1, s0);
    };

    auto isHovered = [&](float x1, float y1, float x2, float y2) -> bool {
        return mouseNdcX >= x1 && mouseNdcX <= x2 && mouseNdcY >= y1 && mouseNdcY <= y2;
    };

    // ============================================================
    // INSTRUMENTS PANEL (I key): live energy/momentum diagnostics
    // with a scrolling total-energy sparkline. dE is the drift
    // since the last reset - the integrator's report card.
    // ============================================================
    if (app.showInstruments && !app.instrMinimized && app.diag.valid) {
        double E = app.diag.ke + app.diag.pe;
        double drift = 0.0;
        if (app.energyBaselineSet && fabs(app.energyBaseline) > 1e-9)
            drift = (E - app.energyBaseline) / fabs(app.energyBaseline) * 100.0;
        double pMag = sqrt(app.diag.px * app.diag.px + app.diag.py * app.diag.py +
                           app.diag.pz * app.diag.pz);
        double vir = (app.diag.pe != 0.0) ? 2.0 * app.diag.ke / fabs(app.diag.pe) : 0.0;

        // dE color: green = conserved, amber = drifting, red = check your dt
        float dr = fabsf((float)drift);
        const float* driftCol = dr < 1.0f ? theme::good : (dr < 5.0f ? theme::warn : theme::bad);

        float panelW = 30 * charW;
        float headerH = charH * 1.5f;
        float graphH = charH * 3.5f;
        float panelH = headerH + charH * 1.25f * 3.0f + graphH + charH * 1.0f;
        float x2 = 1.0f - charW * 0.5f;
        float x1 = x2 - panelW;
        float y2 = 1.0f - botRowHt * botRows - charH * 0.6f;   // above the bottom bar
        float y1 = y2 - panelH;

        addRectGrad(x1, y1, x2, y2, theme::panelTop, theme::panelBot);
        addHLine(x1, x2, y1, theme::cyan, 0.55f);          // top accent
        addHLine(x1, x2, y2 - px, theme::cyan, 0.18f);     // bottom hairline

        // Header strip + minimize button (docks to the top bar)
        addRect(x1, y1 + px, x2, y1 + headerH, 0.10f, 0.16f, 0.26f, 0.55f);
        addText(x1 + charW, y1 + charH * 0.22f, "INSTRUMENTS", theme::cyan, 0.85f);
        {
            float bx1 = x2 - charW * 2.0f, bx2 = x2 - charW * 0.4f;
            bool bHov = isHovered(bx1, y1, bx2, y1 + headerH);
            if (bHov) {
                app.hoverPanelBtn = 1;
                addRect(bx1, y1 + px, bx2, y1 + headerH,
                        theme::cyan[0], theme::cyan[1], theme::cyan[2], 0.25f);
            }
            addChar(bx1 + charW * 0.3f, y1 + charH * 0.22f, '-',
                    theme::bright[0], theme::bright[1], theme::bright[2], bHov ? 1.0f : 0.7f);
        }
        char driftBuf[24];
        snprintf(driftBuf, sizeof(driftBuf), "dE %+.2f%%", drift);
        addText(x2 - (strlen(driftBuf) + 3) * charW, y1 + charH * 0.22f, driftBuf, driftCol, 1.0f);

        // Label/value readout lines
        float ty = y1 + headerH + charH * 0.30f;
        auto addKV = [&](float& cx, float y, const char* k, const char* v, const float* vc) {
            addText(cx, y, k, theme::dim, 1.0f);
            cx += strlen(k) * charW;
            addText(cx, y, v, vc, 1.0f);
            cx += (strlen(v) + 1) * charW;
        };
        char vb[32];
        float cx = x1 + charW;
        snprintf(vb, sizeof(vb), "%.4g", E);
        addKV(cx, ty, "E ", vb, driftCol);
        snprintf(vb, sizeof(vb), "%.2f", vir);
        addKV(cx, ty, "Vir ", vb, theme::value);
        ty += charH * 1.25f;
        cx = x1 + charW;
        snprintf(vb, sizeof(vb), "%.3g", app.diag.ke);
        addKV(cx, ty, "KE ", vb, theme::value);
        snprintf(vb, sizeof(vb), "%.3g", app.diag.pe);
        addKV(cx, ty, "PE ", vb, theme::value);
        ty += charH * 1.25f;
        cx = x1 + charW;
        snprintf(vb, sizeof(vb), "%.3g", pMag);
        addKV(cx, ty, "|P| ", vb, theme::value);
        snprintf(vb, sizeof(vb), "%d", (int)app.diag.count);
        addKV(cx, ty, "N ", vb, theme::value);
        snprintf(vb, sizeof(vb), "%d", app.mergeCount);
        addKV(cx, ty, "Mrg ", vb, app.mergeEnabled ? theme::amber : theme::value);
        ty += charH * 1.25f + charH * 0.35f;

        // Energy sparkline: newest sample at the right edge
        if (app.energyHistory.size() >= 2) {
            float gTop = ty;
            float gBot = y2 - charH * 0.35f;
            float gx1 = x1 + charW * 0.5f;
            float gx2 = x2 - charW * 0.5f;

            // Faint frame + midline gridline
            addRect(gx1, gTop, gx2, gBot, 0.0f, 0.0f, 0.0f, 0.35f);
            addHLine(gx1, gx2, (gTop + gBot) * 0.5f, theme::dim, 0.30f);

            float eMin = app.energyHistory[0], eMax = app.energyHistory[0];
            for (float e : app.energyHistory) { eMin = std::min(eMin, e); eMax = std::max(eMax, e); }
            float range = std::max(eMax - eMin, fabsf(eMax) * 0.002f + 1e-12f);
            float mid = 0.5f * (eMax + eMin);

            const int maxSamples = 150;
            float colW = (gx2 - gx1) / maxSamples;
            int n = (int)app.energyHistory.size();
            for (int k = 0; k < n; k++) {
                float e = app.energyHistory[k];
                float t = ((e - mid) / range) + 0.5f;   // 0..1 centered on the mean
                t = std::clamp(t, 0.0f, 1.0f);
                float cy0 = gBot - t * (gBot - gTop);
                float gx = gx1 + (maxSamples - n + k) * colW;
                bool newest = (k == n - 1);
                addRect(gx, cy0 - charH * 0.06f, gx + colW * 0.85f, cy0 + charH * 0.06f,
                        driftCol[0], driftCol[1], driftCol[2], newest ? 1.0f : 0.75f);
                if (newest)   // brighter head marker on the latest sample
                    addRect(gx, cy0 - charH * 0.14f, gx + colW * 0.85f, cy0 + charH * 0.14f,
                            theme::bright[0], theme::bright[1], theme::bright[2], 0.9f);
            }
        }
    }

    // ============================================================
    // GW OBSERVATORY (bottom-left): strain traces from the quadrupole
    // formula, h+ in cyan and hx in amber. A binary inspiral produces
    // a textbook chirp here; a quiet cluster stays flat.
    // ============================================================
    if (app.showInstruments && !app.gwMinimized && app.gwPlus.size() >= 2) {
        float panelW = 34 * charW;
        float headerH = charH * 1.5f;
        float scopeH = charH * 5.0f;
        float panelH = headerH + scopeH + charH * 0.7f;
        float x1 = -1.0f + charW * 0.5f;
        float x2 = x1 + panelW;
        float y2 = 1.0f - charH * 1.9f;
        float y1 = y2 - panelH;

        addRectGrad(x1, y1, x2, y2, theme::panelTop, theme::panelBot);
        addHLine(x1, x2, y1, theme::cyan, 0.55f);
        addHLine(x1, x2, y2 - px, theme::cyan, 0.18f);
        addRect(x1, y1 + px, x2, y1 + headerH, 0.10f, 0.16f, 0.26f, 0.55f);
        addText(x1 + charW, y1 + charH * 0.22f, "GW OBSERVATORY", theme::cyan, 0.85f);
        {
            float bx1 = x2 - charW * 2.0f, bx2 = x2 - charW * 0.4f;
            bool bHov = isHovered(bx1, y1, bx2, y1 + headerH);
            if (bHov) {
                app.hoverPanelBtn = 3;
                addRect(bx1, y1 + px, bx2, y1 + headerH,
                        theme::cyan[0], theme::cyan[1], theme::cyan[2], 0.25f);
            }
            addChar(bx1 + charW * 0.3f, y1 + charH * 0.22f, '-',
                    theme::bright[0], theme::bright[1], theme::bright[2], bHov ? 1.0f : 0.7f);
        }

        float peak = 1e-12f;
        for (float h : app.gwPlus) peak = std::max(peak, fabsf(h));
        for (float h : app.gwCross) peak = std::max(peak, fabsf(h));
        char peakBuf[24];
        snprintf(peakBuf, sizeof(peakBuf), "h %.2g", peak);
        addText(x2 - (strlen(peakBuf) + 3) * charW, y1 + charH * 0.22f, peakBuf, theme::dim, 1.0f);

        float gTop = y1 + headerH + charH * 0.35f;
        float gBot = y2 - charH * 0.35f;
        float gx1 = x1 + charW * 0.5f;
        float gx2 = x2 - charW * 0.5f;
        addRect(gx1, gTop, gx2, gBot, 0.0f, 0.0f, 0.0f, 0.35f);
        float mid = 0.5f * (gTop + gBot);
        addHLine(gx1, gx2, mid, theme::dim, 0.30f);

        // Continuous scope traces: each column spans from the previous
        // sample's level to the current one (full width, no gaps), so the
        // waveform reads as a connected line instead of striped dots
        const int maxSamples = 260;
        float colW = (gx2 - gx1) / maxSamples;
        int n = (int)app.gwPlus.size();
        float halfSpan = 0.5f * (gBot - gTop) * 0.92f;
        auto trace = [&](const std::vector<float>& h, const float* col, float alpha) {
            float prevY = mid - std::clamp(h[0] / peak, -1.0f, 1.0f) * halfSpan;
            for (int k = 0; k < n; k++) {
                float x = gx1 + (maxSamples - n + k) * colW;
                float y = mid - std::clamp(h[k] / peak, -1.0f, 1.0f) * halfSpan;
                float yLo = std::min(prevY, y) - charH * 0.05f;
                float yHi = std::max(prevY, y) + charH * 0.05f;
                addRect(x, yLo, x + colW, yHi, col[0], col[1], col[2], alpha);
                prevY = y;
            }
        };
        trace(app.gwPlus, theme::cyan, 0.9f);
        trace(app.gwCross, theme::amber, 0.7f);
    }

    if (!app.showMenu) return;

    // ============================================================
    // TOP MENU BAR - glass gradient, hairline border, drop shadow
    // ============================================================
    float menuBarHeight = topRowHt * topRows;
    float menuBarTop = -1.0f + menuBarHeight;  // Top of screen in NDC is -1 (inverted Y)

    addRectGrad(-1.0f, -1.0f, 1.0f, menuBarTop, theme::panelTop, theme::panelBot);
    addHLine(-1.0f, 1.0f, menuBarTop - px, theme::cyan, 0.45f);
    addShadow(-1.0f, 1.0f, menuBarTop, menuBarTop + charH * 0.6f);

    // Menu items
    float menuX = -1.0f + charW;
    float menuY = -1.0f + charH * 0.25f;
    float itemPadding = charW * 0.8f;

    char buf[128];

    app.hoverMenu = -1;
    app.hoverDropdownItem = -1;
    app.hoverRow2Var = -1;

    // Store dropdown info for deferred drawing (so it appears on top of row 2)
    struct DropdownInfo {
        float x, y, width, height;
        int menuIdx;
        bool valid;
    };
    DropdownInfo pendingDropdown = {0, 0, 0, 0, -1, false};

    // Right edge available to menu items: stats block, plus the INS chip
    // when it is docked up here. Items that no longer fit are elided into
    // a ">>" marker instead of overlapping (keyboard shortcuts still work
    // for hidden items).
    float menuLimit = 1.0f - 14.0f * charW - 13.0f * charW - charW;

    // Justify: spread the leftover width evenly between items so spacing
    // is uniform (depends only on reserved widths + window, never values)
    float slotsW = 0;
    for (int m = 0; m < numMenus; m++) {
        char lbuf[32];
        snprintf(lbuf, sizeof(lbuf), "%s(%s):", menus[m].title, menus[m].shortcut);
        slotsW += (strlen(lbuf) + menuMaxVal[m]) * charW + itemPadding;
    }
    float menuGap = std::clamp((menuLimit - (-1.0f + charW) - slotsW) / (numMenus - 1),
                               charW * 0.3f, charW * 6.0f);
    if (topRows > 1) menuGap = charW * 0.6f;
    int curRow = 0;

    // Draw menu bar items. Each slot reserves room for the LONGEST value
    // in its option set, so scrolling through options never shifts the
    // position of neighbouring items.
    for (int m = 0; m < numMenus; m++) {
        const char* currentValue;
        bool toggleOn = menus[m].isToggle && *menus[m].boolValue;
        if (menus[m].isToggle) {
            currentValue = toggleOn ? "ON" : "OFF";
        } else {
            currentValue = menus[m].items[*menus[m].value];
        }

        // Slot reserves the full longest option - nothing is truncated;
        // the UI scale guarantees the whole bar fits
        int maxValLen = menuMaxVal[m];

        // Layout: "Title(Key):Value" - label dim, value bright
        snprintf(buf, sizeof(buf), "%s(%s):", menus[m].title, menus[m].shortcut);
        int labelLen = strlen(buf);
        float itemWidth = (labelLen + maxValLen) * charW + itemPadding;

        float x1 = menuX;
        float x2 = menuX + itemWidth;
        float limit = (curRow == 0) ? menuLimit : 1.0f - charW;
        if (x2 > limit + charW * 0.6f) {
            if (curRow + 1 < topRows) {          // wrap onto the next bar row
                curRow++;
                menuX = -1.0f + charW;
                x1 = menuX;
                x2 = menuX + itemWidth;
            } else {                             // out of room - elide the rest
                addText(menuX, -1.0f + curRow * topRowHt + charH * 0.25f,
                        ">>", theme::dim, 1.0f);
                break;
            }
        }
        float y1 = -1.0f + curRow * topRowHt;
        float y2 = y1 + topRowHt;
        float itemTextY = y1 + charH * 0.25f;

        bool hovered = isHovered(x1, y1, x2, y2);
        bool isOpen = (app.openMenu == m);

        if (hovered) app.hoverMenu = m;

        // Hover/open treatment: cyan glass fill + accent underline
        if (isOpen) {
            addRect(x1, y1, x2, y2, theme::cyan[0], theme::cyan[1], theme::cyan[2], 0.14f);
            addRect(x1, y2 - 2.0f * px, x2, y2,
                    theme::amber[0], theme::amber[1], theme::amber[2], 0.95f);
        } else if (hovered) {
            addRect(x1, y1, x2, y2, theme::cyan[0], theme::cyan[1], theme::cyan[2], 0.08f);
            addRect(x1, y2 - 2.0f * px, x2, y2,
                    theme::cyan[0], theme::cyan[1], theme::cyan[2], 0.75f);
        }

        float tx = menuX + charW * 0.75f;
        addText(tx, itemTextY, buf, (hovered || isOpen) ? theme::bright : theme::dim, 1.0f);
        tx += labelLen * charW;
        const float* valCol = menus[m].isToggle
            ? (toggleOn ? theme::amber : theme::dim)
            : theme::value;
        addText(tx, itemTextY, currentValue, valCol, 1.0f);

        // Store dropdown info for later drawing (if open)
        if (isOpen && !menus[m].isToggle) {
            float dropX = x1;
            float dropY = menuBarTop;
            float dropWidth = 0;
            int itemCount = menus[m].itemCount;
            for (int i = 0; i < itemCount; i++) {
                float w = strlen(menus[m].items[i]) * charW + charW * 3.5f;
                if (w > dropWidth) dropWidth = w;
            }
            float dropHeight = itemCount * charH * 1.2f + charH * 0.4f;
            pendingDropdown = {dropX, dropY, dropWidth, dropHeight, m, true};
        }

        menuX += itemWidth + menuGap;
    }

    // Stats readout (right side of menu bar)
    char fpsBuf[24], partBuf[16];
    snprintf(fpsBuf, sizeof(fpsBuf), app.paused ? "PAUSED" : "%.0f FPS", app.fps);
    snprintf(partBuf, sizeof(partBuf), " %dk", app.numParticles / 1000);
    float statsWidth = 14 * charW;   // fits "999 FPS 500k" - fixed so the
    float statsX = 1.0f - statsWidth; // right edge never wanders
    addText(statsX, menuY, fpsBuf, app.paused ? theme::warn : theme::good, 1.0f);
    addText(statsX + strlen(fpsBuf) * charW, menuY, partBuf, theme::dim, 1.0f);

    // Minimized instruments: a live drift chip docked here; click to restore
    if (app.showInstruments && app.instrMinimized && app.diag.valid) {
        double E = app.diag.ke + app.diag.pe;
        double drift = 0.0;
        if (app.energyBaselineSet && fabs(app.energyBaseline) > 1e-9)
            drift = (E - app.energyBaseline) / fabs(app.energyBaseline) * 100.0;
        float dr = fabsf((float)drift);
        const float* dc = dr < 1.0f ? theme::good : (dr < 5.0f ? theme::warn : theme::bad);
        char chip[32];
        snprintf(chip, sizeof(chip), "dE%+.1f%%", drift);
        float chipW = (strlen(chip) + 4) * charW;
        float cx1 = statsX - chipW - charW;
        bool cHov = isHovered(cx1, -1.0f, cx1 + chipW, row0Bot);
        if (cHov) {
            app.hoverPanelBtn = 2;
            addRect(cx1, -1.0f, cx1 + chipW, row0Bot,
                    theme::cyan[0], theme::cyan[1], theme::cyan[2], 0.12f);
            addRect(cx1, row0Bot - 2.0f * px, cx1 + chipW, row0Bot,
                    theme::cyan[0], theme::cyan[1], theme::cyan[2], 0.75f);
        }
        addText(cx1 + charW * 0.5f, menuY, "INS", cHov ? theme::bright : theme::dim, 1.0f);
        addText(cx1 + charW * 4.0f, menuY, chip, dc, 1.0f);
    }

    // ============================================================
    // BOTTOM VARIABLE BAR - scroll over a value to adjust it
    // ============================================================
    float row2Height = botRowHt * botRows;
    float row2Bottom = 1.0f;  // Bottom of screen in NDC
    float row2Y = row2Bottom - row2Height;

    addRectGrad(-1.0f, row2Y, 1.0f, row2Bottom, theme::panelTop, theme::panelBot);
    addHLine(-1.0f, 1.0f, row2Y, theme::cyan, 0.45f);
    addShadow(-1.0f, 1.0f, row2Y, row2Y - charH * 0.6f);

    const char* boundaryNames[] = {"None", "Reflect", "Wrap"};

    // Build row2 items individually so we can track hover
    // (index corresponds to hoverRow2Var: 0=G ... 15=BH spin)
    float row2X = -1.0f + charW * 0.75f;
    float row2TextY = row2Y + charH * 0.2f;

    float row2Limit = 1.0f - 14.0f * charW - charW;
    float row2Slots = 0;
    for (int v = 0; v < 16; v++)
        row2Slots += (strlen(row2Names[v]) + 1 + row2MaxVal[v]) * charW;
    float row2Gap = std::clamp((row2Limit - (-1.0f + charW * 0.75f) - row2Slots) / 15.0f,
                               charW * 0.8f, charW * 6.0f);
    if (botRows > 1) row2Gap = charW * 0.8f;
    int curRow2 = 0;

    for (int v = 0; v < 16; v++) {
        char valBuf[32];
        bool isOn = false, isToggle = false;
        switch (v) {
            case 0:  snprintf(valBuf, sizeof(valBuf), "%.0f", app.G); break;
            case 1:  snprintf(valBuf, sizeof(valBuf), "%.1f", app.softening); break;
            case 2:  snprintf(valBuf, sizeof(valBuf), "%.1fx", app.timeScale); break;
            case 3:  snprintf(valBuf, sizeof(valBuf), "%.1fk", app.numParticles / 1000.0f); break;
            case 4:  snprintf(valBuf, sizeof(valBuf), "%.3f", app.hubbleConstant); break;
            case 5:  snprintf(valBuf, sizeof(valBuf), "%.0f", app.speedOfLight); break;
            case 6:  snprintf(valBuf, sizeof(valBuf), "%.1f", app.lensingStrength); break;
            case 7:  snprintf(valBuf, sizeof(valBuf), "%s", boundaryNames[app.boundaryMode]); break;
            case 8:  isToggle = true; isOn = app.trailsEnabled;
                     snprintf(valBuf, sizeof(valBuf), "%s", isOn ? "ON" : "OFF"); break;
            case 9:  isToggle = true; isOn = app.darkMatter;
                     snprintf(valBuf, sizeof(valBuf), "%s", isOn ? "ON" : "OFF"); break;
            case 10: snprintf(valBuf, sizeof(valBuf), "%.2f", app.glowIntensity); break;
            case 11: snprintf(valBuf, sizeof(valBuf), "%.0fk", app.blackHoleMass / 1000.0f); break;
            case 12: isToggle = true; isOn = app.mergeEnabled;
                     if (isOn) snprintf(valBuf, sizeof(valBuf), "ON(%d)", app.mergeCount);
                     else snprintf(valBuf, sizeof(valBuf), "OFF");
                     break;
            case 13: isToggle = true; isOn = app.starfieldEnabled;
                     snprintf(valBuf, sizeof(valBuf), "%s", isOn ? "ON" : "OFF"); break;
            case 14: snprintf(valBuf, sizeof(valBuf), "%+.0f", app.blackHoleCharge); break;
            case 15: snprintf(valBuf, sizeof(valBuf), "%+.1f", app.blackHoleSpin); break;
        }

        int nameLen = (int)strlen(row2Names[v]) + 1;   // +1 for the ':'
        float varWidth = (nameLen + row2MaxVal[v]) * charW;
        float x1 = row2X;
        float x2 = row2X + varWidth;
        if (x2 > row2Limit + charW * 0.6f) {
            if (curRow2 + 1 < botRows) {         // wrap onto the next bar row
                curRow2++;
                row2X = -1.0f + charW * 0.75f;
                x1 = row2X;
                x2 = row2X + varWidth;
            } else {                             // out of room - elide the rest
                addText(row2X, row2Y + curRow2 * botRowHt + charH * 0.2f,
                        ">>", theme::dim, 1.0f);
                break;
            }
        }
        float rTop = row2Y + curRow2 * botRowHt;
        float rBot = rTop + botRowHt;
        float itemTextY2 = rTop + charH * 0.2f;

        bool hovered = isHovered(x1, rTop, x2, rBot);
        if (hovered) {
            app.hoverRow2Var = v;
            addRect(x1 - charW * 0.3f, rTop + px, x2 + charW * 0.3f, rBot,
                    theme::cyan[0], theme::cyan[1], theme::cyan[2], 0.12f);
            addRect(x1 - charW * 0.3f, rTop + px, x2 + charW * 0.3f, rTop + 3.0f * px,
                    theme::cyan[0], theme::cyan[1], theme::cyan[2], 0.85f);
        }

        char nameBuf[16];
        snprintf(nameBuf, sizeof(nameBuf), "%s:", row2Names[v]);
        addText(row2X, itemTextY2, nameBuf, hovered ? theme::bright : theme::dim, 1.0f);
        const float* valCol = isToggle ? (isOn ? theme::amber : theme::dim)
                                       : (hovered ? theme::bright : theme::value);
        addText(row2X + nameLen * charW, itemTextY2, valBuf, valCol, 1.0f);

        row2X += varWidth + row2Gap;
    }

    // Minimized GW observatory: live strain chip at the right end of the bar
    if (app.showInstruments && app.gwMinimized && !app.gwPlus.empty()) {
        float peak = 1e-12f;
        int look = (int)app.gwPlus.size();
        for (int k = std::max(0, look - 60); k < look; k++)
            peak = std::max(peak, fabsf(app.gwPlus[k]));
        char chip[32];
        snprintf(chip, sizeof(chip), "h %.1e", peak);
        float chipW = (strlen(chip) + 4) * charW;
        float cx1 = 1.0f - chipW - charW * 0.5f;
        float gwTop = row2Bottom - botRowHt;
        float gwTextY = gwTop + charH * 0.2f;
        bool cHov = isHovered(cx1, gwTop, cx1 + chipW, row2Bottom);
        if (cHov) {
            app.hoverPanelBtn = 4;
            addRect(cx1, gwTop + px, cx1 + chipW, row2Bottom,
                    theme::cyan[0], theme::cyan[1], theme::cyan[2], 0.12f);
            addRect(cx1, gwTop + px, cx1 + chipW, gwTop + 3.0f * px,
                    theme::cyan[0], theme::cyan[1], theme::cyan[2], 0.85f);
        }
        addText(cx1 + charW * 0.5f, gwTextY, "GW", cHov ? theme::bright : theme::dim, 1.0f);
        addText(cx1 + charW * 3.5f, gwTextY, chip,
                theme::amber, 1.0f);
    }

    // ============================================================
    // DROPDOWN (drawn last so it overlays everything)
    // ============================================================
    if (pendingDropdown.valid) {
        int m = pendingDropdown.menuIdx;
        float dropX = pendingDropdown.x;
        float dropY = pendingDropdown.y;
        float dropWidth = pendingDropdown.width;
        float dropHeight = pendingDropdown.height;
        int itemCount = menus[m].itemCount;

        addShadow(dropX, dropX + dropWidth, dropY + dropHeight, dropY + dropHeight + charH * 0.6f);
        addRectGrad(dropX, dropY, dropX + dropWidth, dropY + dropHeight,
                    theme::panelTop, theme::panelBot);
        // near-opaque backing so the scene never bleeds through the list
        addRect(dropX, dropY, dropX + dropWidth, dropY + dropHeight, 0.01f, 0.02f, 0.04f, 0.75f);
        addHLine(dropX, dropX + dropWidth, dropY + dropHeight - px, theme::cyan, 0.45f);
        addRect(dropX, dropY, dropX + px * 0.75f, dropY + dropHeight,
                theme::cyan[0], theme::cyan[1], theme::cyan[2], 0.35f);
        addRect(dropX + dropWidth - px * 0.75f, dropY, dropX + dropWidth, dropY + dropHeight,
                theme::cyan[0], theme::cyan[1], theme::cyan[2], 0.35f);

        // Dropdown items
        float itemY = dropY + charH * 0.2f;
        for (int i = 0; i < itemCount; i++) {
            float iY1 = itemY;
            float iY2 = itemY + charH * 1.1f;

            bool itemHovered = isHovered(dropX, iY1, dropX + dropWidth, iY2);
            int currentVal = *menus[m].value;
            bool isSelected = (currentVal == i);

            if (itemHovered) {
                app.hoverDropdownItem = i;
                addRect(dropX, iY1, dropX + dropWidth, iY2,
                        theme::cyan[0], theme::cyan[1], theme::cyan[2], 0.16f);
            } else if (isSelected) {
                addRect(dropX, iY1, dropX + dropWidth, iY2,
                        theme::amber[0], theme::amber[1], theme::amber[2], 0.10f);
            }

            if (isSelected)
                addChar(dropX + charW * 0.5f, itemY + charH * 0.05f, '>',
                        theme::amber[0], theme::amber[1], theme::amber[2], 1.0f);

            const float* itemCol = isSelected ? theme::amber
                                 : (itemHovered ? theme::bright : theme::value);
            addText(dropX + charW * 2.0f, itemY + charH * 0.05f, menus[m].items[i], itemCol, 1.0f);

            itemY += charH * 1.2f;
        }
    }
}

void printHelp() {
    printf("\n");
    printf("=== TEPHRON - CONTROLS ===\n");
    printf("\n");
    printf("  CAMERA:\n");
    printf("    WASD         Fly camera (forward/back/strafe)\n");
    printf("    Q/E          Fly up/down\n");
    printf("    Arrows       Rotate camera view\n");
    printf("    Left-drag    Rotate camera\n");
    printf("    Right-drag   Orbit around center of mass\n");
    printf("    Scroll       Zoom in/out\n");
    printf("    Home         Center camera on particles\n");
    printf("\n");
    printf("  SIMULATION:\n");
    printf("    Space        Pause/Resume\n");
    printf("    R            Reset particles & camera\n");
    printf("    G            Cycle gravity type\n");
    printf("    F            Cycle distribution\n");
    printf("    M            Cycle mass mode\n");
    printf("    C            Cycle color mode\n");
    printf("    +/-          Add/remove particles\n");
    printf("    ,/.          Slow/speed time\n");
    printf("\n");
    printf("  PHYSICS:\n");
    printf("    U/O          Decrease/increase gravity strength\n");
    printf("    Z/X          Decrease/increase softening\n");
    printf("    B            Cycle boundary mode\n");
    printf("    F1           Toggle dark matter\n");
    printf("    F2           Toggle electromagnetism\n");
    printf("    F3           Toggle strong force\n");
    printf("    F5           Toggle relativity\n");
    printf("    F6           Toggle expansion (Hubble)\n");
    printf("    F7/F8        Decrease/increase Hubble constant\n");
    printf("    F9           Toggle display mode (auto-orbit)\n");
    printf("    L            Cycle lensing strength\n");
    printf("\n");
    printf("  PRESETS:\n");
    printf("    1-0          Load preset (1=Galaxy, 2=Solar, ...)\n");
    printf("\n");
    printf("  INSTRUMENTS & TOOLS:\n");
    printf("    I            Toggle instruments panel (energy/momentum)\n");
    printf("    V            Toggle accretion merging (bound pairs merge)\n");
    printf("    N            Toggle deep-sky starfield\n");
    printf("    F10          Quick-save state (Shift+F10 to load)\n");
    printf("    F12          Screenshot (screenshots/*.bmp)\n");
    printf("\n");
    printf("  VISUALS:\n");
    printf("    T            Toggle trails\n");
    printf("    [/]          Trail update frequency\n");
    printf("    H            Toggle menu overlay\n");
    printf("    F11          Toggle fullscreen\n");
    printf("    Escape       Quit\n");
    printf("\n");
    printf("  CLI: --selftest | --frames N | --preset K | --screenshot | --fixed-dt X\n");
    printf("====================================\n\n");
}

void renderMenu() {
    // Console status line (shown when menu is hidden)
    static uint32_t lastPrint = 0;
    uint32_t now = SDL_GetTicks();

    if (!app.showMenu && app.menuDirty && now - lastPrint > 500) {
        printf("\r%.0f FPS | %d particles | Press H for menu   ",
               app.fps, app.numParticles);
        fflush(stdout);
        lastPrint = now;
        app.menuDirty = false;
    }
}
