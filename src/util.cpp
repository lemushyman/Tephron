#include "common.h"

float randFloat() { return (float)rand() / RAND_MAX; }
float randRange(float min, float max) { return min + randFloat() * (max - min); }
float randGaussian() {
    float u1 = randFloat() + 0.0001f, u2 = randFloat();
    return sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);
}

void randUnitVector(float& x, float& y, float& z) {
    float theta = randFloat() * 2.0f * M_PI;
    float phi = acosf(2.0f * randFloat() - 1.0f);
    x = sinf(phi) * cosf(theta);
    y = sinf(phi) * sinf(theta);
    z = cosf(phi);
}

void hsv2rgb(float h, float s, float v, float& r, float& g, float& b) {
    h = fmodf(h, 1.0f); if (h < 0) h += 1.0f;
    int i = (int)(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s), q = v * (1 - f * s), t = v * (1 - (1 - f) * s);
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(app.physicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    return 0;
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props,
                  VkBuffer& buffer, VkDeviceMemory& memory, bool concurrent) {
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    uint32_t queueFamilies[] = {app.graphicsFamily, app.computeFamily};
    if (concurrent && app.graphicsFamily != app.computeFamily) {
        bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
        bufferInfo.queueFamilyIndexCount = 2;
        bufferInfo.pQueueFamilyIndices = queueFamilies;
    } else {
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    vkCreateBuffer(app.device, &bufferInfo, nullptr, &buffer);

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(app.device, buffer, &memReq);

    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, props);
    if (vkAllocateMemory(app.device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        fprintf(stderr, "FATAL: failed to allocate %.1f MB of GPU memory - "
                        "this machine may not have enough graphics memory\n",
                memReq.size / (1024.0 * 1024.0));
        exit(1);
    }
    vkBindBufferMemory(app.device, buffer, memory, 0);
}

std::vector<char> readFile(const std::string& filename) {
    // Keep the source-tree workflow convenient, but also support distro
    // packages launched from an arbitrary working directory. SDL resolves
    // the real executable location even when /usr/bin/tephron is a symlink.
    std::vector<std::string> candidates{filename};
    if (const char* dataDir = getenv("TEPHRON_DATA_DIR")) {
        std::string root(dataDir);
        if (!root.empty() && root.back() != '/') root += '/';
        candidates.push_back(root + filename);
    }
    if (char* base = SDL_GetBasePath()) {
        candidates.push_back(std::string(base) + filename);
        SDL_free(base);
    }

    for (const std::string& candidate : candidates) {
        std::ifstream file(candidate, std::ios::ate | std::ios::binary);
        if (!file.is_open()) continue;
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        return buffer;
    }

    fprintf(stderr, "Unable to find Tephron resource: %s\n", filename.c_str());
    return {};
}

VkShaderModule createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule module;
    vkCreateShaderModule(app.device, &createInfo, nullptr, &module);
    return module;
}

// Embedded 8x12 bitmap font (covers ASCII 32-127)
// Each character is 8 pixels wide, 12 pixels tall
