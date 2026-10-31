// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "ps5_hdr_presenter.hpp"
#include "ps5_frontend_gpu.hpp"
#include "ps5_render_wait.hpp"
#include <SDL.h>
#include <cstdio>
#include <memory>
#include <vector>

namespace plume { std::unique_ptr<RenderInterface> CreateVulkanInterface(); }
namespace dkrr {
// The guest and its renderer must have joined before this owner is created.
// The existing frontend draws through the game's ImGui Vulkan backend. SDL
// retains image loading and controller input; native Vulkan owns presentation:
// no second SDL display owner, payload, or background process is involved.
class Ps5FrontendPresenter {
    struct State {
        std::unique_ptr<plume::RenderInterface> api;
        std::unique_ptr<plume::RenderDevice> device;
        std::unique_ptr<plume::RenderCommandQueue> queue;
        std::unique_ptr<plume::RenderCommandList> command;
        std::unique_ptr<plume::RenderCommandFence> fence;
        std::unique_ptr<plume::RenderSwapChain> swap;
        std::unique_ptr<plume::RenderCommandSemaphore> acquired;
        std::vector<std::unique_ptr<plume::RenderCommandSemaphore>> rendered;
        std::vector<std::unique_ptr<plume::RenderFramebuffer>> targets;
        RT64::Ps5HdrPresenter hdr;
        Ps5FrontendGpu gpu;
        bool in_flight=false;
        bool wait() {
            if(!in_flight)return true;
            auto* d=static_cast<plume::VulkanDevice*>(device.get());
            auto* f=static_cast<plume::VulkanCommandFence*>(fence.get());
            const auto rc=dkrr::render_wait(9,[&]{return vkWaitForFences(d->vk,1,&f->vk,VK_TRUE,UINT64_C(5000000000));});
            if(rc!=VK_SUCCESS){std::fprintf(stderr,"[ps5][frontend] GPU fence failed: %d; retaining submitted resources\n",int(rc));return false;}
            in_flight=false;
            return vkResetFences(d->vk,1,&f->vk)==VK_SUCCESS;
        }
        bool targetsForSwapchain() {
            using namespace plume;
            for(unsigned i=0;i<swap->getTextureCount();++i){
                rendered.emplace_back(device->createCommandSemaphore());
                const RenderTexture* texture=swap->getTexture(i);
                targets.emplace_back(device->createFramebuffer(RenderFramebufferDesc(&texture,1)));
                if(!rendered.back()||!static_cast<VulkanCommandSemaphore*>(rendered.back().get())->vk||!targets.back()||!static_cast<VulkanFramebuffer*>(targets.back().get())->vk)return false;
            }
            return !targets.empty();
        }
        bool recoverSdr() {
            auto* d=static_cast<plume::VulkanDevice*>(device.get());
            if(!wait()||vkDeviceWaitIdle(d->vk)!=VK_SUCCESS)return false;
            targets.clear();rendered.clear();
            if(!swap->resize()||swap->isEmpty()||
               static_cast<plume::VulkanSwapChain*>(swap.get())->pickedSurfaceFormat.format!=VK_FORMAT_B8G8R8A8_UNORM)return false;
            std::fprintf(stderr,"[ps5][frontend] HDMI HDR unavailable; resumed SDR presentation\n");
            return targetsForSwapchain();
        }
    };
    std::unique_ptr<State> state;
    bool quarantined=false;
public:
    ~Ps5FrontendPresenter() { close(); }
    bool close() {
        if(!state)return !quarantined;
        // A failed GPU wait must not free memory still referenced by a command.
        // The title closes normally on this failure; retain the entire owner.
        if(!state->wait()){state.release();quarantined=true;return false;}
        if(state->device){
            auto* d=static_cast<plume::VulkanDevice*>(state->device.get());
            if(d->vk&&vkDeviceWaitIdle(d->vk)!=VK_SUCCESS){state.release();quarantined=true;return false;}
        }
        state->gpu.close();
        state.reset();
        return true;
    }
    bool open() {
        using namespace plume;
        if(state||quarantined)return false;
        state=std::make_unique<State>();auto& s=*state;
        s.api=CreateVulkanInterface();if(!s.api)return false;
        s.device=s.api->createDevice();if(!s.device)return false;
        s.queue=s.device->createCommandQueue(RenderCommandListType::DIRECT);if(!s.queue)return false;
        s.command=s.queue->createCommandList();s.fence=s.device->createCommandFence();
        if(!s.command||!static_cast<VulkanCommandList*>(s.command.get())->vk||!s.fence||!static_cast<VulkanCommandFence*>(s.fence.get())->vk)return false;
        s.swap=s.queue->createSwapChain(RenderSwapChainDesc(nullptr,RenderFormat::B8G8R8A8_UNORM,3));
        if(!s.swap||!static_cast<VulkanSwapChain*>(s.swap.get())->surface||!s.swap->resize()||s.swap->isEmpty())return false;
        s.acquired=s.device->createCommandSemaphore();
        if(!s.acquired||!static_cast<VulkanCommandSemaphore*>(s.acquired.get())->vk)return false;
        if(!s.targetsForSwapchain())return false;
        std::fprintf(stderr,"[ps5][frontend] Vulkan owner ready: %ux%u\n",s.swap->getWidth(),s.swap->getHeight());
        return true;
    }
    bool initializeUi() {
        return state&&state->gpu.open(state->device.get(),state->queue.get(),state->swap->getTextureCount());
    }
    void newFrame(){state->gpu.newFrame();}
    bool present(SDL_Renderer* renderer,ImDrawData* data) {
        using namespace plume;
        if(!state||!renderer||!data||data->DisplaySize.x<=0||data->DisplaySize.y<=0)return false;
        auto& s=*state;if(!s.wait())return false;
        unsigned index=0;
        if(!dkrr::render_wait(7,[&]{return s.swap->acquireTexture(s.acquired.get(),&index);}))return dkrr_ps5_hdr_status().output==3&&s.recoverSdr();
        if(index>=s.targets.size())return false;
        const bool hdr=static_cast<VulkanSwapChain*>(s.swap.get())->pickedSurfaceFormat.format==VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        if(hdr&&!s.hdr.prepare(s.device.get(),s.swap->getWidth(),s.swap->getHeight()))return false;
        auto* c=s.command.get();c->begin();
        if(!dkrr::render_wait(10,[&]{return s.gpu.prepare(renderer,data,c);})) {
            std::fprintf(stderr,"[ps5][frontend] image preparation failed: %s\n",SDL_GetError());return false;
        }
        if(hdr){c->setFramebuffer(s.hdr.beginScene(c));c->clearColor();s.hdr.beginUi(c);}
        else {c->barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(s.swap->getTexture(index),RenderTextureLayout::COLOR_WRITE));c->setFramebuffer(s.targets[index].get());c->clearColor();}
        s.gpu.draw(data,c,s.swap->getWidth(),s.swap->getHeight());
        if(hdr)s.hdr.finish(c,s.targets[index].get());
        c->barriers(RenderBarrierStage::NONE,RenderTextureBarrier(s.swap->getTexture(index),RenderTextureLayout::PRESENT));c->end();
        const RenderCommandList* list=c;RenderCommandSemaphore* wait=s.acquired.get();RenderCommandSemaphore* done=s.rendered[index].get();
        s.in_flight=true;dkrr::render_wait(11,[&]{s.queue->executeCommandLists(&list,1,&wait,1,&done,1,s.fence.get());});
        const bool shown=dkrr::render_wait(8,[&]{return s.swap->present(index,&done,1);});
        if(!s.wait())return false;
        if(!shown&&dkrr_ps5_hdr_status().output==3)return s.recoverSdr();
        if(shown)dkrr_diag_present();
        return shown;
    }
};
}
