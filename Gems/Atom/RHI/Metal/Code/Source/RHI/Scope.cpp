/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/ResolveScopeAttachment.h>
#include <AzCore/Debug/EventTrace.h>
#include <RHI/CommandList.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/ImageView.h>
#include <RHI/ResourcePoolResolver.h>
#include <RHI/Scope.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>


namespace AZ
{
    namespace Metal
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
            m_waitFencesByQueue = { {0} };
            m_signalFenceValue = 0;
            m_renderPassDescriptor = nil;
            
            for (size_t i = 0; i < static_cast<uint32_t>(ResourceFenceAction::Count); ++i)
            {
                m_resourceFences[i].clear();
            }
            m_queryPoolAttachments.clear();
            m_residentHeaps.clear();

        }

        void Scope::ApplyMSAACustomPositions(const ImageView* imageView)
        {
            const Image& image = imageView->GetImage();
            const RHI::ImageDescriptor& imageDescriptor = image.GetDescriptor();
            
            m_scopeMultisampleState = imageDescriptor.m_multisampleState;
            if(m_scopeMultisampleState.m_customPositionsCount > 0)
            {
                AZStd::vector<MTLSamplePosition> mtlCustomSampleLocations;
                AZStd::transform( m_scopeMultisampleState.m_customPositions.begin(),
                                 m_scopeMultisampleState.m_customPositions.begin() + m_scopeMultisampleState.m_customPositionsCount,
                                 AZStd::back_inserter(mtlCustomSampleLocations), [&](const auto& item)
                {
                    return ConvertSampleLocation(item);
                });
                
                MTLSamplePosition samplePositions[m_scopeMultisampleState.m_customPositionsCount];
                
                for(int i = 0 ; i < m_scopeMultisampleState.m_customPositionsCount; i++)
                {
                    samplePositions[i] = mtlCustomSampleLocations[i];
                }
                [m_renderPassDescriptor setSamplePositions:samplePositions count:m_scopeMultisampleState.m_customPositionsCount];
            }
        }
    
        void Scope::CompileInternal(RHI::Device& device)
        {
            Device& mtlDevice = static_cast<Device&>(device);
            for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
            {
                static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Compile();
            }
            
            if(GetImageAttachments().size() > 0)
            {
                AZ_Assert(m_renderPassDescriptor == nil, "m_renderPassDescriptor should be null");
                m_renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
            }

            if(GetEstimatedItemCount())
            {
                m_residentHeaps.insert(mtlDevice.GetNullDescriptorManager().GetNullDescriptorHeap());
            }
            
            int colorAttachmentIndex = 0;
            AZStd::unordered_map<RHI::AttachmentId, ResolveAttachmentData> attachmentsIndex;
            
            for (const RHI::ImageScopeAttachment* scopeAttachment : GetImageAttachments())
            {
                m_isWritingToSwapChainScope = scopeAttachment->IsSwapChainAttachment() && scopeAttachment->HasUsage(RHI::ScopeAttachmentUsage::RenderTarget);
                if(m_isWritingToSwapChainScope)
                {
                    //Check if the scope attachment for the next scope is going to capture the frame.
                    //We can use this information to cache the swapchain texture for reading purposes.
                    const RHI::ScopeAttachment* frameCaptureScopeAttachment = scopeAttachment->GetNext();
                    if(frameCaptureScopeAttachment)
                    {
                        m_isSwapChainAndFrameCaptureEnabled = frameCaptureScopeAttachment->HasAccessAndUsage(RHI::ScopeAttachmentUsage::Copy, RHI::ScopeAttachmentAccess::Read);
                    }
                    
                    //Cache this as we will use this to request the drawable from the driver in the Execute phase (i.e Scope::Begin)
                    m_swapChainAttachment = scopeAttachment;
                }
                
                const ImageView* imageView = static_cast<const ImageView*>(scopeAttachment->GetImageView());
                const RHI::ImageScopeAttachmentDescriptor& bindingDescriptor = scopeAttachment->GetDescriptor();
                id<MTLTexture> imageViewMtlTexture = imageView->GetMemoryView().GetGpuAddress<id<MTLTexture>>();
                
                const bool isClearAction        = bindingDescriptor.m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear;
                const bool isClearActionStencil = bindingDescriptor.m_loadStoreAction.m_loadActionStencil == RHI::AttachmentLoadAction::Clear;
                
                const bool isLoadAction         = bindingDescriptor.m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Load;
                
                const bool isStoreAction         = bindingDescriptor.m_loadStoreAction.m_storeAction == RHI::AttachmentStoreAction::Store;
                const bool isStoreActionStencil  = bindingDescriptor.m_loadStoreAction.m_storeActionStencil == RHI::AttachmentStoreAction::Store;
                
                
                MTLLoadAction mtlLoadAction = MTLLoadActionDontCare;
                MTLStoreAction mtlStoreAction = MTLStoreActionDontCare;
                MTLLoadAction mtlLoadActionStencil = MTLLoadActionDontCare;
                MTLStoreAction mtlStoreActionStencil = MTLStoreActionDontCare;
                
                if(isClearAction)
                {
                    mtlLoadAction = MTLLoadActionClear;
                }
                else if(isLoadAction)
                {
                    mtlLoadAction = MTLLoadActionLoad;
                }
                
                if(isStoreAction)
                {
                    mtlStoreAction = MTLStoreActionStore;
                }
                
                if(isClearActionStencil)
                {
                    mtlLoadActionStencil = MTLLoadActionClear;
                }
                else if(isLoadAction)
                {
                    mtlLoadActionStencil = MTLLoadActionLoad;
                }
                
                if(isStoreActionStencil)
                {
                    mtlStoreActionStencil = MTLStoreActionStore;
                }
                const AZStd::vector<RHI::ScopeAttachmentUsageAndAccess>& usagesAndAccesses = scopeAttachment->GetUsageAndAccess();
                for (const RHI::ScopeAttachmentUsageAndAccess& usageAndAccess : usagesAndAccesses)
                {
                    switch (usageAndAccess.m_usage)
                    {
                        case RHI::ScopeAttachmentUsage::Shader:
                        {
                            break;
                        }
                        case RHI::ScopeAttachmentUsage::RenderTarget:
                        {
                            id<MTLTexture> renderTargetTexture = imageViewMtlTexture;
                            m_renderPassDescriptor.colorAttachments[colorAttachmentIndex].texture = renderTargetTexture;
                            
                            if(!m_isWritingToSwapChainScope)
                            {
                                //Cubemap/cubemaparray and 3d textures have restrictions placed on them by the
                                //drivers when creating a new texture view. Hence we cant get a view with subresource range
                                //of the original texture. As a result in order to write into specific slice or depth plane
                                //we specify it here. It also means that we cant write into these texturee types via a compute shader
                                const RHI::ImageViewDescriptor& imgViewDescriptor = imageView->GetDescriptor();
                                if(renderTargetTexture.textureType == MTLTextureTypeCube || renderTargetTexture.textureType == MTLTextureTypeCubeArray)
                                {
                                    m_renderPassDescriptor.colorAttachments[colorAttachmentIndex].slice = imgViewDescriptor.m_arraySliceMin;
                                }
                                else if(renderTargetTexture.textureType == MTLTextureType3D)
                                {
                                    m_renderPassDescriptor.colorAttachments[colorAttachmentIndex].depthPlane = imgViewDescriptor.m_depthSliceMin;
                                }
                            }
                            
                            MTLRenderPassColorAttachmentDescriptor* colorAttachment = m_renderPassDescriptor.colorAttachments[colorAttachmentIndex];
                            colorAttachment.loadAction = mtlLoadAction;
                            colorAttachment.storeAction = mtlStoreAction;

                            RHI::ClearValue clearVal = bindingDescriptor.m_loadStoreAction.m_clearValue;
                            if (mtlLoadAction == MTLLoadActionClear)
                            {
                                m_isClearNeeded = true;
                                if(clearVal.m_type == RHI::ClearValueType::Vector4Float)
                                {
                                    colorAttachment.clearColor = MTLClearColorMake(clearVal.m_vector4Float[0], clearVal.m_vector4Float[1], clearVal.m_vector4Float[2], clearVal.m_vector4Float[3]);
                                }
                                else if(clearVal.m_type == RHI::ClearValueType::Vector4Uint)
                                {
                                    colorAttachment.clearColor = MTLClearColorMake(clearVal.m_vector4Uint[0], clearVal.m_vector4Uint[1], clearVal.m_vector4Uint[2], clearVal.m_vector4Uint[3]);
                                }
                            }
                            
                            ApplyMSAACustomPositions(imageView);
                            attachmentsIndex[bindingDescriptor.m_attachmentId] = ResolveAttachmentData{colorAttachmentIndex, m_renderPassDescriptor, RHI::ScopeAttachmentUsage::RenderTarget, isStoreAction};
                            colorAttachmentIndex++;
                            break;
                        }
                        case RHI::ScopeAttachmentUsage::DepthStencil:
                        {
                            m_renderPassDescriptor.depthAttachment.texture = imageViewMtlTexture;
                            if(IsDepthStencilMerged(imageView->GetSpecificFormat()))
                            {
                                m_renderPassDescriptor.stencilAttachment.texture = imageViewMtlTexture;
                            }
                            
                            MTLRenderPassDepthAttachmentDescriptor* depthAttachment = m_renderPassDescriptor.depthAttachment;
                            MTLRenderPassStencilAttachmentDescriptor* stencilAttachment = m_renderPassDescriptor.stencilAttachment;
                            depthAttachment.loadAction = mtlLoadAction;
                            depthAttachment.storeAction = mtlStoreAction;
                            stencilAttachment.loadAction = mtlLoadActionStencil;
                            stencilAttachment.storeAction = mtlStoreActionStencil;
                            if (bindingDescriptor.m_loadStoreAction.m_clearValue.m_type == RHI::ClearValueType::DepthStencil)
                            {
                                m_isClearNeeded = true;
                                depthAttachment.clearDepth = bindingDescriptor.m_loadStoreAction.m_clearValue.m_depthStencil.m_depth;
                                stencilAttachment.clearStencil = bindingDescriptor.m_loadStoreAction.m_clearValue.m_depthStencil.m_stencil;
                            }
                            ApplyMSAACustomPositions(imageView);
                            attachmentsIndex[bindingDescriptor.m_attachmentId] = ResolveAttachmentData{-1, m_renderPassDescriptor, RHI::ScopeAttachmentUsage::DepthStencil, isStoreAction};
                            
                            break;
                        }
                        case RHI::ScopeAttachmentUsage::Resolve:
                        {
                            auto resolveScopeAttachment = static_cast<const RHI::ResolveScopeAttachment*>(scopeAttachment);
                            auto resolveAttachmentId = resolveScopeAttachment->GetDescriptor().m_resolveAttachmentId;
                            AZ_Assert(attachmentsIndex.find(resolveAttachmentId) != attachmentsIndex.end(), "Failed to find resolvable attachment %s", resolveAttachmentId.GetCStr());
                            ResolveAttachmentData resolveAttachmentData = attachmentsIndex[resolveAttachmentId];
                            MTLRenderPassDescriptor* renderPassDesc = resolveAttachmentData.m_renderPassDesc;

                            id<MTLTexture> renderTargetTexture = imageViewMtlTexture;
                            
                            MTLStoreAction resolveStoreAction = resolveAttachmentData.m_isStoreAction ? MTLStoreActionStoreAndMultisampleResolve : MTLStoreActionMultisampleResolve;
                            
                            if(resolveAttachmentData.m_attachmentUsage == RHI::ScopeAttachmentUsage::RenderTarget)
                            {
                                renderPassDesc.colorAttachments[resolveAttachmentData.m_colorAttachmentIndex].resolveTexture = renderTargetTexture;
                                MTLRenderPassColorAttachmentDescriptor* colorAttachment = renderPassDesc.colorAttachments[resolveAttachmentData.m_colorAttachmentIndex];
                                colorAttachment.storeAction = resolveStoreAction;
                            }
                            else if (resolveAttachmentData.m_attachmentUsage == RHI::ScopeAttachmentUsage::DepthStencil)
                            {
                                renderPassDesc.depthAttachment.resolveTexture = renderTargetTexture;
                                MTLRenderPassDepthAttachmentDescriptor* depthAttachment = renderPassDesc.depthAttachment;
                                depthAttachment.storeAction = resolveStoreAction;
                                //Metal drivers support min/max depth resolve filters but there is no way to set them at a higher level yet.
                            }
                            m_isResolveNeeded = true;
                            break;
                        }
                    }
                }
            }
        }

        void Scope::Begin(
            CommandList& commandList,
            AZ::u32 commandListIndex,
            AZ::u32 commandListCount) const
        {
            AZ_TRACE_METHOD();

            if(m_isWritingToSwapChainScope)
            {
                //Metal requires you to request for swapchain drawable as late as possible in the frame. Hence we call for the drawable
                //here and attach it directly to the colorAttachment. The assumption here is that this scope should be the
                //CopyToSwapChain scope. With this way if the internal resolution differs from the window resolution the compositer
                //within the metal driver will perform the appropriate upscale/downscale at the end.
                const RHI::SwapChain* swapChain = (azrtti_cast<const RHI::SwapChainFrameAttachment*>(&m_swapChainAttachment->GetFrameAttachment()))->GetSwapChain();
                SwapChain* metalSwapChain = static_cast<SwapChain*>(const_cast<RHI::SwapChain*>(swapChain));
                m_renderPassDescriptor.colorAttachments[0].texture = metalSwapChain->RequestDrawable(m_isSwapChainAndFrameCaptureEnabled);
            }
            
            const bool isPrologue = commandListIndex == 0;
            commandList.SetName(GetId());
            commandList.SetRenderPassInfo(m_renderPassDescriptor, m_scopeMultisampleState, m_residentHeaps);
            
            if (isPrologue)
            {
                for (const auto& fence : m_resourceFences[static_cast<int>(ResourceFenceAction::Wait)])
                {
                    commandList.WaitOnResourceFence(fence);
                }
                
                for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
                {
                    static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Resolve(commandList);
                }
            }
              
            for (const auto& queryPoolAttachment : m_queryPoolAttachments)
            {
                if (!RHI::CheckBitsAll(queryPoolAttachment.m_access, RHI::ScopeAttachmentAccess::Write))
                {
                    continue;
                }

                //Attach occlusion testing related visibility buffer
                if(queryPoolAttachment.m_pool->GetDescriptor().m_type == RHI::QueryType::Occlusion)
                {
                    commandList.AttachVisibilityBuffer(queryPoolAttachment.m_pool->GetVisibilityBuffer());
                }
            }
            
            //If a scope has resolve texture or if it is using a clear load action we know the encoder type is Render
            //and hence we create the encoder here even though there may not be any draw commands by the scope. This will
            //allow us to use the driver to clear a render target or do a msaa resolve within a scope with no draw work.
            if(m_isResolveNeeded || m_isClearNeeded)
            {
                commandList.CreateEncoder(CommandEncoderType::Render);
            }
        }

        void Scope::End(
            CommandList& commandList,
            AZ::u32 commandListIndex,
            AZ::u32 commandListCount) const
        {
            AZ_TRACE_METHOD();
            const bool isEpilogue = (commandListIndex + 1) == commandListCount;
            
            commandList.FlushEncoder();
            
            if (isEpilogue)
            {
                for (const auto& fence : m_resourceFences[static_cast<int>(ResourceFenceAction::Signal)])
                {
                    commandList.SignalResourceFence(fence);
                }
            }
        }
        
        MTLRenderPassDescriptor* Scope::GetRenderPassDescriptor() const
        {
            return  m_renderPassDescriptor;
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
            bool hasWaitFences = false;
            for (uint32_t i = 0; i < RHI::HardwareQueueClassCount; ++i)
            {
                hasWaitFences = hasWaitFences || (m_waitFencesByQueue[i] > 0);
            }
            return hasWaitFences;
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
    
        void Scope::QueueResourceFence(ResourceFenceAction fenceAction, Fence& fence)
        {
            m_resourceFences[static_cast<uint32_t>(fenceAction)].push_back(fence);
        }
    
        void Scope::AddQueryPoolUse(RHI::Ptr<RHI::QueryPool> queryPool, const RHI::Interval& interval, RHI::ScopeAttachmentAccess access)
        {
            m_queryPoolAttachments.push_back({ AZStd::static_pointer_cast<QueryPool>(queryPool), interval, access });
        }
    }
}
