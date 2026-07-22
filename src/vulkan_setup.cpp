#include "common.h"

void cleanupHDRResources() {
    if (app.hdrFramebuffer) { vkDestroyFramebuffer(app.device, app.hdrFramebuffer, nullptr); app.hdrFramebuffer = VK_NULL_HANDLE; }
    if (app.hdrFbView) { vkDestroyImageView(app.device, app.hdrFbView, nullptr); app.hdrFbView = VK_NULL_HANDLE; }
    if (app.hdrSamplerView) { vkDestroyImageView(app.device, app.hdrSamplerView, nullptr); app.hdrSamplerView = VK_NULL_HANDLE; }
    if (app.hdrSampler) { vkDestroySampler(app.device, app.hdrSampler, nullptr); app.hdrSampler = VK_NULL_HANDLE; }
    if (app.hdrImage) { vkDestroyImage(app.device, app.hdrImage, nullptr); app.hdrImage = VK_NULL_HANDLE; }
    if (app.hdrMemory) { vkFreeMemory(app.device, app.hdrMemory, nullptr); app.hdrMemory = VK_NULL_HANDLE; }
}

void createHDRResources() {
    uint32_t w = app.swapchainExtent.width;
    uint32_t h = app.swapchainExtent.height;
    app.hdrMipLevels = app.lowFidelity ? 1
                     : (uint32_t)std::floor(std::log2(std::max(w, h))) + 1;

    // Create HDR image with mip chain
    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    imageInfo.extent = {w, h, 1};
    imageInfo.mipLevels = app.hdrMipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkCreateImage(app.device, &imageInfo, nullptr, &app.hdrImage);

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(app.device, app.hdrImage, &memReq);
    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(app.device, &allocInfo, nullptr, &app.hdrMemory);
    vkBindImageMemory(app.device, app.hdrImage, app.hdrMemory, 0);

    // Framebuffer image view: mip 0 only
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = app.hdrImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCreateImageView(app.device, &viewInfo, nullptr, &app.hdrFbView);

    // Sampler image view: all mips
    viewInfo.subresourceRange.levelCount = app.hdrMipLevels;
    vkCreateImageView(app.device, &viewInfo, nullptr, &app.hdrSamplerView);

    // Sampler with linear mipmap filtering
    VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = (float)app.hdrMipLevels;
    vkCreateSampler(app.device, &samplerInfo, nullptr, &app.hdrSampler);

    // HDR framebuffer
    VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fbInfo.renderPass = app.hdrRenderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = &app.hdrFbView;
    fbInfo.width = w;
    fbInfo.height = h;
    fbInfo.layers = 1;
    vkCreateFramebuffer(app.device, &fbInfo, nullptr, &app.hdrFramebuffer);
}

void updateCompositeDescriptor() {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = app.hdrSampler;
    imageInfo.imageView = app.hdrSamplerView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorBufferInfo bhInfo{app.compositeUboBuffer, 0, sizeof(CompositeUBO)};

    VkWriteDescriptorSet writes[2] = {};
    writes[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.compositeDescSet, 0, 0, 1,
                 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo, nullptr, nullptr};
    writes[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.compositeDescSet, 1, 0, 1,
                 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &bhInfo, nullptr};
    vkUpdateDescriptorSets(app.device, 2, writes, 0, nullptr);
}

void cleanupSwapchain() {
    cleanupHDRResources();
    for (auto fb : app.framebuffers) vkDestroyFramebuffer(app.device, fb, nullptr);
    for (auto iv : app.swapchainImageViews) vkDestroyImageView(app.device, iv, nullptr);
    vkDestroySwapchainKHR(app.device, app.swapchain, nullptr);
    app.framebuffers.clear();
    app.swapchainImageViews.clear();
}

