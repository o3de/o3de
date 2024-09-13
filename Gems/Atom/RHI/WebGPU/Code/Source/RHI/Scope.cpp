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
#include <Atom/RHI/ResolveScopeAttachment.h>

namespace AZ::WebGPU
{
    void RenderPassDescriptorBuilder::Add(const RHI::ImageScopeAttachment& scopeAttachment)
    {
        const int deviceIndex = scopeAttachment.GetScope().GetDeviceIndex();
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
                m_colorAttachmentIds[m_wgpuColorAttachments.size() - 1] = scopeAttachment.GetDescriptor().m_attachmentId;
            }
            break;

        case RHI::ScopeAttachmentUsage::DepthStencil:
            {
                m_wgpuDepthStencilAttachmentInfo.view = imageView->GetNativeView();
                m_wgpuDepthStencilAttachmentInfo.depthLoadOp = ConvertLoadOp(loadStoreAction.m_loadAction);
                m_wgpuDepthStencilAttachmentInfo.depthStoreOp = ConvertStoreOp(loadStoreAction.m_storeAction);
                m_wgpuDepthStencilAttachmentInfo.depthClearValue = loadStoreAction.m_clearValue.m_depthStencil.m_depth;
                m_wgpuDepthStencilAttachmentInfo.depthReadOnly = scopeAttachment.GetAccess() == RHI::ScopeAttachmentAccess::Read;
                if (RHI::CheckBitsAll(RHI::GetImageAspectFlags(imageView->GetFormat()), RHI::ImageAspectFlags::Stencil))
                {
                    m_wgpuDepthStencilAttachmentInfo.stencilLoadOp = ConvertLoadOp(loadStoreAction.m_loadActionStencil);
                    m_wgpuDepthStencilAttachmentInfo.stencilStoreOp = ConvertStoreOp(loadStoreAction.m_storeActionStencil);
                    m_wgpuDepthStencilAttachmentInfo.stencilClearValue = loadStoreAction.m_clearValue.m_depthStencil.m_stencil;
                    m_wgpuDepthStencilAttachmentInfo.stencilReadOnly = scopeAttachment.GetAccess() == RHI::ScopeAttachmentAccess::Read;
                }
            }
            break;
        case RHI::ScopeAttachmentUsage::Resolve:
            {
                const RHI::ResolveScopeAttachment* resolveScopeAttachment =
                    azrtti_cast<const RHI::ResolveScopeAttachment*>(&scopeAttachment);
                auto targetAttachmentId = resolveScopeAttachment->GetDescriptor().m_resolveAttachmentId;
                auto findIt = AZStd::find(m_colorAttachmentIds.begin(), m_colorAttachmentIds.end(), targetAttachmentId);
                AZ_Assert(findIt != m_colorAttachmentIds.end(), "Failed to find color attachment %s to resolve", targetAttachmentId.GetCStr());
                size_t index = AZStd::distance(m_colorAttachmentIds.begin(), findIt);
                m_wgpuColorAttachments[index].resolveTarget = imageView->GetNativeView();
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

            if (m_usesRenderpass)
            {
                BuildRenderPass();
                if (m_renderPassBuilder)
                {
                    commandList.BeginRenderPass(m_renderPassBuilder);
                }
            }
            else if (m_usesComputepass)
            {
                commandList.BeginComputePass();
            }
        }
    }

    void Scope::End(CommandList& commandList, uint32_t commandListIndex, uint32_t commandListCount)
    {
        const bool isEpilogue = (commandListIndex + 1) == commandListCount;
        if (isEpilogue)
        {
            if (m_usesRenderpass)
            {
                if (m_renderPassBuilder)
                {
                    commandList.EndRenderPass();
                }
            }
            else if (m_usesComputepass)
            {
                commandList.EndComputePass();
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

    void Scope::ActivateInternal()
    {
        bool haveRenderAttachments = false;
        bool haveShaderAttachments = false;
        bool haveClearLoadOps = false;
        const auto& imageAttachments = GetImageAttachments();
        const auto& bufferAttachments = GetBufferAttachments();
        for (auto attachment : imageAttachments)
        {
            haveRenderAttachments |= attachment->GetUsage() == RHI::ScopeAttachmentUsage::RenderTarget ||
                attachment->GetUsage() == RHI::ScopeAttachmentUsage::DepthStencil ||
                attachment->GetUsage() == RHI::ScopeAttachmentUsage::Resolve;

            haveShaderAttachments |= attachment->GetUsage() == RHI::ScopeAttachmentUsage::Shader;

            haveClearLoadOps |= attachment->GetDescriptor().m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear ||
                attachment->GetDescriptor().m_loadStoreAction.m_loadActionStencil == RHI::AttachmentLoadAction::Clear;
        }

        for (auto attachment : bufferAttachments)
        {
            haveShaderAttachments |= attachment->GetUsage() == RHI::ScopeAttachmentUsage::Shader;
            haveClearLoadOps |= attachment->GetDescriptor().m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear;
        }

        if (haveClearLoadOps || GetEstimatedItemCount() > 0)
        {
            if (haveRenderAttachments)
            {
                m_usesRenderpass = true;
            }
            else if (haveShaderAttachments)
            {
                m_usesComputepass = true;
            }
        }
    }

    void Scope::DeactivateInternal()
    {
        for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
        {
            static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Deactivate();
        }
        m_usesRenderpass = false;
        m_usesComputepass = false;
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
