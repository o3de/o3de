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
#include <Atom/RHI/DeviceResourcePool.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ResolveScopeAttachment.h>
#include <Atom/RHI/RHISystemInterface.h>

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
            m_shadingRateAttachment = nullptr;
            m_depthStencilAccess = RHI::ScopeAttachmentAccess::Unknown;
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
            constexpr D3D12_RESOURCE_STATES VALID_COMPUTE_QUEUE_RESOURCE_STATES =
                (D3D12_RESOURCE_STATE_UNORDERED_ACCESS |
                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                 D3D12_RESOURCE_STATE_COPY_DEST |
                 D3D12_RESOURCE_STATE_COPY_SOURCE);

            constexpr D3D12_RESOURCE_STATES VALID_COPY_QUEUE_RESOURCE_STATES =
                (D3D12_RESOURCE_STATE_COPY_DEST |
                 D3D12_RESOURCE_STATE_COPY_SOURCE);

            switch (GetHardwareQueueClass())
            {
            case RHI::HardwareQueueClass::Graphics:
                return true;

            case RHI::HardwareQueueClass::Compute:
                return RHI::CheckBitsAll(VALID_COMPUTE_QUEUE_RESOURCE_STATES, state);

            case RHI::HardwareQueueClass::Copy:
                return RHI::CheckBitsAll(VALID_COPY_QUEUE_RESOURCE_STATES, state);
            }
            return false;
        }

        void Scope::QueueAliasingBarrier(
            const D3D12_RESOURCE_ALIASING_BARRIER& barrier, const BarrierOp::CommandListState* state /*= nullptr*/)
        {
            m_aliasingBarriers.emplace_back(barrier, state);
        }

        const D3D12_RESOURCE_TRANSITION_BARRIER& Scope::QueueResolveTransition(
            const D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier, const BarrierOp::CommandListState* state /*= nullptr*/)
        {
            AZ_Assert(
                transitionBarrier.StateAfter == D3D12_RESOURCE_STATE_RESOLVE_SOURCE || transitionBarrier.StateAfter == D3D12_RESOURCE_STATE_RESOLVE_DEST,
                "Invalid state for resolve barrier");
            return QueueTransitionInternal(m_resolveTransitionBarrierRequests, BarrierOp(transitionBarrier, state));
        }

        const D3D12_RESOURCE_TRANSITION_BARRIER& Scope::QueuePrologueTransition(
            const D3D12_RESOURCE_TRANSITION_BARRIER& barrier, const BarrierOp::CommandListState* state /*= nullptr*/)
        {
            return QueueTransitionInternal(m_prologueTransitionBarrierRequests, BarrierOp(barrier, state));
        }

        const D3D12_RESOURCE_TRANSITION_BARRIER& Scope::QueueEpilogueTransition(
            const D3D12_RESOURCE_TRANSITION_BARRIER& barrier, const BarrierOp::CommandListState* state /*= nullptr*/)
        {
            return QueueTransitionInternal(m_epilogueTransitionBarrierRequests, BarrierOp(barrier, state));
        }

        void Scope::QueuePreDiscardTransition(
            const D3D12_RESOURCE_TRANSITION_BARRIER& barrier, const BarrierOp::CommandListState* state /*= nullptr*/)
        {
            m_preDiscardTransitionBarrierRequests.emplace_back(barrier, state);
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

        const D3D12_RESOURCE_TRANSITION_BARRIER& Scope::QueueTransitionInternal(
            AZStd::vector<BarrierOp>& barriers, const BarrierOp& barrierToAdd)
        {
            auto findIt = AZStd::find_if(
                barriers.begin(),
                barriers.end(),
                [&](const BarrierOp& element)
                {
                    if (element.m_barrier.Transition.pResource != barrierToAdd.m_barrier.Transition.pResource)
                    {
                        return false;
                    }

                    return element.m_barrier.Transition.Subresource == barrierToAdd.m_barrier.Transition.Subresource ||
                        (element.m_barrier.Transition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES ||
                            barrierToAdd.m_barrier.Transition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
                });

            if (findIt == barriers.end())
            {
                barriers.push_back(barrierToAdd);
                return barriers.back().m_barrier.Transition;
            }
            else
            {
                BarrierOp& mergeBarrier = *findIt;
                mergeBarrier.m_barrier.Transition.StateAfter |= barrierToAdd.m_barrier.Transition.StateAfter;
                return mergeBarrier.m_barrier.Transition;
            }
        }
    
        bool Scope::IsResourceDiscarded(const RHI::ImageScopeAttachment& scopeAttachment) const
        {
            const ImageView* imageView = static_cast<const ImageView*>(scopeAttachment.GetImageView()->GetDeviceImageView(GetDeviceIndex()).get());
            return IsInDiscardResourceRequests(imageView->GetMemory());
        }

        bool Scope::IsResourceDiscarded(const RHI::BufferScopeAttachment& scopeAttachment) const
        {
            const BufferView* bufferView = static_cast<const BufferView*>(scopeAttachment.GetBufferView()->GetDeviceBufferView(GetDeviceIndex()).get());
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

        void Scope::CompileInternal()
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
                const ImageView* imageView = static_cast<const ImageView*>(scopeAttachment->GetImageView()->GetDeviceImageView(GetDeviceIndex()).get());
                const RHI::ImageScopeAttachmentDescriptor& bindingDescriptor = scopeAttachment->GetDescriptor();

                const bool isFullView           = imageView->IsFullView();
                const bool isClearAction        = bindingDescriptor.m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear;
                const bool isClearActionStencil = bindingDescriptor.m_loadStoreAction.m_loadActionStencil == RHI::AttachmentLoadAction::Clear;
                const bool isClear              = isClearAction || isClearActionStencil;
                bool isFullClear                = isClearAction && isFullView;
                RHI::ScopeAttachmentAccess access = scopeAttachment->GetAccess();

                CommandList::ImageClearRequest clearRequest;
                clearRequest.m_clearValue = bindingDescriptor.m_loadStoreAction.m_clearValue;
                clearRequest.m_imageView = imageView;

                AZStd::vector<CommandList::ImageClearRequest>* clearRequestQueue = nullptr;

                switch (scopeAttachment->GetUsage())
                {
                case RHI::ScopeAttachmentUsage::Shader:
                    if (RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Write))
                    { 
                        clearRequestQueue = &m_clearImageRequests;
                    }
                            
                    break;

                case RHI::ScopeAttachmentUsage::RenderTarget:
                    if (RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Write))
                    {
                        clearRequestQueue = &m_clearRenderTargetRequests;
                    }

                    // Accumulate color attachments for the render target bind command.
                    m_colorAttachments.push_back(imageView);
                    break;

                case RHI::ScopeAttachmentUsage::DepthStencil:
                    if (RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Write))
                    {
                        clearRequestQueue = &m_clearRenderTargetRequests;
                        clearRequest.m_clearFlags |= isClearAction ? D3D12_CLEAR_FLAG_DEPTH : clearRequest.m_clearFlags;
                        clearRequest.m_clearFlags |= isClearActionStencil ? D3D12_CLEAR_FLAG_STENCIL : clearRequest.m_clearFlags;
                        isFullClear &= isClearActionStencil;
                    }
                            
                    // Set the depth stencil attachment.
                    AZ_Error(
                        "Scope",
                        m_depthStencilAttachment == nullptr ||
                            !RHI::CheckBitsAll(m_depthStencilAttachment->GetDescriptor().m_aspectFlags, RHI::ImageAspectFlags::DepthStencil) ||
                            !RHI::CheckBitsAll(imageView->GetDescriptor().m_aspectFlags, RHI::ImageAspectFlags::DepthStencil),
                        "More than one depth stencil attachment was used on scope '%s'",
                        GetId().GetCStr());

                    // Handle the case with multiple ScopeAttachmentUsage::DepthStencil attachments.
                    // One for the Depth and another for the Stencil, with different access.
                    // Need to create a new ImageView that has both depth and stencil.
                    if (m_depthStencilAttachment)
                    {
                        RHI::ImageViewDescriptor descriptor = m_depthStencilAttachment->GetDescriptor();
                        descriptor.m_aspectFlags |= imageView->GetDescriptor().m_aspectFlags;
                        // Check if we need to create a new depth stencil image view or we can reuse the one saved in the scope
                        if (!m_fullDepthStencilView || &m_fullDepthStencilView->GetImage() != &m_depthStencilAttachment->GetImage() ||
                            m_fullDepthStencilView->GetDescriptor() != descriptor)
                        {
                            RHI::Ptr<ImageView> fullDepthStencil = ImageView::Create();
                            fullDepthStencil->Init(m_depthStencilAttachment->GetImage(), descriptor);
                            m_fullDepthStencilView = fullDepthStencil;
                        }
                        imageView = m_fullDepthStencilView.get();
                    }
                    m_depthStencilAttachment = imageView;
                    m_depthStencilAccess |= access;
                    break;

                case RHI::ScopeAttachmentUsage::ShadingRate:
                    {
                        m_shadingRateAttachment = imageView;
                    }
                    break;
                case RHI::ScopeAttachmentUsage::Uninitialized:
                    AZ_Assert(false, "ScopeAttachmentUsage is Uninitialized");
                    break;
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
                const BufferView* bufferView = static_cast<const BufferView*>(scopeAttachment->GetBufferView()->GetDeviceBufferView(GetDeviceIndex()).get());
                const RHI::BufferScopeAttachmentDescriptor& bindingDescriptor = scopeAttachment->GetDescriptor();

                const bool isClearAction = bindingDescriptor.m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear;
                
                bool isFullClear = false;

                const bool isShaderUsage = scopeAttachment->GetUsage() == RHI::ScopeAttachmentUsage::Shader;
                if (isClearAction && isShaderUsage)
                {
                    AZ_Assert(
                        RHI::CheckBitsAny(scopeAttachment->GetAccess(), RHI::ScopeAttachmentAccess::Write),
                        "We shouldnt be clearing without write access");
                    CommandList::BufferClearRequest request;
                    request.m_clearValue = bindingDescriptor.m_loadStoreAction.m_clearValue;
                    request.m_bufferView = bufferView;
                    m_clearBufferRequests.push_back(request);

                    isFullClear = bufferView->IsFullView();
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
            AZ_PROFILE_FUNCTION(RHI);

            commandList.GetValidator().BeginScope(*this);
            commandList.SetAftermathEventMarker(GetId().GetCStr());
            
            const bool isPrologue = commandListIndex == 0;
            if (isPrologue)
            {
                if (RHI::RHISystemInterface::Get()->GpuMarkersEnabled())
                {
                    PIXBeginEvent(commandList.GetCommandList(), PIX_MARKER_CMDLIST_COL, GetMarkerLabel().data());
                }

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
            if (m_colorAttachments.size() || m_depthStencilAttachment || m_shadingRateAttachment)
            {
                commandList.SetRenderTargets(
                    static_cast<uint32_t>(m_colorAttachments.size()),
                    m_colorAttachments.data(),
                    m_depthStencilAttachment,
                    m_depthStencilAccess,
                    m_shadingRateAttachment);
            }
        }

        void Scope::End(
            CommandList& commandList,
            uint32_t commandListIndex,
            uint32_t commandListCount) const
        {
            AZ_PROFILE_FUNCTION(RHI);

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
                            auto srcImageView = static_cast<const ImageView*>(imageAttachment->GetImageView()->GetDeviceImageView(GetDeviceIndex()).get());
                            auto dstImageView = static_cast<const ImageView*>(resolveAttachment->GetImageView()->GetDeviceImageView(GetDeviceIndex()).get());
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

                if (RHI::RHISystemInterface::Get()->GpuMarkersEnabled())
                {
                    PIXEndEvent(commandList.GetCommandList());
                }
            }

            commandList.GetValidator().EndScope();
        }
    }
}
