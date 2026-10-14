// SPDX-License-Identifier: GPL-3.0-or-later
// Native Plume/RADV integration test. No game data, network or console file writes.
#include "plume_vulkan.h"
#include "plume_probe_vert.h"
#include "plume_probe_frag.h"
#include <ps5platform/fp.h>
#include <ps5platform/heap.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <cstdarg>
#include <unistd.h>
#if defined(DKR_NATIVE_TITLE)
#include <ps5platform/klog.h>
extern "C" int sceSystemServiceHideSplashScreen(void);
#endif

namespace plume { std::unique_ptr<RenderInterface> CreateVulkanInterface(); }
using namespace plume;

static void report(const char *s) {
    std::printf("plume-probe: %s\n", s); std::fflush(stdout);
#if defined(DKR_NATIVE_TITLE)
    char line[1200]; std::snprintf(line, sizeof(line), "[DKR-R] plume-probe: %s\n", s);
    sceKernelDebugOutText(0, line);
#endif
}
static void reportf(const char *format, ...) {
    char line[1024]; va_list args; va_start(args, format);
    std::vsnprintf(line, sizeof(line), format, args); va_end(args); report(line);
}

static bool draw(RenderCommandList *cmd, RenderFramebuffer *fb, RenderTexture *texture,
                 RenderPipelineLayout *layout, RenderPipeline *pipeline, uint32_t w, uint32_t h) {
    cmd->begin();
    cmd->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(texture, RenderTextureLayout::COLOR_WRITE));
    cmd->setFramebuffer(fb);
    cmd->setViewports(RenderViewport(0, 0, float(w), float(h)));
    cmd->setScissors(RenderRect(0, 0, w, h));
    cmd->clearColor(0, RenderColor(0, 0, 0.2f, 1));
    cmd->setGraphicsPipelineLayout(layout);
    cmd->setPipeline(pipeline);
    cmd->drawInstanced(3, 1, 0, 0);
    return true;
}

