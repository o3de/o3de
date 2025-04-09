/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/ResolveScopeAttachment.h>
#include <Atom/RHI/SwapChain.h>
#include <RHI/CommandList.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/ImageView.h>
#include <RHI/ResourcePoolResolver.h>
#include <RHI/Scope.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<Scope> Scope::Create()
        {
            return aznew Scope();
        }
    
        void Scope::InitInternal()
        {
            m_markerName = Name(GetMarkerLabel());
        }
    
        void Scope::DeactivateInternal()
        {
            for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
            {
                static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Deactivate();
            }
            m_waitFencesByQueue = { {0} };
            m_signalFenceValue = 0;
            m_renderPassContext = {};
            
            for (size_t i = 0; i < static_cast<uint32_t>(ResourceFenceAction::Count); ++i)
            {
                m_resourceFences[i].clear();
            }
            m_queryPoolAttachments.clear();
            m_residentHeaps.clear();

        }        
    
        void Scope::CompileInternal()
        {
            RHI::Device& device = GetDevice();
            Device& mtlDevice = static_cast<Device&>(device);
            for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
            {
                static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Compile();
            }
            
            if(GetEstimatedItemCount())
            {
                m_residentHeaps.insert(mtlDevice.GetNullDescriptorManager().GetNullDescriptorHeap());
            }
        }
    
        template<class T>
        bool NeedsEncoder(T* attachmentDescriptor)
        {
            //If a scope has resolve texture or if it is using a clear load action we know the encoder type is Render
            //and hence we create the encoder here even though there may not be any draw commands by the scope. This will
            //allow us to use the driver to clear a render target or do a msaa resolve within a scope with no draw work.
            return attachmentDescriptor != nil && (attachmentDescriptor.resolveTexture != nil || attachmentDescriptor.loadAction == MTLLoadActionClear);
        }
    
        bool Scope::IsFirstUsage(const RHI::ScopeAttachment* scopeAttachment) const
        {
            while (scopeAttachment != nullptr && scopeAttachment->GetScope().GetId() == GetId())
            {
                scopeAttachment = scopeAttachment->GetPrevious();
            }

            if (!scopeAttachment || scopeAttachment->GetScope().GetFrameGraphGroupId() != GetFrameGraphGroupId())
            {
                return true;
            }
            return false;
        }

        void Scope::Begin(
            CommandList& commandList,
            AZ::u32 commandListIndex,
            AZ::u32 commandListCount) const
        {
            AZ_PROFILE_FUNCTION(RHI);            
            
            const bool isPrologue = commandListIndex == 0;
            commandList.SetName(m_markerName);
            commandList.SetRenderPassInfo(m_renderPassContext.m_renderPassDescriptor, m_renderPassContext.m_scopeMultisampleState, m_residentHeaps);
            
            if (isPrologue)
            {
                for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
                {
                    static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Resolve(commandList);
                }
                
                if (RHI::CheckBitsAll(GetActivationFlags(), RHI::Scope::ActivationFlags::Subpass))
                {
                    // When merging passes as subpasses, we may need to clear at the beginning of the subpass.
                    // We may not be able to use the clear load action, because there's another subpass before that
                    // is using the attachment. Metal doesn't provide a way of clearing a render attachment mid pass
                    // so we use a full screen triangle to do the clearing.
                    AZStd::vector<ClearAttachments::ClearData> clearAttachmentData;
                    for (auto imageAttachment : GetImageAttachments())
                    {
                        const auto& loadStoreAction = imageAttachment->GetScopeAttachmentDescriptor().m_loadStoreAction;
                        bool clear = loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear;
                        bool clearStencil = loadStoreAction.m_loadActionStencil == RHI::AttachmentLoadAction::Clear;
                        if (clear || clearStencil)
                        {
                            // Check if there's another scope before this that is using the attachment
                            if (!IsFirstUsage(imageAttachment))
                            {
                                // We can't use load clear action, so we have to do manual clearing.
                                if (imageAttachment->GetUsage() == RHI::ScopeAttachmentUsage::RenderTarget)
                                {
                                    const ImageView* imageView = static_cast<const ImageView*>(imageAttachment->GetImageView()->GetDeviceImageView(GetDeviceIndex()).get());
                                    id<MTLTexture> imageViewMtlTexture = imageView->GetMemoryView().GetGpuAddress<id<MTLTexture>>();
                                    
                                    ClearAttachments::ClearData& data = clearAttachmentData.emplace_back();
                                    data.m_clearValue = loadStoreAction.m_clearValue;
                                    data.m_imageAspects = RHI::ImageAspectFlags::Color;
                                    for(uint32_t i = 0; i < RHI::Limits::Pipeline::AttachmentColorCountMax; ++i)
                                    {
                                        if (m_renderPassContext.m_renderPassDescriptor.colorAttachments[i].texture == imageViewMtlTexture)
                                        {
                                            data.m_attachmentIndex = i;
                                            break;
                                        }
                                    }
                                    AZ_Assert(data.m_attachmentIndex < RHI::Limits::Pipeline::AttachmentColorCountMax, "Attachment index not found for image attachent %s", imageAttachment->GetDescriptor().m_attachmentId.GetCStr());
                                }
                                else if (imageAttachment->GetUsage() == RHI::ScopeAttachmentUsage::DepthStencil)
                                {
                                    ClearAttachments::ClearData& data = clearAttachmentData.emplace_back();
                                    data.m_clearValue = loadStoreAction.m_clearValue;
                                    if (clear)
                                    {
                                        data.m_imageAspects |= RHI::ImageAspectFlags::Depth;
                                    }
                                    if (clearStencil)
                                    {
                                        data.m_imageAspects |= RHI::ImageAspectFlags::Stencil;
                                    }
                                }
                            }
                        }
                    }
                    
                    if (!clearAttachmentData.empty())
                    {
                        static_cast<Device&>(GetDevice()).ClearRenderAttachments(commandList, m_renderPassContext.m_renderPassDescriptor, clearAttachmentData);
                    }
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
                    commandList.AttachVisibilityBuffer(AZStd::static_pointer_cast<QueryPool>(queryPoolAttachment.m_pool->GetDeviceQueryPool(GetDeviceIndex()))->GetVisibilityBuffer());
                }
            }
            
            bool needsEncoder = false;
            if (m_renderPassContext.m_renderPassDescriptor != nil)
            {
                needsEncoder |= NeedsEncoder(m_renderPassContext.m_renderPassDescriptor.depthAttachment);
                needsEncoder |= NeedsEncoder(m_renderPassContext.m_renderPassDescriptor.stencilAttachment);

                for(uint32_t i = 0; i < RHI::Limits::Pipeline::AttachmentColorCountMax && needsEncoder; ++i)
                {
                    MTLRenderPassColorAttachmentDescriptor* colorDescriptor = [m_renderPassContext.m_renderPassDescriptor.colorAttachments objectAtIndexedSubscript:i];
                    needsEncoder |= NeedsEncoder(colorDescriptor);
                }
            }
            
            if(needsEncoder)
            {
                commandList.CreateEncoder(CommandEncoderType::Render);
            }
        }

        void Scope::End(CommandList& commandList) const
        {
            AZ_PROFILE_FUNCTION(RHI);
            commandList.FlushEncoder();
        }
        
        void Scope::SignalAllResourceFences(CommandList& commandList) const
        {
            for (const auto& fence : m_resourceFences[static_cast<int>(ResourceFenceAction::Signal)])
            {
                commandList.SignalResourceFence(fence);
            }
        }
    
        void Scope::SignalAllResourceFences(id <MTLCommandBuffer> mtlCommandBuffer) const
        {
            for (const auto& fence : m_resourceFences[static_cast<int>(ResourceFenceAction::Signal)])
            {
                fence.SignalFromGpu(mtlCommandBuffer);
            }
        }
    
        void Scope::WaitOnAllResourceFences(CommandList& commandList) const
        {
            for (const auto& fence : m_resourceFences[static_cast<int>(ResourceFenceAction::Wait)])
            {
                commandList.WaitOnResourceFence(fence);
            }
        }
    
        void Scope::WaitOnAllResourceFences(id <MTLCommandBuffer> mtlCommandBuffer) const
        {
            for (const auto& fence : m_resourceFences[static_cast<int>(ResourceFenceAction::Wait)])
            {
                fence.WaitOnGpu(mtlCommandBuffer);
            }
        }
    
        MTLRenderPassDescriptor* Scope::GetRenderPassDescriptor() const
        {
            return  m_renderPassContext.m_renderPassDescriptor;
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
            m_queryPoolAttachments.push_back({ queryPool, interval, access });
        }
    
        void Scope::SetRenderPassInfo(const RenderPassContext& renderPassContext)
        {
            m_renderPassContext = renderPassContext;
        }
    }
}
