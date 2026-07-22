#include "common.h"

void captureScreenshot() {
    uint32_t w = app.swapchainExtent.width;
    uint32_t h = app.swapchainExtent.height;
    VkDeviceSize size = (VkDeviceSize)w * h * 4;
    VkImage src = app.swapchainImages[app.lastPresentedImage];

    vkQueueWaitIdle(app.graphicsQueue);

    VkBuffer buf; VkDeviceMemory mem;
    createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 buf, mem);

    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo cmdAlloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmdAlloc.commandPool = app.commandPool;
    cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAlloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(app.device, &cmdAlloc, &cmd);

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = src;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {w, h, 1};
    vkCmdCopyImageToBuffer(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buf, 1, &region);

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(app.graphicsQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(app.graphicsQueue);
    vkFreeCommandBuffers(app.device, app.commandPool, 1, &cmd);

    void* data;
    vkMapMemory(app.device, mem, 0, size, 0, &data);

    mkdir("screenshots", 0755);
    char path[256];
    time_t t = time(nullptr);
    struct tm tmv;
    localtime_r(&t, &tmv);
    snprintf(path, sizeof(path), "screenshots/gravity_%04d%02d%02d_%02d%02d%02d.bmp",
             tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
             tmv.tm_hour, tmv.tm_min, tmv.tm_sec);

    FILE* f = fopen(path, "wb");
    if (f) {
        // 54-byte BMP header, 32bpp, rows stored bottom-up; swapchain is
        // B8G8R8A8 so pixel bytes are already in BMP's BGRA order
        uint32_t imageSize = w * h * 4;
        uint32_t fileSize = 54 + imageSize;
        uint8_t hdr[54] = {0};
        hdr[0] = 'B'; hdr[1] = 'M';
        memcpy(hdr + 2, &fileSize, 4);
        uint32_t dataOffset = 54; memcpy(hdr + 10, &dataOffset, 4);
        uint32_t infoSize = 40; memcpy(hdr + 14, &infoSize, 4);
        memcpy(hdr + 18, &w, 4);
        memcpy(hdr + 22, &h, 4);
        uint16_t planes = 1; memcpy(hdr + 26, &planes, 2);
        uint16_t bpp = 32; memcpy(hdr + 28, &bpp, 2);
        memcpy(hdr + 34, &imageSize, 4);
        fwrite(hdr, 54, 1, f);
        const uint8_t* pixels = (const uint8_t*)data;
        for (int row = (int)h - 1; row >= 0; row--)
            fwrite(pixels + (size_t)row * w * 4, 4, w, f);
        fclose(f);
        printf("Screenshot saved: %s\n", path);
    } else {
        fprintf(stderr, "Screenshot failed: cannot open %s\n", path);
    }

    vkUnmapMemory(app.device, mem);
    vkDestroyBuffer(app.device, buf, nullptr);
    vkFreeMemory(app.device, mem, nullptr);
}