static int run() {
#if defined(DKR_NATIVE_TITLE)
    report("build=plume-radv-native-v5 FSELF title (no game; system-service closure)");
#else
    report("build=plume-radv-v2 native GPU test (no game; system-module .sprx imports)");
#endif
    setenv("MESA_SHADER_CACHE_DISABLE", "true", 1);
    ps5_fp_ieee();
    auto api = CreateVulkanInterface();
    if (!api) { report("FAIL instance/required extensions"); return 1; }
    report("instance=PASS api-request=1.2 resolver=static-RADV");
    auto device = api->createDevice();
    if (!device) { report("FAIL device/features"); return 2; }
    reportf("device=%s", device->getDescription().name.c_str());
    auto *nativeDevice = static_cast<VulkanDevice *>(device.get());
    auto queue = device->createCommandQueue(RenderCommandListType::DIRECT);
    auto *nativeQueue = static_cast<VulkanCommandQueue *>(queue.get());
    if (!nativeQueue->queue || !nativeQueue->queue->vk) { report("FAIL graphics queue"); return 3; }
    auto cmd = queue->createCommandList();
    auto *nativeCmd = static_cast<VulkanCommandList *>(cmd.get());
    if (!nativeCmd->vk) { report("FAIL command buffer"); return 4; }
    auto fence = device->createCommandFence();
    auto *nativeFence = static_cast<VulkanCommandFence *>(fence.get());
    if (!nativeFence->vk) { report("FAIL fence"); return 5; }
    auto wait = [&]() {
        const VkResult res = vkWaitForFences(nativeDevice->vk, 1, &nativeFence->vk, VK_TRUE, UINT64_C(5000000000));
        if (res != VK_SUCCESS) {
            std::printf("plume-probe: FAIL GPU wait result=%d; process exits without unsafe resource destruction\n", res);
            std::fflush(stdout);
            // A hung submission cannot safely have its resources destroyed.
            _exit(20);
        }
        return vkResetFences(nativeDevice->vk, 1, &nativeFence->vk) == VK_SUCCESS;
    };
    auto vs = device->createShader(plume_probe_vert, sizeof(plume_probe_vert), "main", RenderShaderFormat::SPIRV);
    auto fs = device->createShader(plume_probe_frag, sizeof(plume_probe_frag), "main", RenderShaderFormat::SPIRV);
    if (!static_cast<VulkanShader *>(vs.get())->vk || !static_cast<VulkanShader *>(fs.get())->vk) {
        report("FAIL shader modules"); return 6;
    }
    RenderPipelineLayoutDesc layoutDesc;
    auto layout = device->createPipelineLayout(layoutDesc);
    RenderGraphicsPipelineDesc pipelineDesc;
    pipelineDesc.pipelineLayout = layout.get();
    pipelineDesc.vertexShader = vs.get();
    pipelineDesc.pixelShader = fs.get();
    pipelineDesc.renderTargetCount = 1;
    pipelineDesc.renderTargetFormat[0] = RenderFormat::B8G8R8A8_UNORM;
    pipelineDesc.renderTargetBlend[0] = RenderBlendDesc::Copy();
    pipelineDesc.primitiveTopology = RenderPrimitiveTopology::TRIANGLE_LIST;
    auto pipeline = device->createGraphicsPipeline(pipelineDesc);
    if (!static_cast<VulkanGraphicsPipeline *>(pipeline.get())->vk) { report("FAIL pipeline"); return 7; }
    report("shader-compile/pipeline=PASS");
    // Render offscreen and read actual GPU output back before display testing.
    RenderTextureDesc textureDesc;
    textureDesc.dimension = RenderTextureDimension::TEXTURE_2D;
    textureDesc.format = RenderFormat::B8G8R8A8_UNORM;
    textureDesc.width = 128; textureDesc.height = 128; textureDesc.depth = 1;
    textureDesc.mipLevels = 1; textureDesc.arraySize = 1;
    textureDesc.flags = RenderTextureFlag::RENDER_TARGET;
    auto texture = device->createTexture(textureDesc);
    if (!static_cast<VulkanTexture *>(texture.get())->vk) { report("FAIL offscreen texture"); return 8; }
    const RenderTexture *attachment = texture.get();
    RenderFramebufferDesc fbDesc;
    fbDesc.colorAttachments = &attachment; fbDesc.colorAttachmentsCount = 1;
    auto framebuffer = device->createFramebuffer(fbDesc);
    if (!static_cast<VulkanFramebuffer *>(framebuffer.get())->vk) { report("FAIL framebuffer"); return 9; }
    auto readback = device->createBuffer(RenderBufferDesc::ReadbackBuffer(128 * 128 * 4));
    if (!static_cast<VulkanBuffer *>(readback.get())->vk) { report("FAIL readback buffer"); return 10; }
    draw(cmd.get(), framebuffer.get(), texture.get(), layout.get(), pipeline.get(), 128, 128);
    cmd->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(texture.get(), RenderTextureLayout::COPY_SOURCE));
    // This pinned Plume implements buffer->image and image->image in its
    // generic copyTextureRegion, but not image->buffer. Its fallback would
    // dereference a null destination texture. Use Vulkan's explicit reverse
    // copy here; the rendering, resources and submission still use Plume.
    nativeCmd->endActiveRenderPass();
    VkBufferImageCopy imageCopy = {};
    imageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopy.imageSubresource.layerCount = 1;
    imageCopy.imageExtent = {128, 128, 1};
    vkCmdCopyImageToBuffer(nativeCmd->vk, static_cast<VulkanTexture *>(texture.get())->vk,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, static_cast<VulkanBuffer *>(readback.get())->vk,
                          1, &imageCopy);
    VkMemoryBarrier host = {};
    host.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(nativeCmd->vk, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                         0, 1, &host, 0, nullptr, 0, nullptr);
    cmd->end();
    const RenderCommandList *list = cmd.get();
    queue->executeCommandLists(&list, 1, nullptr, 0, nullptr, 0, fence.get());
    if (!wait()) { report("FAIL fence reset"); return 11; }
    auto *nativeReadback = static_cast<VulkanBuffer *>(readback.get());
    if (vmaInvalidateAllocation(nativeDevice->allocator, nativeReadback->allocation, 0, VK_WHOLE_SIZE) != VK_SUCCESS) {
        report("FAIL readback invalidate"); return 23;
    }
    const auto *pixels = static_cast<const uint8_t *>(readback->map());
    if (!pixels) { report("FAIL readback map"); return 12; }
    const uint8_t *center = pixels + (64 * 128 + 64) * 4;
    const uint8_t *corner = pixels;
    const bool correct = center[0] == 0 && center[1] == 0 && center[2] == 255 && center[3] == 255 &&
                         corner[0] >= 49 && corner[0] <= 53 && corner[1] == 0 && corner[2] == 0 && corner[3] == 255;
    reportf("GPU-readback=%s center-BGRA=%u,%u,%u,%u corner-BGRA=%u,%u,%u,%u",
                correct ? "PASS" : "FAIL", center[0], center[1], center[2], center[3], corner[0], corner[1], corner[2], corner[3]);
    std::fflush(stdout); readback->unmap();
    if (!correct) return 13;
    // A separate surface uses the enumerated PS5 display mode, no SDL VideoOut.
    auto swap = queue->createSwapChain(RenderSwapChainDesc(nullptr, RenderFormat::B8G8R8A8_UNORM, 3));
    if (!static_cast<VulkanSwapChain *>(swap.get())->surface || !swap->resize() || swap->isEmpty()) {
        report("FAIL display/swapchain"); return 14;
    }
    auto acquired = device->createCommandSemaphore();
    if (!static_cast<VulkanCommandSemaphore *>(acquired.get())->vk) { report("FAIL acquire semaphore"); return 15; }
    std::vector<std::unique_ptr<RenderCommandSemaphore>> rendered;
    std::vector<std::unique_ptr<RenderFramebuffer>> framebuffers;
    for (uint32_t i = 0; i < swap->getTextureCount(); ++i) {
        rendered.emplace_back(device->createCommandSemaphore());
        const RenderTexture *color = swap->getTexture(i);
        fbDesc.colorAttachments = &color;
        framebuffers.emplace_back(device->createFramebuffer(fbDesc));
        if (!static_cast<VulkanCommandSemaphore *>(rendered.back().get())->vk ||
            !static_cast<VulkanFramebuffer *>(framebuffers.back().get())->vk) {
            report("FAIL display resources"); return 16;
        }
    }
    report("display=READY expected: red triangle on dark blue, approximately 20 seconds");
    for (uint32_t frame = 0; frame < 200; ++frame) {
        uint32_t index = 0;
        if (!swap->acquireTexture(acquired.get(), &index) || index >= framebuffers.size()) {
            report("FAIL acquire"); return 17;
        }
        draw(cmd.get(), framebuffers[index].get(), swap->getTexture(index), layout.get(), pipeline.get(), swap->getWidth(), swap->getHeight());
        // A small white marker moves across the bottom to distinguish a live frame from a frozen image.
        const int x = int((frame % 200) * (swap->getWidth() - 32) / 200);
        RenderRect marker(x, int(swap->getHeight()) - 40, x + 32, int(swap->getHeight()) - 24);
        cmd->clearColor(0, RenderColor(1, 1, 1, 1), &marker, 1);
        cmd->barriers(RenderBarrierStage::NONE, RenderTextureBarrier(swap->getTexture(index), RenderTextureLayout::PRESENT));
        cmd->end();
        RenderCommandSemaphore *waitSem = acquired.get(), *signalSem = rendered[index].get();
        queue->executeCommandLists(&list, 1, &waitSem, 1, &signalSem, 1, fence.get());
        if (!swap->present(index, &signalSem, 1)) { report("FAIL present"); _exit(18); }
        if (!wait()) return 19;
        if (frame == 0) {
#if defined(DKR_NATIVE_TITLE)
            reportf("hide-splash rc=0x%08x", unsigned(sceSystemServiceHideSplashScreen()));
#endif
            report("first-present=PASS (TV visibility still requires observation)");
        }
        usleep(100000);
    }
    VkResult idle = vkDeviceWaitIdle(nativeDevice->vk);
    if (idle != VK_SUCCESS) { report("FAIL final idle"); _exit(21); }
    report("PASS GPU readback and 200 presents; cleaning up");
    return 0;
}

