// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "plume_render_interface.h"
#include "plume_vulkan.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "ps5_sdl_texture_tracking.hpp"
#include "ps5_sdl_texture_pixels.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>
namespace dkrr {
// Uses the exact ImGui Vulkan backend used by the running game's Inspector.
// The caller retires the previous command fence before prepare()/close().
class Ps5FrontendGpu {
    struct Image {
        std::unique_ptr<plume::RenderTexture> texture;
        std::unique_ptr<plume::RenderBuffer> upload;
        VkDescriptorSet set=VK_NULL_HANDLE;
        std::uint64_t version=0,last_used=0,bytes=0;
    };
    plume::VulkanDevice* device=nullptr;
    VkDescriptorPool pool=VK_NULL_HANDLE;
    VkRenderPass pass=VK_NULL_HANDLE;
    VkSampler linear=VK_NULL_HANDLE,nearest=VK_NULL_HANDLE;
    std::unordered_map<SDL_Texture*,Image> images;
    struct Replacement {ImDrawCmd* command;ImTextureID original;};
    std::vector<Replacement> replacements;
    std::uint64_t frame=0,total_bytes=0,uploads=0;
    bool initialized=false;
    void erase(std::unordered_map<SDL_Texture*,Image>::iterator it) {
        if(it->second.set)ImGui_ImplVulkan_RemoveTexture(it->second.set);
        total_bytes-=it->second.bytes;images.erase(it);
    }
    bool image(SDL_Renderer* renderer,SDL_Texture* source,plume::RenderCommandList* command,VkDescriptorSet& result) {
        using namespace plume;
        const auto version=sdl_textures::version(source);if(!version)return false;
        auto it=images.find(source);
        if(it!=images.end()&&it->second.version!=version){erase(it);it=images.end();}
        if(it!=images.end()){it->second.last_used=frame;result=it->second.set;return true;}
        std::vector<unsigned char> pixels;int w=0,h=0;
        if(!sdl_textures::pixels(renderer,source,pixels,w,h))return false;
        const auto bytes=std::uint64_t(pixels.size());
        constexpr std::uint64_t budget=128U*1024U*1024U;
        while(images.size()>=250||total_bytes+bytes>budget) {
            auto oldest=images.end();
            for(auto entry=images.begin();entry!=images.end();++entry)
                if(entry->second.last_used!=frame&&(oldest==images.end()||entry->second.last_used<oldest->second.last_used))oldest=entry;
            if(oldest==images.end())return false;
            erase(oldest);
        }
        Image next;next.bytes=bytes;next.last_used=frame;next.version=version;
        next.texture=device->createTexture(RenderTextureDesc::Texture2D(unsigned(w),unsigned(h),1,RenderFormat::B8G8R8A8_UNORM));
        next.upload=device->createBuffer(RenderBufferDesc::UploadBuffer(bytes));
        if(!next.texture||!next.upload)return false;
        auto* texture=static_cast<VulkanTexture*>(next.texture.get());auto* buffer=static_cast<VulkanBuffer*>(next.upload.get());
        if(!texture->vk||!texture->imageView||!buffer->vk)return false;
        auto* mapped=next.upload->map();if(!mapped)return false;
        std::memcpy(mapped,pixels.data(),pixels.size());const auto flushed=vmaFlushAllocation(device->allocator,buffer->allocation,0,bytes);next.upload->unmap();
        if(flushed!=VK_SUCCESS)return false;
        SDL_ScaleMode scale=SDL_ScaleModeLinear;SDL_GetTextureScaleMode(source,&scale);
        next.set=ImGui_ImplVulkan_AddTexture(scale==SDL_ScaleModeNearest?nearest:linear,texture->imageView,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        if(!next.set)return false;
        // Publish resources before recording references to them. A later failure
        // leaves the command unsubmitted and the owner retains all allocations.
        auto added=images.emplace(source,std::move(next));auto& stored=added.first->second;total_bytes+=stored.bytes;
        command->barriers(RenderBarrierStage::COPY,RenderTextureBarrier(stored.texture.get(),RenderTextureLayout::COPY_DEST));
        command->copyTextureRegion(RenderTextureCopyLocation::Subresource(stored.texture.get()),RenderTextureCopyLocation::PlacedFootprint(stored.upload.get(),RenderFormat::B8G8R8A8_UNORM,unsigned(w),unsigned(h),1,unsigned(w)));
        command->barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(stored.texture.get(),RenderTextureLayout::SHADER_READ));
        result=stored.set;++uploads;return true;
    }
public:
    ~Ps5FrontendGpu(){close();}
    bool open(plume::RenderDevice* render_device,plume::RenderCommandQueue* render_queue,unsigned count) {
        using namespace plume;
        if(device||!render_device||!render_queue||count<2)return false;
        device=static_cast<VulkanDevice*>(render_device);auto* queue=static_cast<VulkanCommandQueue*>(render_queue);
        if(!ImGui_ImplVulkan_LoadFunctions([](const char* name,void* instance){return vkGetInstanceProcAddr(*static_cast<VkInstance*>(instance),name);},&device->renderInterface->instance))return false;
        const auto format=VK_FORMAT_B8G8R8A8_UNORM;
        pass=VulkanGraphicsPipeline::createRenderPass(device,&format,1,VK_FORMAT_UNDEFINED,VK_SAMPLE_COUNT_1_BIT);if(!pass)return false;
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,256};VkDescriptorPoolCreateInfo info{};
        info.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;info.flags=VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;info.maxSets=256;info.poolSizeCount=1;info.pPoolSizes=&size;
        if(vkCreateDescriptorPool(device->vk,&info,nullptr,&pool)!=VK_SUCCESS)return false;
        VkSamplerCreateInfo sampler{};sampler.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler.minFilter=sampler.magFilter=VK_FILTER_LINEAR;sampler.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler.addressModeU=sampler.addressModeV=sampler.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;sampler.maxLod=0;
        if(vkCreateSampler(device->vk,&sampler,nullptr,&linear)!=VK_SUCCESS)return false;
        sampler.minFilter=sampler.magFilter=VK_FILTER_NEAREST;
        if(vkCreateSampler(device->vk,&sampler,nullptr,&nearest)!=VK_SUCCESS)return false;
        ImGui_ImplVulkan_InitInfo gui{};gui.Instance=device->renderInterface->instance;gui.PhysicalDevice=device->physicalDevice;
        gui.Device=device->vk;gui.QueueFamily=queue->familyIndex;gui.Queue=queue->queue->vk;gui.DescriptorPool=pool;gui.RenderPass=pass;
        gui.MinImageCount=2;gui.ImageCount=count;gui.CheckVkResultFn=[](VkResult rc){if(rc!=VK_SUCCESS)std::fprintf(stderr,"[ps5][frontend] ImGui Vulkan rc=%d\n",int(rc));};
        initialized=ImGui_ImplVulkan_Init(&gui);return initialized;
    }
    void newFrame(){ImGui_ImplVulkan_NewFrame();}
    bool prepare(SDL_Renderer* renderer,ImDrawData* data,plume::RenderCommandList* command) {
        if(!initialized||!data||!command||!renderer)return false;
        restore();++frame;
        for(auto it=images.begin();it!=images.end();) {
            it->second.upload.reset(); // caller has retired the previous fence
            if(sdl_textures::version(it->first)!=it->second.version){auto old=it++;erase(old);}else ++it;
        }
        const auto font=ImGui::GetIO().Fonts->TexID;
        for(int n=0;n<data->CmdListsCount;++n)for(auto& draw:data->CmdLists[n]->CmdBuffer) {
            if(draw.UserCallback||!draw.ElemCount||draw.TextureId==font)continue;
            VkDescriptorSet set=VK_NULL_HANDLE;
            if(!image(renderer,reinterpret_cast<SDL_Texture*>(draw.TextureId),command,set)){restore();return false;}
            replacements.push_back({&draw,draw.TextureId});draw.TextureId=reinterpret_cast<ImTextureID>(set);
        }
        return true;
    }
    void draw(ImDrawData* data,plume::RenderCommandList* command,unsigned width,unsigned height) {
        // Layout remains 1080p, just as in-game; the backend scales vertices and
        // clipping to the real output extent without shrinking the menu.
        const auto original=data->FramebufferScale;
        data->FramebufferScale={float(width)/data->DisplaySize.x,float(height)/data->DisplaySize.y};
        auto* native=static_cast<plume::VulkanCommandList*>(command);native->checkActiveRenderPass();
        ImGui_ImplVulkan_RenderDrawData(data,native->vk);
        data->FramebufferScale=original;restore();
    }
    void restore(){for(auto& item:replacements)item.command->TextureId=item.original;replacements.clear();}
    void close() {
        if(!device)return;restore();
        if(initialized){while(!images.empty())erase(images.begin());ImGui_ImplVulkan_Shutdown();initialized=false;}
        if(linear)vkDestroySampler(device->vk,linear,nullptr);if(nearest)vkDestroySampler(device->vk,nearest,nullptr);
        if(pool)vkDestroyDescriptorPool(device->vk,pool,nullptr);if(pass)vkDestroyRenderPass(device->vk,pass,nullptr);
        std::fprintf(stderr,"[ps5][frontend] GPU UI retired: source-image uploads=%llu; no per-frame software framebuffer copy\n",static_cast<unsigned long long>(uploads));
        device=nullptr;pool=VK_NULL_HANDLE;pass=VK_NULL_HANDLE;linear=nearest=VK_NULL_HANDLE;
    }
    std::uint64_t uploadCount()const{return uploads;}
};
}
