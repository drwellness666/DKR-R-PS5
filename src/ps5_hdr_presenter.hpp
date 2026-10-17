// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "ps5_hdr_options.h"
#include "ps5_hdr_shaders.h"
#include "plume_render_interface.h"
#include "plume_vulkan.h"
#include <memory>
namespace RT64 {
// Owned and used only by the serial present worker. SDR allocates none of this.
// Two BGRA targets keep the established VI/UI pipelines and alpha composition.
// Every submitted command is completed before these targets are reused/resized.
class Ps5HdrPresenter {
    std::unique_ptr<plume::RenderPipelineLayout> layout;
    std::unique_ptr<plume::RenderPipeline> pipeline;
    std::unique_ptr<plume::RenderDescriptorSet> set;
    std::unique_ptr<plume::RenderTexture> scene,ui;
    std::unique_ptr<plume::RenderFramebuffer> sceneFb,uiFb;
    unsigned width=0,height=0;
public:
    bool prepare(plume::RenderDevice *device,unsigned w,unsigned h){
        using namespace plume;
        if(!w||!h||w>3840||h>2160)return false;
        if(!pipeline){
            RenderDescriptorSetBuilder descriptors;descriptors.begin();descriptors.addTexture(0);descriptors.addTexture(1);descriptors.end();
            RenderPipelineLayoutBuilder builder;builder.begin();builder.addPushConstant(0,0,16,RenderShaderStageFlag::PIXEL);builder.addDescriptorSet(descriptors);builder.end();
            layout=builder.create(device);set=descriptors.create(device);
            auto vs=device->createShader(dkrr_hdr_vert,sizeof(dkrr_hdr_vert),"main",RenderShaderFormat::SPIRV);
            auto ps=device->createShader(dkrr_hdr_frag,sizeof(dkrr_hdr_frag),"main",RenderShaderFormat::SPIRV);
            if(!layout||!set||!vs||!ps)return false;
            if(!static_cast<VulkanDescriptorSet*>(set.get())->vk || !static_cast<VulkanPipelineLayout*>(layout.get())->vk || !static_cast<VulkanShader*>(vs.get())->vk || !static_cast<VulkanShader*>(ps.get())->vk)return false;
            RenderGraphicsPipelineDesc p;p.pipelineLayout=layout.get();p.vertexShader=vs.get();p.pixelShader=ps.get();
            p.renderTargetFormat[0]=RenderFormat::R10G10B10A2_UNORM;p.renderTargetCount=1;
            pipeline=device->createGraphicsPipeline(p);if(!pipeline||!static_cast<VulkanGraphicsPipeline*>(pipeline.get())->vk)return false;
        }
        if(width==w&&height==h)return true;
        // Caller's previous present fence has completed before this boundary.
        sceneFb.reset();uiFb.reset();scene.reset();ui.reset();width=height=0;
        auto desc=RenderTextureDesc::ColorTarget(w,h,RenderFormat::B8G8R8A8_UNORM);
        scene=device->createTexture(desc);ui=device->createTexture(desc);if(!scene||!ui)return false;
        for(auto* texture:{scene.get(),ui.get()}){const auto* vk=static_cast<VulkanTexture*>(texture);if(!vk->vk||!vk->imageView)return false;}
        const RenderTexture* s=scene.get();const RenderTexture* u=ui.get();
        sceneFb=device->createFramebuffer(RenderFramebufferDesc(&s,1));uiFb=device->createFramebuffer(RenderFramebufferDesc(&u,1));
        if(!sceneFb||!uiFb||!static_cast<VulkanFramebuffer*>(sceneFb.get())->vk||!static_cast<VulkanFramebuffer*>(uiFb.get())->vk)return false;
        set->setTexture(0,scene.get(),RenderTextureLayout::SHADER_READ);set->setTexture(1,ui.get(),RenderTextureLayout::SHADER_READ);
        width=w;height=h;return true;
    }
    plume::RenderFramebuffer* beginScene(plume::RenderCommandList* c){
        using namespace plume;c->barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(scene.get(),RenderTextureLayout::COLOR_WRITE));
        return sceneFb.get();
    }
    plume::RenderFramebuffer* beginUi(plume::RenderCommandList* c){
        using namespace plume;c->barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(ui.get(),RenderTextureLayout::COLOR_WRITE));
        c->setFramebuffer(uiFb.get());c->clearColor(0,RenderColor(0,0,0,0));return uiFb.get();
    }
    void finish(plume::RenderCommandList* c,plume::RenderFramebuffer* target){
        using namespace plume;
        c->barriers(RenderBarrierStage::GRAPHICS,{RenderTextureBarrier(scene.get(),RenderTextureLayout::SHADER_READ),RenderTextureBarrier(ui.get(),RenderTextureLayout::SHADER_READ)});
        c->setFramebuffer(target);c->setViewports(RenderViewport(0,0,float(width),float(height)));c->setScissors(RenderRect(0,0,int(width),int(height)));
        const auto choice=dkrr_ps5_hdr_calibration();const float values[]={float(choice.paper_white),float(choice.peak),choice.expansion/100.0f,float(dkrr_ps5_hdr_pattern())};
        c->setPipeline(pipeline.get());c->setGraphicsPipelineLayout(layout.get());c->setGraphicsDescriptorSet(set.get(),0);c->setGraphicsPushConstants(0,values);
        c->setVertexBuffers(0,nullptr,0,nullptr);c->drawInstanced(3,1,0,0);
    }
};
}