int main() {
#if defined(DKR_NATIVE_TITLE)
    // This log belongs to this diagnostic's own mounted application folder.
    // Append preserves the preceding run when comparing startup failures.
    FILE *log = std::fopen("/data/homebrew/PPSA94677/plume-probe.log", "a");
    if (!log) log = std::fopen("/app0/plume-probe.log", "a");
    if (log) { stdout = log; stderr = log; }
    sceKernelDebugOutText(0, "[DKR-R] plume-radv-native-v5 entered main\n");
#endif
    setvbuf(stdout, nullptr, _IOLBF, 0);
    setvbuf(stderr, nullptr, _IOLBF, 0);
    int status;
    try { status = run(); }
    catch (const std::exception &e) { std::printf("plume-probe: exception: %s\n", e.what()); status = 22; }
    struct ps5_heap_stats heap = {};
    ps5_heap_stats(&heap);
    reportf("completed status=%d heap-peak=%zu heap-libc-fallbacks=%llu", status, heap.peak_bytes, heap.libc_fallbacks);
    std::fflush(stdout);
#if defined(DKR_NATIVE_TITLE)
    // This native runtime's documented baseline must not return from main:
    // on this 13.60 setup exit() raised SIGSYS after successful GPU cleanup.
    // The system service (or the user's Close Game action) closes the title.
    report("resources cleaned; waiting for system-service closure");
    for (;;) usleep(100000);
#endif
    return status;
}
