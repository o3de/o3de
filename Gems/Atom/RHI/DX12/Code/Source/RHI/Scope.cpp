/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Scope.h>
#include <RHI/Image.h>
#include <RHI/ImageView.h>
#include <RHI/Buffer.h>
#include <RHI/BufferView.h>
#include <RHI/SwapChain.h>
#include <RHI/Conversions.h>
#include <RHI/ResourcePoolResolver.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>
#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/ResourcePool.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ResolveScopeAttachment.h>
#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<Scope> Scope::Create()
        {
            return aznew Scope();
        }

        void Scope::DeactivateInternal()
        {
            for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
            {
                static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Deactivate();
            }

            m_prologueTransitionBarrierRequests.clear();
            m_epilogueTransitionBarrierRequests.clear();
            m_preDiscardTransitionBarrierRequests.clear();
            m_resolveTransitionBarrierRequests.clear();
            m_aliasingBarriers.clear();
            m_depthStencilAttachment = nullptr;
            m_depthStencilAccess = RHI::ScopeAttachmentAccess::ReadWrite;
            m_colorAttachments.clear();
            m_clearRenderTargetRequests.clear();
            m_clearImageRequests.clear();
            m_clearBufferRequests.clear();
            m_discardResourceRequests.clear();
            m_waitFencesByQueue = { {0} };
            m_signalFenceValue = 0;
        }

        void Scope::SetSignalFenceValue(uint64_t fenceValue)
        {
            m_signalFenceValue = fenceValue;
        }

        bool Scope::HasSignalFence() const
        {
            return m_signalFenceValue > 0;
        }

        bool Scope::HasWaitFences() const
        {
            return
                m_waitFencesByQueue[0] > 0 ||
                m_waitFencesByQueue[1] > 0 ||
                m_waitFencesByQueue[2] > 0;
        }

        uint64_t Scope::GetSignalFenceValue() const
        {
            return m_signalFenceValue;
        }

        void Scope::SetWaitFenceValueByQueue(RHI::HardwareQueueClass hardwareQueueClass, uint64_t fenceValue)
        {
            m_waitFencesByQueue[static_cast<uint32_t>(hardwareQueueClass)] = fenceValue;
        }

        uint64_t Scope::GetWaitFenceValueByQueue(RHI::HardwareQueueClass hardwareQueueClass) const
        {
            return m_waitFencesByQueue[static_cast<uint32_t>(hardwareQueueClass)];
        }

        const FenceValueSet& Scope::GetWaitFences() const
        {
            return m_waitFencesByQueue;
        }

        const bool Scope::IsStateSupportedByQueue(D3D12_RESOURCE_STATES state) const
        {
            const D3D12_RESOURCE_STATES VALID_COMPUTE_QUEUE_RESOURCE_STATES =
                (D3D12_RESOURCE_STATE_UNORDERED_ACCESS |
                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                 D3D12_RESOURCE_STATE_COPY_DEST |
                 D3D12_RESOURCE_STATE_COPY_SOURCE);

            const D3D12_RESOURCE_STATES VALID_GRAPHICS_QUEUE_RESOURCE_STATES =
                (D3D12_RESOURCE_STATES)DX12_RESOURCE_STATE_VALID_API_MASK;

            switch (GetHardwareQueueClass())
            {
            case RHI::HardwareQueueClass::Graphics:
                return RHI::CheckBitsAll(VALID_GRAPHICS_QUEUE_RESOURCE_STATES, state);

            case RHI::HardwareQueueClass::Compute:
                return RHI::CheckBitsAll(VALID_COMPUTE_QUEUE_RESOURCE_STATES, state);

            case RHI::HardwareQueueClass::Copy:
                return RHI::CheckBitsAny(state, (D3D12_RESOURCE_STATES)DX12_RESOURCE_STATE_COPY_QUEUE_BIT);
            }
            return false;
        }

        void Scope::QueueAliasingBarrier(const D3D12_RESOURCE_ALIASING_BARRIER& barrier)
        {
            m_aliasingBarriers.push_back(barrier);
        }

        void Scope::QueueResolveTransition(const D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier)
        {
            AZ_Assert(
                transitionBarrier.StateAfter == D3D12_RESOURCE_STATE_RESOLVE_SOURCE || transitionBarrier.StateAfter == D3D12_RESOURCE_STATE_RESOLVE_DEST,
                "Invalid state for resolve barrier");
            m_resolveTransitionBarrierRequests.push_back(transitionBarrier);
        }

        void Scope::QueuePrologueTransition(const D3D12_RESOURCE_TRANSITION_BARRIER& barrier)
        {
            m_prologueTransitionBarrierRequests.push_back(barrier);
        }

        void Scope::QueueEpilogueTransition(const D3D12_RESOURCE_TRANSITION_BARRIER& barrier)
        {
            m_epilogueTransitionBarrierRequests.push_back(barrier);
        }

        void Scope::QueuePreDiscardTransition(const D3D12_RESOURCE_TRANSITION_BARRIER& barrier)
        {
            m_preDiscardTransitionBarrierRequests.push_back(barrier);
        }
  
        bool Scope::IsInDiscardResourceRequests(ID3D12Resource* nativeResource) const
        {
            auto findIter = AZStd::find_if(m_discardResourceRequests.begin(), m_discardResourceRequests.end(), [nativeResource](const ID3D12Resource* entry)
            {
                return entry == nativeResource;
            });

            if (findIter == m_discardResourceRequests.end())
            {
                return false;
            }
            
            return true;
        }
    
        bool Scope::IsResourceDiscarded(const RHI::ImageScopeAttachment& scopeAttachment) const
        {
            const ImageView* imageView = static_cast<const ImageView*>(scopeAttachment.GetImageView());
            return IsInDiscardResourceRequests(imageView->GetMemory());
        }

        bool Scope::IsResourceDiscarded(const RHI::BufferScopeAttachment& scopeAttachment) const
        {
            const BufferView* bufferView = static_cast<const BufferView*>(scopeAttachment.GetBufferView());
            return IsInDiscardResourceRequests(bufferView->GetMemory());
        }

        void Scope::CompileAttachmentInternal(
            bool isFullResourceClear,
            const RHI::ScopeAttachment& scopeAttachment,
            ID3D12Resource* resource)
        {
            const bool isFirstUsage = scopeAttachment.GetPrevious() == nullptr;
            const bool isTransient = scopeAttachment.GetFrameAttachment().GetLifetimeType() == RHI::AttachmentLifetimeType::Transient;

            // We are required to discard transient resources on first use, but only if aren't
            // clearing the *full* resource. Since it's possible that our first use is just a
            // partial clear, we may still need to 
            if (isFirstUsage && isTransient && !isFullResourceClear)
            {
                m_discardResourceRequests.push_back(resource);
            }
        }

        void Scope::CompileInternal([[maybe_unused]] RHI::Device& device)
        {
            for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
            {
                static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Compile(*this);
            }

            m_prologueTransitionBarrierRequests.reserve(GetAttachments().size());
            m_epilogueTransitionBarrierRequests.reserve(GetAttachments().size());
            m_preDiscardTransitionBarrierRequests.reserve(GetAttachments().size());

            for (const RHI::ImageScopeAttachment* scopeAttachment : GetImageAttachments())
            {
                const AZStd::vector<RHI::ScopeAttachmentUsageAndAccess>& usagesAndAccesses = scopeAttachment->GetUsageAndAccess();
                const ImageView* imageView = static_cast<const ImageView*>(scopeAttachment->GetImageView());
                const RHI::ImageScopeAttachmentDescriptor& bindingDescriptor = scopeAttachment->GetDescriptor();

                const bool isFullView           = imageView->IsFullView();
                const bool isClearAction        = bindingDescriptor.m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear;
                const bool isClearActionStencil = bindingDescriptor.m_loadStoreAction.m_loadActionStencil == RHI::AttachmentLoadAction::Clear;
                const bool isClear              = isClearAction || isClearActionStencil;
                bool isFullClear                = isClearAction && isFullView;

                CommandList::ImageClearRequest clearRequest;
                clearRequest.m_clearValue = bindingDescriptor.m_loadStoreAction.m_clearValue;
                clearRequest.m_imageView = imageView;

                AZStd::vector<CommandList::ImageClearRequest>* clearRequestQueue = nullptr;

                for (const RHI::ScopeAttachmentUsageAndAccess& usageAndAccess : usagesAndAccesses)
                {
                   switch (usageAndAccess.m_usage)
                    {
                    case RHI::ScopeAttachmentUsage::Shader:
                        if (RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Write))
                        { 
                            clearRequestQueue = &m_clearImageRequests;
                        }
                            
                        break;

                    case RHI::ScopeAttachmentUsage::RenderTarget:
                        if (RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Write))
                        {
                            clearRequestQueue = &m_clearRenderTargetRequests;
                        }

                        // Accumulate color attachments for the render target bind command.
                        m_colorAttachments.push_back(imageView);
                        break;

                    case RHI::ScopeAttachmentUsage::DepthStencil:
                        if (RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Write))
                        {
                            clearRequestQueue = &m_clearRenderTargetRequests;
                            clearRequest.m_clearFlags |= isClearAction ? D3D12_CLEAR_FLAG_DEPTH : clearRequest.m_clearFlags;
                            clearRequest.m_clearFlags |= isClearActionStencil ? D3D12_CLEAR_FLAG_STENCIL : clearRequest.m_clearFlags;
                            isFullClear &= isClearActionStencil;
                        }
                            
                        // Set the depth stencil attachment.
                        AZ_Error("Scope", m_depthStencilAttachment == nullptr, "More than one depth stencil attachment was used on scope '%s'", GetId().GetCStr());
                        m_depthStencilAttachment = imageView;
                        m_depthStencilAccess = usageAndAccess.m_access;
                        break;

                    case RHI::ScopeAttachmentUsage::Uninitialized:
                        AZ_Assert(false, "ScopeAttachmentUsage is Uninitialized");
                        break;
                    }
                }
                
                //Since we can only have one usage with write access there should only be one clearRequestQueue
                if (isClear && clearRequestQueue)
                {
                    clearRequestQueue->push_back(clearRequest);
                }
                CompileAttachmentInternal(isFullClear, *scopeAttachment, imageView->GetMemory());
            }

            for (const RHI::BufferScopeAttachment* scopeAttachment : GetBufferAttachments())
            {
                const AZStd::vector<RHI::ScopeAttachmentUsageAndAccess>& usagesAndAccesses = scopeAttachment->GetUsageAndAccess();
                const BufferView* bufferView = static_cast<const BufferView*>(scopeAttachment->GetBufferView());
                const RHI::BufferScopeAttachmentDescriptor& bindingDescriptor = scopeAttachment->GetDescriptor();

                const bool isClearAction = bindingDescriptor.m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear;
                
                bool isFullClear = false;

                for (const RHI::ScopeAttachmentUsageAndAccess& usageAndAccess : usagesAndAccesses)
                {
                    const bool isShaderUsage = usageAndAccess.m_usage == RHI::ScopeAttachmentUsage::Shader;
                    if (isClearAction && isShaderUsage)
                    {
                        AZ_Assert(RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Write), "We shouldnt be clearing without write access");
                        CommandList::BufferClearRequest request;
                        request.m_clearValue = bindingDescriptor.m_loadStoreAction.m_clearValue;
                        request.m_bufferView = bufferView;
                        m_clearBufferRequests.push_back(request);

                        isFullClear = bufferView->IsFullView();
                        break;
                    }
                }

                CompileAttachmentInternal(isFullClear, *scopeAttachment, bufferView->GetMemory());
            }
        }

        void Scope::Begin(
            CommandList& commandList,
            uint32_t commandListIndex,
            uint32_t commandListCount) const
        {
            AZ_UNUSED(commandListCount);
            AZ_TRACE_METHOD();

            commandList.GetValidator().BeginScope(*this);

            PIXBeginEvent(0xFFFF00FF, GetId().GetCStr());

            if (RHI::Factory::Get().IsPixModuleLoaded() || RHI::Factory::Get().IsRenderDocModuleLoaded())
            {
                PIXBeginEvent(commandList.GetCommandList(), 0xFFFF00FF, GetId().GetCStr());
            }

            commandList.SetAftermathEventMarker(GetId().GetCStr());
            
            const bool isPrologue = commandListIndex == 0;
            if (isPrologue)
            {
                for (const auto& barrier : m_preDiscardTransitionBarrierRequests)
                {
                    commandList.QueueTransitionBarrier(barrier);
                }

                commandList.FlushBarriers();

                for (ID3D12Resource* resource : m_discardResourceRequests)
                {
                    commandList.DiscardResource(resource);
                }

                for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
                {
                    static_cast<ResourcePoolResolver*>(resolvePolicyBase)->QueuePrologueTransitionBarriers(commandList);
                }

                for (const auto& barrier : m_aliasingBarriers)
                {
                    commandList.QueueAliasingBarrier(barrier);
                }

                for (const auto& barrier : m_prologueTransitionBarrierRequests)
                {
                    commandList.QueueTransitionBarrier(barrier);
                }

                commandList.FlushBarriers();
                
                for (const auto& request : m_clearRenderTargetRequests)
                {
                    commandList.ClearRenderTarget(request);
                }

                for (const auto& request : m_clearImageRequests)
                {
                    commandList.ClearUnorderedAccess(request);
                }

                for (const auto& request : m_clearBufferRequests)
                {
                    commandList.ClearUnorderedAccess(request);
                }

                for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
                {
                    static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Resolve(commandList);
                }
            }

            // Bind output merger attachments to *all* command lists in the batch.
            if (m_colorAttachments.size() || m_depthStencilAttachment)
            {
                commandList.SetRenderTargets(
                    static_cast<uint32_t>(m_colorAttachments.size()),
                    m_colorAttachments.data(),
                    m_depthStencilAttachment,
                    m_depthStencilAccess);
            }
        }

        void Scope::End(
            CommandList& commandList,
            uint32_t commandListIndex,
            uint32_t commandListCount) const
        {
            AZ_TRACE_METHOD();

            const bool isEpilogue = (commandListIndex + 1) == commandListCount;
            if (isEpilogue)
            {
                // Need to do all resolve operations
                for (const auto& request : m_resolveTransitionBarrierRequests)
                {
                    commandList.QueueTransitionBarrier(request);
                }
                commandList.FlushBarriers();
                for (RHI::ResolveScopeAttachment* resolveAttachment : GetResolveAttachments())
                {
                    for (RHI::ImageScopeAttachment* imageAttachment : GetImageAttachments())
                    {
                        if (imageAttachment->GetDescriptor().m_attachmentId == resolveAttachment->GetDescriptor().m_resolveAttachmentId)
                        {
                            auto srcImageView = static_cast<const ImageView*>(imageAttachment->GetImageView());
                            auto dstImageView = static_cast<const ImageView*>(resolveAttachment->GetImageView());
                            commandList.GetCommandList()->ResolveSubresource(
                                dstImageView->GetMemory(),
                                0, 
                                srcImageView->GetMemory(),
                                0, 
                                dstImageView->GetFormat());
                            break;
                        }
                    }                    
                }

                for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
                {
                    static_cast<ResourcePoolResolver*>(resolvePolicyBase)->QueueEpilogueTransitionBarriers(commandList);
                }

                for (const auto& request : m_epilogueTransitionBarrierRequests)
                {
                    commandList.QueueTransitionBarrier(request);
                }
            }

            if (RHI::Factory::Get().IsPixModuleLoaded() || RHI::Factory::Get().IsRenderDocModuleLoaded())
            {
                PIXEndEvent(commandList.GetCommandList());
            }
            PIXEndEvent();

            commandList.GetValidator().EndScope();
        }
    }
}
