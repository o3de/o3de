/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/CommandList.h>
#include <RHI/ImageView.h>
#include <RHI/ResourcePoolResolver.h>
#include <RHI/Scope.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>

namespace AZ::WebGPU
{
    void RenderPassDescriptorBuilder::Add(const RHI::ImageScopeAttachment& scopeAttachment)
    {
        int deviceIndex = scopeAttachment.GetScope().GetDeviceIndex();
        const ImageView* imageView = static_cast<const ImageView*>(scopeAttachment.GetImageView()->GetDeviceImageView(deviceIndex).get());
        const RHI::ImageScopeAttachmentDescriptor& bindingDescriptor = scopeAttachment.GetDescriptor();
        const RHI::ImageViewDescriptor& viewDescriptor = bindingDescriptor.GetViewDescriptor();
        const RHI::ImageDescriptor& imageDescriptor = imageView->GetImage().GetDescriptor();
        const auto& loadStoreAction = scopeAttachment.GetScopeAttachmentDescriptor().m_loadStoreAction;
        switch (scopeAttachment.GetUsage())
        {
        case RHI::ScopeAttachmentUsage::RenderTarget:
            {
                wgpu::RenderPassColorAttachment& colorAttachment = m_wgpuColorAttachments.emplace_back(wgpu::RenderPassColorAttachment{});
                colorAttachment.view = imageView->GetNativeView();
                colorAttachment.depthSlice = imageDescriptor.m_dimension == RHI::ImageDimension::Image3D ? viewDescriptor.m_depthSliceMin
                                                                                                         : WGPU_DEPTH_SLICE_UNDEFINED;
                colorAttachment.resolveTarget = nullptr;
                colorAttachment.loadOp = ConvertLoadOp(loadStoreAction.m_loadAction);
                colorAttachment.storeOp = ConvertStoreOp(loadStoreAction.m_storeAction);
                colorAttachment.clearValue = ConvertClearValue(loadStoreAction.m_clearValue);
            }
            break;

        case RHI::ScopeAttachmentUsage::DepthStencil:
            {
                m_wgpuDepthStencilAttachmentInfo.view = imageView->GetNativeView();
                m_wgpuDepthStencilAttachmentInfo.depthLoadOp = ConvertLoadOp(loadStoreAction.m_loadAction);
                m_wgpuDepthStencilAttachmentInfo.depthStoreOp = ConvertStoreOp(loadStoreAction.m_storeAction);
                m_wgpuDepthStencilAttachmentInfo.depthClearValue = loadStoreAction.m_clearValue.m_depthStencil.m_depth;
                m_wgpuDepthStencilAttachmentInfo.depthReadOnly = scopeAttachment.GetAccess() == RHI::ScopeAttachmentAccess::Read;
                m_wgpuDepthStencilAttachmentInfo.stencilLoadOp = ConvertLoadOp(loadStoreAction.m_loadActionStencil);
                m_wgpuDepthStencilAttachmentInfo.stencilStoreOp = ConvertStoreOp(loadStoreAction.m_storeActionStencil);
                m_wgpuDepthStencilAttachmentInfo.stencilClearValue = loadStoreAction.m_clearValue.m_depthStencil.m_stencil;
                m_wgpuDepthStencilAttachmentInfo.stencilReadOnly = scopeAttachment.GetAccess() == RHI::ScopeAttachmentAccess::Read;
            }
            break;

        case RHI::ScopeAttachmentUsage::Uninitialized:
            AZ_Assert(false, "ScopeAttachmentUsage is Uninitialized");
            break;
        }
    }

    const wgpu::RenderPassDescriptor& RenderPassDescriptorBuilder::End()
    {
        colorAttachmentCount = m_wgpuColorAttachments.size();
        colorAttachments = m_wgpuColorAttachments.empty() ? nullptr : m_wgpuColorAttachments.data();
        depthStencilAttachment = m_wgpuDepthStencilAttachmentInfo.view ? &m_wgpuDepthStencilAttachmentInfo : nullptr;
        occlusionQuerySet = nullptr;
        timestampWrites = nullptr;
        m_isValid = colorAttachmentCount || depthStencilAttachment;
        return *this;
    }

    RenderPassDescriptorBuilder::operator bool() const
    {
        return m_isValid;
    }

    RHI::Ptr<Scope> Scope::Create()
    {
        return aznew Scope();
    }

    void Scope::Begin(CommandList& commandList, uint32_t commandListIndex, [[maybe_unused]] uint32_t commandListCount)
    {
        const bool isPrologue = commandListIndex == 0;
        if (isPrologue)
        {
            for (RHI::ResourcePoolResolver* resolverBase : GetResourcePoolResolves())
            {
                auto* resolver = static_cast<ResourcePoolResolver*>(resolverBase);
                resolver->Resolve(commandList);
            }

            BuildRenderPass();
            if (m_renderPassBuilder)
            {
                commandList.BeginRenderPass(m_renderPassBuilder);
            }
        }
    }

    void Scope::End(CommandList& commandList, uint32_t commandListIndex, uint32_t commandListCount)
    {
        const bool isEpilogue = (commandListIndex + 1) == commandListCount;
        if (isEpilogue)
        {
            if (m_renderPassBuilder)
            {
                commandList.EndRenderPass();
            }
        }
    }

    bool Scope::HasSignalFence() const
    {
        return false;
    }

    bool Scope::HasWaitFences() const
    {
        return false;
    }

    void Scope::DeactivateInternal()
    {
        for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
        {
            static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Deactivate();
        }
    }

    void Scope::CompileInternal()
    {
        for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
        {
            static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Compile(GetHardwareQueueClass());
        }
    }

    void Scope::BuildRenderPass()
    {
        m_renderPassBuilder = {};
        for (const RHI::ImageScopeAttachment* scopeAttachment : GetImageAttachments())
        {
            m_renderPassBuilder.Add(*scopeAttachment);
        }
        m_renderPassBuilder.End();
        m_renderPassBuilder.label = GetName().GetCStr();
    }
} // namespace AZ::WebGPU
