// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5_frontend_gpu.hpp"
#include <imgui_internal.h>
#include <cassert>
#include <cstdio>
namespace plume {std::unique_ptr<RenderInterface> CreateVulkanInterface(RenderWindow);}
int main() {
    using namespace plume;constexpr unsigned W=640,H=360;
    auto api=CreateVulkanInterface(nullptr);assert(api);auto device=api->createDevice();assert(device);
    auto queue=device->createCommandQueue(RenderCommandListType::DIRECT);auto command=queue->createCommandList();auto fence=device->createCommandFence();assert(command&&fence);
    auto texture=device->createTexture(RenderTextureDesc::ColorTarget(W,H,RenderFormat::B8G8R8A8_UNORM));const RenderTexture* target=texture.get();
    auto framebuffer=device->createFramebuffer(RenderFramebufferDesc(&target,1));auto readback=device->createBuffer(RenderBufferDesc::ReadbackBuffer(W*H*4));assert(framebuffer&&readback);
    auto* surface=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_RGBA32);auto* renderer=SDL_CreateSoftwareRenderer(surface);assert(renderer);
    auto* image=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,128,128);assert(image);
    std::vector<Uint32> bytes(128*128,0xff3476bc);assert(!SDL_UpdateTexture(image,nullptr,bytes.data(),128*4));SDL_SetTextureScaleMode(image,SDL_ScaleModeLinear);
    auto generation=dkrr::sdl_textures::version(image);assert(generation);
    // The readback adapter must ignore modulation and restore all SDL state.
    SDL_SetTextureColorMod(image,22,33,44);SDL_SetTextureAlphaMod(image,77);SDL_SetTextureBlendMode(image,SDL_BLENDMODE_ADD);
    SDL_RenderSetScale(renderer,1.5F,1.5F);SDL_Rect clip{3,4,100,101};SDL_RenderSetClipRect(renderer,&clip);
    std::vector<unsigned char> copied;int iw=0,ih=0;assert(dkrr::sdl_textures::pixels(renderer,image,copied,iw,ih)&&iw==128&&ih==128);
    assert(copied[0]==0x34&&copied[1]==0x76&&copied[2]==0xbc&&copied[3]==255);
    Uint8 red,green,blue,alpha;SDL_GetTextureColorMod(image,&red,&green,&blue);SDL_GetTextureAlphaMod(image,&alpha);assert(red==22&&green==33&&blue==44&&alpha==77);
    SDL_Rect restored{};SDL_RenderGetClipRect(renderer,&restored);assert(!std::memcmp(&restored,&clip,sizeof(clip)));assert(SDL_GetRenderTarget(renderer)==nullptr);
    SDL_RenderSetScale(renderer,1,1);SDL_RenderSetClipRect(renderer,nullptr);
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={W,H};io.DeltaTime=1.F/60;
    dkrr::Ps5FrontendGpu gpu;assert(gpu.open(device.get(),queue.get(),3));
    for(unsigned frame=0;frame<4;++frame) {
        if(frame==2){bytes.assign(bytes.size(),0xffbc7634);assert(!SDL_UpdateTexture(image,nullptr,bytes.data(),128*4));assert(dkrr::sdl_textures::version(image)!=generation);}
        gpu.newFrame();ImGui::NewFrame();auto* draw=ImGui::GetBackgroundDrawList();
        draw->AddRectFilled({0,0},{W,H},IM_COL32(23,74,162,255));
        draw->AddImage(reinterpret_cast<ImTextureID>(image),{320,0},{640,360});
        draw->AddText({20,20},IM_COL32_WHITE,"DKR-R GPU FRONTEND");ImGui::Render();auto* data=ImGui::GetDrawData();
        command->begin();assert(gpu.prepare(renderer,data,command.get()));
        command->barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(texture.get(),RenderTextureLayout::COLOR_WRITE));command->setFramebuffer(framebuffer.get());command->clearColor();
        gpu.draw(data,command.get(),W,H);
        // Texture IDs must remain valid SDL references for the next UI build.
        bool found=false;for(int n=0;n<data->CmdListsCount;++n)for(auto& cmd:data->CmdLists[n]->CmdBuffer)if(cmd.TextureId==reinterpret_cast<ImTextureID>(image))found=true;assert(found);
        command->barriers(RenderBarrierStage::COPY,RenderTextureBarrier(texture.get(),RenderTextureLayout::COPY_SOURCE));
        // Plume's generic copyTextureRegion supports buffer-to-image, not
        // image-to-buffer. Use Vulkan's explicit readback operation here.
        auto* vkcommand=static_cast<VulkanCommandList*>(command.get());vkcommand->endActiveRenderPass();
        VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={W,H,1};
        vkCmdCopyImageToBuffer(vkcommand->vk,static_cast<VulkanTexture*>(texture.get())->vk,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,static_cast<VulkanBuffer*>(readback.get())->vk,1,&copy);command->end();
        const RenderCommandList* list=command.get();queue->executeCommandLists(&list,1,nullptr,0,nullptr,0,fence.get());queue->waitForCommandFence(fence.get());
        auto* native=static_cast<VulkanDevice*>(device.get());auto* buffer=static_cast<VulkanBuffer*>(readback.get());assert(vmaInvalidateAllocation(native->allocator,buffer->allocation,0,VK_WHOLE_SIZE)==VK_SUCCESS);
        const auto* output=static_cast<const Uint32*>(readback->map());assert(output);assert(output[(H/2)*W+W/4]==0xff174aa2);assert(output[(H/2)*W+W*3/4]==(frame<2?0xffbc7634:0xff3476bc));readback->unmap();
        assert(gpu.uploadCount()==(frame<2?1:2));
    }
    gpu.close();ImGui::DestroyContext();SDL_DestroyTexture(image);assert(!dkrr::sdl_textures::version(image));SDL_DestroyRenderer(renderer);SDL_FreeSurface(surface);
    puts("PASS actual ImGui Vulkan frontend: GPU pixel readback, exact SDL image conversion/state restoration, retained SDL texture IDs, one upload per image generation and clean shutdown");
}