void createSwapchain() {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app.physicalDevice, app.surface, &caps);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(app.physicalDevice, app.surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(app.physicalDevice, app.surface, &formatCount, formats.data());

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& fmt : formats) {
        if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM) { surfaceFormat = fmt; break; }
    }

    VkExtent2D extent;
    if (caps.currentExtent.width != UINT32_MAX) {
        extent = caps.currentExtent;
    } else {
        int w, h;
        SDL_Vulkan_GetDrawableSize(app.window, &w, &h);
        extent.width = std::clamp((uint32_t)w, caps.minImageExtent.width, caps.maxImageExtent.width);
        extent.height = std::clamp((uint32_t)h, caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) imageCount = caps.maxImageCount;

    // Select present mode: prefer MAILBOX (low latency, no tearing) > FIFO (vsync fallback)
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(app.physicalDevice, app.surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(app.physicalDevice, app.surface, &presentModeCount, presentModes.data());

    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;  // Always available
    for (const auto& mode : presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosenPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    VkSwapchainCreateInfoKHR swapInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapInfo.surface = app.surface;
    swapInfo.minImageCount = imageCount;
    swapInfo.imageFormat = surfaceFormat.format;
    swapInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapInfo.imageExtent = extent;
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // Allow F12 screenshots (copy swapchain image to a host buffer)
    if (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        swapInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapInfo.preTransform = caps.currentTransform;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapInfo.presentMode = chosenPresentMode;
    swapInfo.clipped = VK_TRUE;

    vkCreateSwapchainKHR(app.device, &swapInfo, nullptr, &app.swapchain);
    app.swapchainFormat = surfaceFormat.format;
    app.swapchainExtent = extent;

    vkGetSwapchainImagesKHR(app.device, app.swapchain, &imageCount, nullptr);
    app.swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(app.device, app.swapchain, &imageCount, app.swapchainImages.data());

    app.swapchainImageViews.resize(imageCount);
    for (size_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = app.swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = app.swapchainFormat;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCreateImageView(app.device, &viewInfo, nullptr, &app.swapchainImageViews[i]);
    }

    // Framebuffers
    app.framebuffers.resize(imageCount);
    for (size_t i = 0; i < imageCount; i++) {
        VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbInfo.renderPass = app.renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &app.swapchainImageViews[i];
        fbInfo.width = app.swapchainExtent.width;
        fbInfo.height = app.swapchainExtent.height;
        fbInfo.layers = 1;
        vkCreateFramebuffer(app.device, &fbInfo, nullptr, &app.framebuffers[i]);
    }

    app.width = extent.width;
    app.height = extent.height;

    // Create HDR resources matching swapchain size
    if (app.hdrRenderPass) {
        createHDRResources();
    }
}

void recreateSwapchain() {
    int w, h;
    SDL_Vulkan_GetDrawableSize(app.window, &w, &h);
    while (w == 0 || h == 0) {
        SDL_Vulkan_GetDrawableSize(app.window, &w, &h);
        SDL_WaitEvent(nullptr);
    }

    vkDeviceWaitIdle(app.device);
    cleanupSwapchain();
    createSwapchain();
    if (app.compositeDescSet) updateCompositeDescriptor();
    app.menuDirty = true;
}

bool initVulkan() {
    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "Tephron";
    appInfo.apiVersion = VK_API_VERSION_1_2;

    uint32_t sdlExtCount;
    SDL_Vulkan_GetInstanceExtensions(app.window, &sdlExtCount, nullptr);
    std::vector<const char*> extensions(sdlExtCount);
    SDL_Vulkan_GetInstanceExtensions(app.window, &sdlExtCount, extensions.data());

    VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &app.instance) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance\n");
        return false;
    }

    if (!SDL_Vulkan_CreateSurface(app.window, app.instance, &app.surface)) {
        fprintf(stderr, "Failed to create surface\n");
        return false;
    }

    // Physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(app.instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(app.instance, &deviceCount, devices.data());

    // GRAV_DEVICE=N forces a specific enumerated device (testing/debugging)
    const char* devOverride = getenv("GRAV_DEVICE");
    if (devOverride && atoi(devOverride) >= 0 && atoi(devOverride) < (int)deviceCount) {
        app.physicalDevice = devices[atoi(devOverride)];
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(app.physicalDevice, &props);
        strncpy(app.gpuName, props.deviceName, sizeof(app.gpuName) - 1);
    } else {
        for (const auto& dev : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(dev, &props);
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                app.physicalDevice = dev;
                strncpy(app.gpuName, props.deviceName, sizeof(app.gpuName) - 1);
                break;
            }
            if (!app.physicalDevice) {
                app.physicalDevice = dev;
                strncpy(app.gpuName, props.deviceName, sizeof(app.gpuName) - 1);
            }
        }
    }

    // ------------------------------------------------------------
    // Device-tier adaptation: size buffers and default particle
    // counts to what this GPU can actually hold and move. Keeps a
    // flagship at full scale and lets integrated laptop graphics
    // (or even llvmpipe software Vulkan) run comfortably.
    // Caps are multiples of 16 so scratch offsets stay 256-aligned.
    // ------------------------------------------------------------
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(app.physicalDevice, &props);
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(app.physicalDevice, &memProps);
        VkDeviceSize deviceLocal = 0;
        for (uint32_t h = 0; h < memProps.memoryHeapCount; h++)
            if (memProps.memoryHeaps[h].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                deviceLocal = std::max(deviceLocal, memProps.memoryHeaps[h].size);

        const char* tier;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            deviceLocal >= (VkDeviceSize)6 * 1024 * 1024 * 1024) {
            app.maxParticlesCap = MAX_PARTICLES; app.perfScale = 1.0f;
            tier = "discrete";
        } else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            app.maxParticlesCap = 250000; app.perfScale = 0.6f;
            tier = "discrete (small VRAM)";
        } else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            app.maxParticlesCap = 60000; app.perfScale = 0.25f;
            app.lowFidelity = true;
            tier = "integrated";
        } else {
            app.maxParticlesCap = 16000; app.perfScale = 0.08f;
            app.lowFidelity = true;
            tier = "software";
        }
        if (app.lowFidelity) app.starfieldEnabled = false;
        app.scratchAccOff = (VkDeviceSize)app.maxParticlesCap * sizeof(Particle);
        app.scratchRowOff = app.scratchAccOff + (VkDeviceSize)app.maxParticlesCap * 4 * sizeof(float);
        printf("GPU tier: %s (%s) - particle cap %d, preset scale %.2f%s\n",
               tier, props.deviceName, app.maxParticlesCap, app.perfScale,
               app.lowFidelity ? ", low-fidelity rendering" : "");
    }

    // Queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(app.physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(app.physicalDevice, &queueFamilyCount, queueFamilies.data());

    bool foundCompute = false;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            VkBool32 present = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(app.physicalDevice, i, app.surface, &present);
            if (present) {
                app.graphicsFamily = i;
                // Use same queue family for compute to avoid cross-queue sync issues
                if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    app.computeFamily = i;
                    foundCompute = true;
                }
            }
        }
        // Fallback: use separate compute queue only if graphics doesn't support compute
        if (!foundCompute && (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            app.computeFamily = i;
            foundCompute = true;
        }
    }

    // Logical device
    std::set<uint32_t> uniqueQueues = {app.graphicsFamily, app.computeFamily};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float priority = 1.0f;
    for (uint32_t family : uniqueQueues) {
        VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueInfo.queueFamilyIndex = family;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        queueCreateInfos.push_back(queueInfo);
    }

    const char* deviceExts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // Query supported features and enable what we need
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(app.physicalDevice, &supportedFeatures);

    VkPhysicalDeviceFeatures features{};
    features.shaderFloat64 = supportedFeatures.shaderFloat64;
    features.largePoints = supportedFeatures.largePoints;  // For point sizes > 1

    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceInfo.queueCreateInfoCount = queueCreateInfos.size();
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.enabledExtensionCount = 1;
    deviceInfo.ppEnabledExtensionNames = deviceExts;
    deviceInfo.pEnabledFeatures = &features;

    vkCreateDevice(app.physicalDevice, &deviceInfo, nullptr, &app.device);
    vkGetDeviceQueue(app.device, app.graphicsFamily, 0, &app.graphicsQueue);
    vkGetDeviceQueue(app.device, app.computeFamily, 0, &app.computeQueue);

    // Render pass
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkRenderPassCreateInfo passInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    passInfo.attachmentCount = 1;
    passInfo.pAttachments = &colorAttachment;
    passInfo.subpassCount = 1;
    passInfo.pSubpasses = &subpass;
    vkCreateRenderPass(app.device, &passInfo, nullptr, &app.renderPass);

    // HDR render pass (RGBA16F, for offscreen HDR accumulation)
    {
        VkAttachmentDescription hdrAttachment{};
        hdrAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        hdrAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        hdrAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        hdrAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        hdrAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        hdrAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference hdrRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription hdrSubpass{};
        hdrSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        hdrSubpass.colorAttachmentCount = 1;
        hdrSubpass.pColorAttachments = &hdrRef;

        VkRenderPassCreateInfo hdrPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        hdrPassInfo.attachmentCount = 1;
        hdrPassInfo.pAttachments = &hdrAttachment;
        hdrPassInfo.subpassCount = 1;
        hdrPassInfo.pSubpasses = &hdrSubpass;
        vkCreateRenderPass(app.device, &hdrPassInfo, nullptr, &app.hdrRenderPass);
    }

    createSwapchain();

    // Descriptor layouts
    // Compute set: particles, sim UBO, Verlet acc history, tile monopoles, diagnostics
    VkDescriptorSetLayoutBinding compBindings[] = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {8, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {9, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {10, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {11, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {12, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}
    };
    VkDescriptorSetLayoutCreateInfo compLayoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    compLayoutInfo.bindingCount = 13;
    compLayoutInfo.pBindings = compBindings;
    vkCreateDescriptorSetLayout(app.device, &compLayoutInfo, nullptr, &app.computeDescLayout);

    VkDescriptorSetLayoutBinding gfxBindings[] = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
    };
    VkDescriptorSetLayoutCreateInfo gfxLayoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    gfxLayoutInfo.bindingCount = 2;
    gfxLayoutInfo.pBindings = gfxBindings;
    vkCreateDescriptorSetLayout(app.device, &gfxLayoutInfo, nullptr, &app.graphicsDescLayout);

    // Buffers
    VkDeviceSize particleSize = (VkDeviceSize)app.maxParticlesCap * sizeof(Particle);
    createBuffer(particleSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, app.particleBuffer, app.particleMemory, true);  // concurrent access
    createBuffer(particleSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 app.stagingBuffer, app.stagingMemory);
    createBuffer(sizeof(SimUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 app.simUboBuffer, app.simUboMemory);
    createBuffer(sizeof(RenderUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 app.renderUboBuffer, app.renderUboMemory);

    // Trail buffers
    VkDeviceSize trailBufSize = (VkDeviceSize)app.maxParticlesCap * TRAIL_LENGTH * sizeof(TrailPoint);
    createBuffer(trailBufSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 app.trailBuffer, app.trailMemory);
    createBuffer(sizeof(TrailUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 app.trailUboBuffer, app.trailUboMemory);

    // Persistent readback buffer for GPU→CPU data transfer (trails, COM calculation, etc.)
    // Created once, reused every frame — eliminates per-frame alloc/free overhead
    createBuffer((VkDeviceSize)app.maxParticlesCap * sizeof(Particle), VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 app.readbackBuffer, app.readbackMemory);

    // Velocity Verlet acceleration history (zeroed on every particle upload)
    createBuffer((VkDeviceSize)app.maxParticlesCap * 4 * sizeof(float),
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, app.accBuffer, app.accMemory, true);
    // Tile summaries for the far-field solver: 2 vec4 aggregate + 16 vec4 of
    // sub-monopoles per tile (layout mirrored in physics.comp / tilecom.comp,
    // which hardcode MAX_TILES_STORE)
    static_assert(MAX_TILES_TOTAL == 1954, "update MAX_TILES_STORE in the shaders");
    createBuffer((VkDeviceSize)MAX_TILES_TOTAL * 18 * 4 * sizeof(float),
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, app.tileComBuffer, app.tileComMemory, true);
    // Diagnostics partial sums, read directly by the CPU after the compute fence
    createBuffer(MAX_TILES_TOTAL * 16 * sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 app.diagBuffer, app.diagMemory, true);
    // Trail row indirection map (particle index -> trail buffer row)
    createBuffer((VkDeviceSize)app.maxParticlesCap * sizeof(int32_t),
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 app.trailRowMapBuffer, app.trailRowMapMemory, true);

    // Async sort machinery: snapshot readback target, permutation upload,
    // and the gather scratch (particle | acc | rowMap regions, 256-aligned)
    createBuffer((VkDeviceSize)app.maxParticlesCap * sizeof(Particle), VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 app.snapshotBuffer, app.snapshotMemory);
    createBuffer((VkDeviceSize)app.maxParticlesCap * sizeof(int32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 app.permBuffer, app.permMemory, true);
    createBuffer((VkDeviceSize)app.maxParticlesCap * (sizeof(Particle) + 4 * sizeof(float) + sizeof(int32_t)),
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 app.sortScratchBuffer, app.sortScratchMemory, true);
    // Black hole channels: per-frame emission (GPU->CPU) and sim list (CPU->GPU)
    createBuffer(sizeof(BHOutCPU), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 app.bhOutBuffer, app.bhOutMemory, true);
    createBuffer(sizeof(BHSimCPU), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 app.bhSimBuffer, app.bhSimMemory, true);

    // Trail descriptor layout
    VkDescriptorSetLayoutBinding trailBindings[] = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}
    };
    VkDescriptorSetLayoutCreateInfo trailLayoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    trailLayoutInfo.bindingCount = 3;
    trailLayoutInfo.pBindings = trailBindings;
    vkCreateDescriptorSetLayout(app.device, &trailLayoutInfo, nullptr, &app.trailDescLayout);

    // Descriptor pool & sets
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 20},  // compute set (12), graphics, trail (2), headroom
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5},   // sim, render, trail, composite BH, extra
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2}  // text overlay + HDR composite
    };
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.poolSizeCount = 3;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 5;  // compute, graphics, text, trail, composite
    vkCreateDescriptorPool(app.device, &poolInfo, nullptr, &app.descriptorPool);

    VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = app.descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &app.computeDescLayout;
    vkAllocateDescriptorSets(app.device, &allocInfo, &app.computeDescSet);

    allocInfo.pSetLayouts = &app.graphicsDescLayout;
    vkAllocateDescriptorSets(app.device, &allocInfo, &app.graphicsDescSet);

    allocInfo.pSetLayouts = &app.trailDescLayout;
    vkAllocateDescriptorSets(app.device, &allocInfo, &app.trailDescSet);

    // Update descriptor sets
    VkDescriptorBufferInfo particleBufInfo{app.particleBuffer, 0, (VkDeviceSize)app.maxParticlesCap * sizeof(Particle)};
    VkDescriptorBufferInfo simUboBufInfo{app.simUboBuffer, 0, sizeof(SimUBO)};
    VkDescriptorBufferInfo renderUboBufInfo{app.renderUboBuffer, 0, sizeof(RenderUBO)};
    VkDescriptorBufferInfo trailBufInfo{app.trailBuffer, 0, (VkDeviceSize)app.maxParticlesCap * TRAIL_LENGTH * sizeof(TrailPoint)};
    VkDescriptorBufferInfo trailUboBufInfo{app.trailUboBuffer, 0, sizeof(TrailUBO)};
    VkDescriptorBufferInfo accBufInfo{app.accBuffer, 0, (VkDeviceSize)app.maxParticlesCap * 4 * sizeof(float)};
    VkDescriptorBufferInfo tileComBufInfo{app.tileComBuffer, 0,
                                          (VkDeviceSize)MAX_TILES_TOTAL * 18 * 4 * sizeof(float)};
    VkDescriptorBufferInfo diagBufInfo{app.diagBuffer, 0, MAX_TILES_TOTAL * 16 * sizeof(float)};

    VkDescriptorBufferInfo rowMapBufInfo{app.trailRowMapBuffer, 0, (VkDeviceSize)app.maxParticlesCap * sizeof(int32_t)};
    VkDescriptorBufferInfo permBufInfo{app.permBuffer, 0, (VkDeviceSize)app.maxParticlesCap * sizeof(int32_t)};
    VkDescriptorBufferInfo scratchPInfo{app.sortScratchBuffer, 0, (VkDeviceSize)app.maxParticlesCap * sizeof(Particle)};
    VkDescriptorBufferInfo scratchAInfo{app.sortScratchBuffer, app.scratchAccOff, (VkDeviceSize)app.maxParticlesCap * 4 * sizeof(float)};
    VkDescriptorBufferInfo scratchRInfo{app.sortScratchBuffer, app.scratchRowOff, (VkDeviceSize)app.maxParticlesCap * sizeof(int32_t)};
    VkDescriptorBufferInfo bhOutBufInfo{app.bhOutBuffer, 0, sizeof(BHOutCPU)};
    VkDescriptorBufferInfo bhSimBufInfo{app.bhSimBuffer, 0, sizeof(BHSimCPU)};
    VkWriteDescriptorSet writes[18] = {};
    writes[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.computeDescSet, 0, 0, 1,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &particleBufInfo, nullptr};
    writes[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.computeDescSet, 1, 0, 1,
                 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &simUboBufInfo, nullptr};
    writes[2] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.graphicsDescSet, 0, 0, 1,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &particleBufInfo, nullptr};
    writes[3] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.graphicsDescSet, 1, 0, 1,
                 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &renderUboBufInfo, nullptr};
    writes[4] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.trailDescSet, 0, 0, 1,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &trailBufInfo, nullptr};
    writes[5] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.trailDescSet, 1, 0, 1,
                 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &trailUboBufInfo, nullptr};
    writes[6] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.computeDescSet, 2, 0, 1,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &accBufInfo, nullptr};
    writes[7] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.computeDescSet, 3, 0, 1,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &tileComBufInfo, nullptr};
    writes[8] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.computeDescSet, 4, 0, 1,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &diagBufInfo, nullptr};
    writes[9] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.computeDescSet, 5, 0, 1,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &trailBufInfo, nullptr};
    writes[10] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.computeDescSet, 6, 0, 1,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &rowMapBufInfo, nullptr};
    writes[11] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.trailDescSet, 2, 0, 1,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &rowMapBufInfo, nullptr};
    writes[12] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.computeDescSet, 7, 0, 1,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &permBufInfo, nullptr};
    writes[13] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.computeDescSet, 8, 0, 1,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &scratchPInfo, nullptr};
    writes[14] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.computeDescSet, 9, 0, 1,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &scratchAInfo, nullptr};
    writes[15] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.computeDescSet, 10, 0, 1,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &scratchRInfo, nullptr};
    writes[16] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.computeDescSet, 11, 0, 1,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &bhOutBufInfo, nullptr};
    writes[17] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, app.computeDescSet, 12, 0, 1,
                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &bhSimBufInfo, nullptr};
    vkUpdateDescriptorSets(app.device, 18, writes, 0, nullptr);

    // Pipelines
    auto compCode = readFile("shaders/physics.comp.spv");
    auto vertCode = readFile("shaders/particle.vert.spv");
    auto fragCode = readFile("shaders/particle.frag.spv");

    if (compCode.empty() || vertCode.empty() || fragCode.empty()) {
        fprintf(stderr, "Failed to load shaders\n");
        return false;
    }

    // Compute pipeline
    VkShaderModule compModule = createShaderModule(compCode);
    VkPipelineShaderStageCreateInfo compStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    compStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    compStage.module = compModule;
    compStage.pName = "main";

    VkPushConstantRange compPcRange{VK_SHADER_STAGE_COMPUTE_BIT, 0, 2 * sizeof(int32_t)};
    VkPipelineLayoutCreateInfo compLayoutCI{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    compLayoutCI.setLayoutCount = 1;
    compLayoutCI.pSetLayouts = &app.computeDescLayout;
    compLayoutCI.pushConstantRangeCount = 1;
    compLayoutCI.pPushConstantRanges = &compPcRange;
    vkCreatePipelineLayout(app.device, &compLayoutCI, nullptr, &app.computePipelineLayout);

    VkComputePipelineCreateInfo compPipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    compPipelineInfo.stage = compStage;
    compPipelineInfo.layout = app.computePipelineLayout;
    vkCreateComputePipelines(app.device, VK_NULL_HANDLE, 1, &compPipelineInfo, nullptr, &app.computePipeline);
    vkDestroyShaderModule(app.device, compModule, nullptr);

    // Tile center-of-mass reduction pipeline (shares the compute layout/set)
    auto tileComCode = readFile("shaders/tilecom.comp.spv");
    if (tileComCode.empty()) {
        fprintf(stderr, "Failed to load tilecom shader\n");
        return false;
    }
    VkShaderModule tileComModule = createShaderModule(tileComCode);
    VkPipelineShaderStageCreateInfo tileComStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    tileComStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    tileComStage.module = tileComModule;
    tileComStage.pName = "main";
    VkComputePipelineCreateInfo tileComPipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    tileComPipelineInfo.stage = tileComStage;
    tileComPipelineInfo.layout = app.computePipelineLayout;
    vkCreateComputePipelines(app.device, VK_NULL_HANDLE, 1, &tileComPipelineInfo, nullptr, &app.tileComPipeline);
    vkDestroyShaderModule(app.device, tileComModule, nullptr);

    // GPU trail ring-buffer update pipeline (shares the compute layout/set)
    auto trailUpCode = readFile("shaders/trailupdate.comp.spv");
    if (trailUpCode.empty()) {
        fprintf(stderr, "Failed to load trailupdate shader\n");
        return false;
    }
    VkShaderModule trailUpModule = createShaderModule(trailUpCode);
    VkPipelineShaderStageCreateInfo trailUpStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    trailUpStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    trailUpStage.module = trailUpModule;
    trailUpStage.pName = "main";
    VkComputePipelineCreateInfo trailUpPipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    trailUpPipelineInfo.stage = trailUpStage;
    trailUpPipelineInfo.layout = app.computePipelineLayout;
    vkCreateComputePipelines(app.device, VK_NULL_HANDLE, 1, &trailUpPipelineInfo, nullptr, &app.trailUpdatePipeline);
    vkDestroyShaderModule(app.device, trailUpModule, nullptr);

    // Async-sort apply pipeline (shares the compute layout/set)
    auto permCode = readFile("shaders/permute.comp.spv");
    if (permCode.empty()) {
        fprintf(stderr, "Failed to load permute shader\n");
        return false;
    }
    VkShaderModule permModule = createShaderModule(permCode);
    VkPipelineShaderStageCreateInfo permStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    permStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    permStage.module = permModule;
    permStage.pName = "main";
    VkComputePipelineCreateInfo permPipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    permPipelineInfo.stage = permStage;
    permPipelineInfo.layout = app.computePipelineLayout;
    vkCreateComputePipelines(app.device, VK_NULL_HANDLE, 1, &permPipelineInfo, nullptr, &app.permutePipeline);
    vkDestroyShaderModule(app.device, permModule, nullptr);

    // Graphics pipeline
    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                 VK_SHADER_STAGE_VERTEX_BIT, vertModule, "main", nullptr};
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                 VK_SHADER_STAGE_FRAGMENT_BIT, fragModule, "main", nullptr};

    VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

    VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;

    VkPipelineMultisampleStateCreateInfo multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Additive blending: src*alpha + dst*1
    // Order-independent, smooth accumulation, no draw-order noise
    // Dense areas glow brighter; per-particle tone mapping keeps colors vibrant
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = 0xF;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &colorBlendAttachment;

    VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynStates;

    VkPipelineLayoutCreateInfo gfxLayoutCI{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    gfxLayoutCI.setLayoutCount = 1;
    gfxLayoutCI.pSetLayouts = &app.graphicsDescLayout;
    vkCreatePipelineLayout(app.device, &gfxLayoutCI, nullptr, &app.graphicsPipelineLayout);

    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = app.graphicsPipelineLayout;
    pipelineInfo.renderPass = app.hdrRenderPass;

    VkResult pipeResult = vkCreateGraphicsPipelines(app.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &app.graphicsPipeline);
    if (pipeResult != VK_SUCCESS) {
        fprintf(stderr, "Failed to create graphics pipeline: %d\n", pipeResult);
        return false;
    }
    vkDestroyShaderModule(app.device, vertModule, nullptr);
    vkDestroyShaderModule(app.device, fragModule, nullptr);

    // Trail pipeline for world-space particle trails
    auto trailVertCode = readFile("shaders/trail.vert.spv");
    auto trailFragCode = readFile("shaders/trail.frag.spv");

    if (!trailVertCode.empty() && !trailFragCode.empty()) {
        VkShaderModule trailVertModule = createShaderModule(trailVertCode);
        VkShaderModule trailFragModule = createShaderModule(trailFragCode);

        VkPipelineShaderStageCreateInfo trailStages[2] = {};
        trailStages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                         VK_SHADER_STAGE_VERTEX_BIT, trailVertModule, "main", nullptr};
        trailStages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                         VK_SHADER_STAGE_FRAGMENT_BIT, trailFragModule, "main", nullptr};

        VkPipelineLayoutCreateInfo trailLayoutCI{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        trailLayoutCI.setLayoutCount = 1;
        trailLayoutCI.pSetLayouts = &app.trailDescLayout;
        vkCreatePipelineLayout(app.device, &trailLayoutCI, nullptr, &app.trailPipelineLayout);

        // Additive blending for trail glow
        VkPipelineColorBlendAttachmentState trailBlendAttachment{};
        trailBlendAttachment.colorWriteMask = 0xF;
        trailBlendAttachment.blendEnable = VK_TRUE;
        trailBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        trailBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        trailBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        trailBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        trailBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        trailBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo trailColorBlend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        trailColorBlend.attachmentCount = 1;
        trailColorBlend.pAttachments = &trailBlendAttachment;

        VkGraphicsPipelineCreateInfo trailPipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        trailPipelineInfo.stageCount = 2;
        trailPipelineInfo.pStages = trailStages;
        trailPipelineInfo.pVertexInputState = &vertexInput;
        trailPipelineInfo.pInputAssemblyState = &inputAssembly;  // Point list
        trailPipelineInfo.pViewportState = &viewportState;
        trailPipelineInfo.pRasterizationState = &rasterizer;
        trailPipelineInfo.pMultisampleState = &multisampling;
        trailPipelineInfo.pColorBlendState = &trailColorBlend;
        trailPipelineInfo.pDynamicState = &dynamicState;
        trailPipelineInfo.layout = app.trailPipelineLayout;
        trailPipelineInfo.renderPass = app.hdrRenderPass;

        vkCreateGraphicsPipelines(app.device, VK_NULL_HANDLE, 1, &trailPipelineInfo, nullptr, &app.trailPipeline);
        vkDestroyShaderModule(app.device, trailVertModule, nullptr);
        vkDestroyShaderModule(app.device, trailFragModule, nullptr);
        printf("World-space trail pipeline created successfully\n");
    }

    // Command pools & buffers
    VkCommandPoolCreateInfo poolCI{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolCI.queueFamilyIndex = app.graphicsFamily;
    vkCreateCommandPool(app.device, &poolCI, nullptr, &app.commandPool);
    poolCI.queueFamilyIndex = app.computeFamily;
    vkCreateCommandPool(app.device, &poolCI, nullptr, &app.computeCommandPool);

    app.commandBuffers.resize(app.framebuffers.size());
    VkCommandBufferAllocateInfo cmdAllocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmdAllocInfo.commandPool = app.commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = app.commandBuffers.size();
    vkAllocateCommandBuffers(app.device, &cmdAllocInfo, app.commandBuffers.data());

    cmdAllocInfo.commandPool = app.computeCommandPool;
    cmdAllocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(app.device, &cmdAllocInfo, &app.computeCommandBuffer);

    // Sync objects
    VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateSemaphore(app.device, &semInfo, nullptr, &app.imageAvailable);
    vkCreateSemaphore(app.device, &semInfo, nullptr, &app.renderFinished);
    vkCreateFence(app.device, &fenceInfo, nullptr, &app.renderFence);
    vkCreateFence(app.device, &fenceInfo, nullptr, &app.computeFence);

    // Async snapshot copy: pre-allocated command buffer + unsignaled fence
    VkCommandBufferAllocateInfo snapAlloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    snapAlloc.commandPool = app.commandPool;
    snapAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    snapAlloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(app.device, &snapAlloc, &app.snapshotCmdBuf);
    VkFenceCreateInfo snapFenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(app.device, &snapFenceInfo, nullptr, &app.snapshotFence);

    // Initialize text overlay
    if (createFontTexture()) {
        createTextPipeline();
    }

    // Composite pipeline (fullscreen HDR -> LDR with bloom + black hole lensing)
    {
        // Black hole UBO for the lensing/shadow pass
        createBuffer(sizeof(CompositeUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     app.compositeUboBuffer, app.compositeUboMemory);

        // Descriptor set layout: HDR sampler + black hole UBO
        VkDescriptorSetLayoutBinding bindings[2] = {};
        bindings[0] = {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
        bindings[1] = {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};

        VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layoutInfo.bindingCount = 2;
        layoutInfo.pBindings = bindings;
        vkCreateDescriptorSetLayout(app.device, &layoutInfo, nullptr, &app.compositeDescLayout);

        // Allocate descriptor set
        VkDescriptorSetAllocateInfo dsAllocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        dsAllocInfo.descriptorPool = app.descriptorPool;
        dsAllocInfo.descriptorSetCount = 1;
        dsAllocInfo.pSetLayouts = &app.compositeDescLayout;
        vkAllocateDescriptorSets(app.device, &dsAllocInfo, &app.compositeDescSet);

        // Update descriptor with HDR image
        updateCompositeDescriptor();

        // Pipeline layout with push constants (camera orientation + time for the starfield)
        VkPushConstantRange pcRange{VK_SHADER_STAGE_FRAGMENT_BIT, 0, 8 * sizeof(float)};
        VkPipelineLayoutCreateInfo plInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        plInfo.setLayoutCount = 1;
        plInfo.pSetLayouts = &app.compositeDescLayout;
        plInfo.pushConstantRangeCount = 1;
        plInfo.pPushConstantRanges = &pcRange;
        vkCreatePipelineLayout(app.device, &plInfo, nullptr, &app.compositePipelineLayout);

        // Load shaders
        auto compVertCode = readFile("shaders/composite.vert.spv");
        auto compFragCode = readFile("shaders/composite.frag.spv");
        if (!compVertCode.empty() && !compFragCode.empty()) {
            VkShaderModule vertMod = createShaderModule(compVertCode);
            VkShaderModule fragMod = createShaderModule(compFragCode);

            VkPipelineShaderStageCreateInfo compStages[2] = {};
            compStages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
            compStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            compStages[0].module = vertMod;
            compStages[0].pName = "main";
            compStages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
            compStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            compStages[1].module = fragMod;
            compStages[1].pName = "main";

            // No vertex input (fullscreen triangle from gl_VertexIndex)
            VkPipelineVertexInputStateCreateInfo compVertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

            VkPipelineInputAssemblyStateCreateInfo compIA{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
            compIA.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

            VkPipelineViewportStateCreateInfo compVP{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
            compVP.viewportCount = 1;
            compVP.scissorCount = 1;

            VkPipelineRasterizationStateCreateInfo compRast{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
            compRast.polygonMode = VK_POLYGON_MODE_FILL;
            compRast.lineWidth = 1.0f;
            compRast.cullMode = VK_CULL_MODE_NONE;

            VkPipelineMultisampleStateCreateInfo compMS{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
            compMS.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            // No blending - composite writes final color
            VkPipelineColorBlendAttachmentState compBlend{};
            compBlend.colorWriteMask = 0xF;
            compBlend.blendEnable = VK_FALSE;

            VkPipelineColorBlendStateCreateInfo compCB{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
            compCB.attachmentCount = 1;
            compCB.pAttachments = &compBlend;

            VkDynamicState compDynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo compDyn{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
            compDyn.dynamicStateCount = 2;
            compDyn.pDynamicStates = compDynStates;

            VkGraphicsPipelineCreateInfo compPipeInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
            compPipeInfo.stageCount = 2;
            compPipeInfo.pStages = compStages;
            compPipeInfo.pVertexInputState = &compVertexInput;
            compPipeInfo.pInputAssemblyState = &compIA;
            compPipeInfo.pViewportState = &compVP;
            compPipeInfo.pRasterizationState = &compRast;
            compPipeInfo.pMultisampleState = &compMS;
            compPipeInfo.pColorBlendState = &compCB;
            compPipeInfo.pDynamicState = &compDyn;
            compPipeInfo.layout = app.compositePipelineLayout;
            compPipeInfo.renderPass = app.renderPass;

            vkCreateGraphicsPipelines(app.device, VK_NULL_HANDLE, 1, &compPipeInfo, nullptr, &app.compositePipeline);

            vkDestroyShaderModule(app.device, vertMod, nullptr);
            vkDestroyShaderModule(app.device, fragMod, nullptr);
            printf("HDR composite pipeline created successfully\n");
        }
    }

    return true;
}

void cleanup() {
    vkDeviceWaitIdle(app.device);

    // Trail effect cleanup
    if (app.trailPipeline) vkDestroyPipeline(app.device, app.trailPipeline, nullptr);
    if (app.trailPipelineLayout) vkDestroyPipelineLayout(app.device, app.trailPipelineLayout, nullptr);
    if (app.trailDescLayout) vkDestroyDescriptorSetLayout(app.device, app.trailDescLayout, nullptr);
    if (app.trailBuffer) vkDestroyBuffer(app.device, app.trailBuffer, nullptr);
    if (app.trailMemory) vkFreeMemory(app.device, app.trailMemory, nullptr);
    if (app.trailUboBuffer) vkDestroyBuffer(app.device, app.trailUboBuffer, nullptr);
    if (app.trailUboMemory) vkFreeMemory(app.device, app.trailUboMemory, nullptr);

    // Text overlay cleanup
    if (app.textVertexBuffer) vkDestroyBuffer(app.device, app.textVertexBuffer, nullptr);
    if (app.textVertexMemory) vkFreeMemory(app.device, app.textVertexMemory, nullptr);
    if (app.textPipeline) vkDestroyPipeline(app.device, app.textPipeline, nullptr);
    if (app.textPipelineLayout) vkDestroyPipelineLayout(app.device, app.textPipelineLayout, nullptr);
    if (app.textDescLayout) vkDestroyDescriptorSetLayout(app.device, app.textDescLayout, nullptr);
    if (app.fontSampler) vkDestroySampler(app.device, app.fontSampler, nullptr);
    if (app.fontImageView) vkDestroyImageView(app.device, app.fontImageView, nullptr);
    if (app.fontImage) vkDestroyImage(app.device, app.fontImage, nullptr);
    if (app.fontMemory) vkFreeMemory(app.device, app.fontMemory, nullptr);

    vkDestroyFence(app.device, app.renderFence, nullptr);
    vkDestroyFence(app.device, app.computeFence, nullptr);
    vkDestroySemaphore(app.device, app.imageAvailable, nullptr);
    vkDestroySemaphore(app.device, app.renderFinished, nullptr);

    vkDestroyCommandPool(app.device, app.commandPool, nullptr);
    vkDestroyCommandPool(app.device, app.computeCommandPool, nullptr);

    vkDestroyBuffer(app.device, app.particleBuffer, nullptr);
    vkFreeMemory(app.device, app.particleMemory, nullptr);
    vkDestroyBuffer(app.device, app.stagingBuffer, nullptr);
    vkFreeMemory(app.device, app.stagingMemory, nullptr);
    if (app.readbackBuffer) vkDestroyBuffer(app.device, app.readbackBuffer, nullptr);
    if (app.readbackMemory) vkFreeMemory(app.device, app.readbackMemory, nullptr);
    if (app.accBuffer) vkDestroyBuffer(app.device, app.accBuffer, nullptr);
    if (app.accMemory) vkFreeMemory(app.device, app.accMemory, nullptr);
    if (app.tileComBuffer) vkDestroyBuffer(app.device, app.tileComBuffer, nullptr);
    if (app.tileComMemory) vkFreeMemory(app.device, app.tileComMemory, nullptr);
    if (app.diagBuffer) vkDestroyBuffer(app.device, app.diagBuffer, nullptr);
    if (app.diagMemory) vkFreeMemory(app.device, app.diagMemory, nullptr);
    if (app.tileComPipeline) vkDestroyPipeline(app.device, app.tileComPipeline, nullptr);
    if (app.trailUpdatePipeline) vkDestroyPipeline(app.device, app.trailUpdatePipeline, nullptr);
    if (app.trailRowMapBuffer) vkDestroyBuffer(app.device, app.trailRowMapBuffer, nullptr);
    if (app.trailRowMapMemory) vkFreeMemory(app.device, app.trailRowMapMemory, nullptr);
    if (app.snapshotBuffer) vkDestroyBuffer(app.device, app.snapshotBuffer, nullptr);
    if (app.snapshotMemory) vkFreeMemory(app.device, app.snapshotMemory, nullptr);
    if (app.permBuffer) vkDestroyBuffer(app.device, app.permBuffer, nullptr);
    if (app.permMemory) vkFreeMemory(app.device, app.permMemory, nullptr);
    if (app.sortScratchBuffer) vkDestroyBuffer(app.device, app.sortScratchBuffer, nullptr);
    if (app.sortScratchMemory) vkFreeMemory(app.device, app.sortScratchMemory, nullptr);
    if (app.permutePipeline) vkDestroyPipeline(app.device, app.permutePipeline, nullptr);
    if (app.snapshotFence) vkDestroyFence(app.device, app.snapshotFence, nullptr);
    if (app.bhOutBuffer) vkDestroyBuffer(app.device, app.bhOutBuffer, nullptr);
    if (app.bhOutMemory) vkFreeMemory(app.device, app.bhOutMemory, nullptr);
    if (app.bhSimBuffer) vkDestroyBuffer(app.device, app.bhSimBuffer, nullptr);
    if (app.bhSimMemory) vkFreeMemory(app.device, app.bhSimMemory, nullptr);
    if (app.compositeUboBuffer) vkDestroyBuffer(app.device, app.compositeUboBuffer, nullptr);
    if (app.compositeUboMemory) vkFreeMemory(app.device, app.compositeUboMemory, nullptr);
    vkDestroyBuffer(app.device, app.simUboBuffer, nullptr);
    vkFreeMemory(app.device, app.simUboMemory, nullptr);
    vkDestroyBuffer(app.device, app.renderUboBuffer, nullptr);
    vkFreeMemory(app.device, app.renderUboMemory, nullptr);

    vkDestroyDescriptorPool(app.device, app.descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(app.device, app.computeDescLayout, nullptr);
    vkDestroyDescriptorSetLayout(app.device, app.graphicsDescLayout, nullptr);

    // Composite pipeline cleanup
    if (app.compositePipeline) vkDestroyPipeline(app.device, app.compositePipeline, nullptr);
    if (app.compositePipelineLayout) vkDestroyPipelineLayout(app.device, app.compositePipelineLayout, nullptr);
    if (app.compositeDescLayout) vkDestroyDescriptorSetLayout(app.device, app.compositeDescLayout, nullptr);

    vkDestroyPipeline(app.device, app.computePipeline, nullptr);
    vkDestroyPipelineLayout(app.device, app.computePipelineLayout, nullptr);
    vkDestroyPipeline(app.device, app.graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(app.device, app.graphicsPipelineLayout, nullptr);

    cleanupSwapchain();
    vkDestroyRenderPass(app.device, app.renderPass, nullptr);
    if (app.hdrRenderPass) vkDestroyRenderPass(app.device, app.hdrRenderPass, nullptr);
    vkDestroyDevice(app.device, nullptr);
    vkDestroySurfaceKHR(app.instance, app.surface, nullptr);
    vkDestroyInstance(app.instance, nullptr);

    SDL_DestroyWindow(app.window);
    TTF_Quit();
    SDL_Quit();
}

// ============================================================
// SELFTEST (--selftest): numerically verifies the physics stack.
// Runs real GPU simulations and checks conservation laws through
// the same diagnostics pipeline the instruments panel uses.