void renderFrame() {
    vkWaitForFences(app.device, 1, &app.renderFence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(app.device, app.swapchain, UINT64_MAX,
                                             app.imageAvailable, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || app.needsResize) {
        app.needsResize = false;
        recreateSwapchain();
        return;
    }

    vkResetFences(app.device, 1, &app.renderFence);
    vkResetCommandBuffer(app.commandBuffers[imageIndex], 0);

    // Update uniforms before recording commands
    updateUniforms();

    VkCommandBuffer cmd = app.commandBuffers[imageIndex];
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkViewport viewport{0, 0, (float)app.swapchainExtent.width, (float)app.swapchainExtent.height, 0, 1};
    VkRect2D scissor{{0, 0}, app.swapchainExtent};

    // ============================================================
    // PASS 1: Render trails + particles to HDR offscreen buffer
    // ============================================================
    {
        VkClearValue hdrClear{{{0.0f, 0.0f, 0.0f, 0.0f}}};
        VkRenderPassBeginInfo hdrRPInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        hdrRPInfo.renderPass = app.hdrRenderPass;
        hdrRPInfo.framebuffer = app.hdrFramebuffer;
        hdrRPInfo.renderArea = {{0, 0}, app.swapchainExtent};
        hdrRPInfo.clearValueCount = 1;
        hdrRPInfo.pClearValues = &hdrClear;

        vkCmdBeginRenderPass(cmd, &hdrRPInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Draw world-space trails FIRST (behind particles)
        if (app.trailsEnabled && app.trailPipeline && app.numParticles > 0) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app.trailPipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    app.trailPipelineLayout, 0, 1, &app.trailDescSet, 0, nullptr);
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);
            vkCmdDraw(cmd, app.numParticles * app.trailLength, 1, 0, 0);
        }

        // Draw particles
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app.graphicsPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                app.graphicsPipelineLayout, 0, 1, &app.graphicsDescSet, 0, nullptr);
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        vkCmdDraw(cmd, app.numParticles, 1, 0, 0);

        vkCmdEndRenderPass(cmd);
    }

    // ============================================================
    // MIP CHAIN GENERATION: downsample via vkCmdBlitImage
    // ============================================================
    {
        // Transition mip 0 from COLOR_ATTACHMENT_OPTIMAL to TRANSFER_SRC_OPTIMAL
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = app.hdrImage;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        int32_t mipW = (int32_t)app.swapchainExtent.width;
        int32_t mipH = (int32_t)app.swapchainExtent.height;

        for (uint32_t i = 1; i < app.hdrMipLevels; i++) {
            int32_t nextW = std::max(mipW / 2, 1);
            int32_t nextH = std::max(mipH / 2, 1);

            // Transition mip i to TRANSFER_DST
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.subresourceRange.baseMipLevel = i;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            // Blit from mip i-1 to mip i
            VkImageBlit blit{};
            blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipW, mipH, 1};
            blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1};
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {nextW, nextH, 1};
            vkCmdBlitImage(cmd, app.hdrImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           app.hdrImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit, VK_FILTER_LINEAR);

            // Transition mip i from TRANSFER_DST to TRANSFER_SRC (for next level)
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.subresourceRange.baseMipLevel = i;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            mipW = nextW;
            mipH = nextH;
        }

        // Transition ALL mip levels from TRANSFER_SRC to SHADER_READ_ONLY
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, app.hdrMipLevels, 0, 1};
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    // ============================================================
    // PASS 2: Composite HDR to swapchain + text overlay
    // ============================================================
    {
        VkClearValue clearColor{{{0.02f, 0.02f, 0.04f, 1.0f}}};
        VkRenderPassBeginInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpInfo.renderPass = app.renderPass;
        rpInfo.framebuffer = app.framebuffers[imageIndex];
        rpInfo.renderArea = {{0, 0}, app.swapchainExtent};
        rpInfo.clearValueCount = 1;
        rpInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Draw fullscreen composite (HDR -> LDR with bloom + tone mapping)
        if (app.compositePipeline) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app.compositePipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    app.compositePipelineLayout, 0, 1, &app.compositeDescSet, 0, nullptr);
            // Camera orientation + wall-clock time for the deep-sky starfield
            float compositePush[8] = {
                app.camRotX, app.camRotY, app.camZoom,
                (float)app.swapchainExtent.width / (float)app.swapchainExtent.height,
                SDL_GetTicks() * 0.001f,
                app.starfieldEnabled ? 1.0f : 0.0f,
                app.lowFidelity ? 1.0f : 0.0f,
                0.0f
            };
            vkCmdPushConstants(cmd, app.compositePipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, sizeof(compositePush), compositePush);
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);
            vkCmdDraw(cmd, 3, 1, 0, 0);  // Fullscreen triangle
        }

        // Render text overlay (on top, unaffected by bloom)
        if ((app.showMenu || app.showInstruments) && app.textPipeline && app.fontCharWidth > 0) {
            std::vector<TextVertex> textVerts;
            buildMenuText(textVerts);

            if (!textVerts.empty()) {
                void* data;
                vkMapMemory(app.device, app.textVertexMemory, 0, textVerts.size() * sizeof(TextVertex), 0, &data);
                memcpy(data, textVerts.data(), textVerts.size() * sizeof(TextVertex));
                vkUnmapMemory(app.device, app.textVertexMemory);
                app.textVertexCount = (int)textVerts.size();

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app.textPipeline);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    app.textPipelineLayout, 0, 1, &app.textDescSet, 0, nullptr);
                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(cmd, 0, 1, &app.textVertexBuffer, &offset);
                vkCmdDraw(cmd, app.textVertexCount, 1, 0, 0);
            }
        }

        vkCmdEndRenderPass(cmd);
    }

    vkEndCommandBuffer(cmd);

    VkSemaphore waitSems[] = {app.imageAvailable};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSems[] = {app.renderFinished};

    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = waitSems;
    submit.pWaitDstStageMask = waitStages;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = signalSems;

    vkQueueSubmit(app.graphicsQueue, 1, &submit, app.renderFence);

    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = signalSems;
    present.swapchainCount = 1;
    present.pSwapchains = &app.swapchain;
    present.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(app.graphicsQueue, &present);
    app.lastPresentedImage = imageIndex;
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
    }
}
