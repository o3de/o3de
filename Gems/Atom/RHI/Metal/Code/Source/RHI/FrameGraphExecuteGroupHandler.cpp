/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/FrameGraphExecuteGroup.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>
#include <Atom/RHI/SwapChain.h>
#include <RHI/FrameGraphExecuteGroup.h>
#include <RHI/FrameGraphExecuteGroupHandler.h>
#include <RHI/Device.h>
#include <RHI/SwapChain.h>

namespace AZ
{
    namespace Metal
    {
        RHI::ResultCode FrameGraphExecuteGroupHandler::Init(
            Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups)
        {
            m_device = &device;
            m_executeGroups = executeGroups;
            m_hardwareQueueClass = static_cast<FrameGraphExecuteGroup*>(executeGroups.back())->GetHardwareQueueClass();
            
            m_commandBuffer.Init(device.GetCommandQueueContext().GetCommandQueue(m_hardwareQueueClass).GetPlatformQueue());
            m_commandBuffer.AcquireMTLCommandBuffer();
            m_workRequest.m_commandBuffer = &m_commandBuffer;
            for(auto* rhiGroup : m_executeGroups)
            {
                FrameGraphExecuteGroup* group = static_cast<FrameGraphExecuteGroup*>(rhiGroup);
                group->SetCommandBuffer(&m_commandBuffer);
                group->SetHandler(this);
            }
            return InitInternal(device, executeGroups);
        }
    
        void FrameGraphExecuteGroupHandler::Shutdown()
        {
            m_device = nullptr;
            m_executeGroups.clear();
            m_isExecuted = false;
        }

        void FrameGraphExecuteGroupHandler::End()
        {
            EndInternal();
            CommandQueue* cmdQueue = &m_device->GetCommandQueueContext().GetCommandQueue(m_hardwareQueueClass);
            cmdQueue->ExecuteWork(AZStd::move(m_workRequest));
#if defined(AZ_FORCE_CPU_GPU_INSYNC)
            //Cache the name of the scope we just queued and wait for it to finish on the cpu
            m_device->SetLastExecutingScope(m_workRequest.m_commandList->GetName().GetStringView());
            cmdQueue->FlushCommands();
            cmdQueue->WaitForIdle();
#endif
            m_isExecuted = true;
        }

        bool FrameGraphExecuteGroupHandler::IsComplete() const
        {
            for (const auto& group : m_executeGroups)
            {
                if (!group->IsComplete())
                {
                    return false;
                }
            }

            return true;
        }

        bool FrameGraphExecuteGroupHandler::IsExecuted() const
        {
            return m_isExecuted;
        }

        template<class T>
        void InsertWorkRequestElements(T& destination, const T& source)
        {
            destination.insert(destination.end(), source.begin(), source.end());
        }

        void FrameGraphExecuteGroupHandler::AddWorkRequest(const ExecuteWorkRequest& workRequest)
        {
            InsertWorkRequestElements(m_workRequest.m_swapChainsToPresent, workRequest.m_swapChainsToPresent);
            InsertWorkRequestElements(m_workRequest.m_commandLists, workRequest.m_commandLists);
            InsertWorkRequestElements(m_workRequest.m_scopeFencesToSignal, workRequest.m_scopeFencesToSignal);
            m_workRequest.m_signalFenceValue = AZStd::max(m_workRequest.m_signalFenceValue, workRequest.m_signalFenceValue);
        }
    
        void FrameGraphExecuteGroupHandler::UpdateSwapChain(RenderPassContext &context)
        {
            // Check if the renderpass is using the swapchain texture
            if(context.m_swapChainAttachment)
            {
                // Metal requires you to request for swapchain drawable as late as possible in the frame. Hence we call for the drawable
                // here and attach it directly to the colorAttachment.
                uint32_t deviceIndex = static_cast<FrameGraphExecuteGroup*>(m_executeGroups.front())->GetScopes().front()->GetDeviceIndex();
                const RHI::DeviceSwapChain* swapChain = context.m_swapChainAttachment->GetSwapChain()->GetDeviceSwapChain(deviceIndex).get();
                SwapChain* metalSwapChain = static_cast<SwapChain*>(const_cast<RHI::DeviceSwapChain*>(swapChain));
                bool needsImageView = false;
                for(auto* scopeAttachment = context.m_swapChainAttachment->GetFirstScopeAttachment(deviceIndex);
                    scopeAttachment != nullptr;
                    scopeAttachment = scopeAttachment->GetNext())
                {
                    if (scopeAttachment->GetUsage() == RHI::ScopeAttachmentUsage::Shader)
                    {
                        needsImageView = true;
                        break;
                    }
                }
                // This call may block if the presentation system doesn't have any drawables available.
                id<MTLTexture> drawableTexture = metalSwapChain->RequestDrawable(needsImageView);
                context.m_renderPassDescriptor.colorAttachments[context.m_swapChainAttachmentIndex].texture = drawableTexture;
             }
        }
    
        void FrameGraphExecuteGroupHandler::BeginGroup(const FrameGraphExecuteGroup* group)
        {
            if(!m_hasBegun.exchange(true))
            {
                BeginInternal();
            }
            BeginGroupInternal(group);
        }
    
        void FrameGraphExecuteGroupHandler::EndGroup(const FrameGraphExecuteGroup* group)
        {
            EndGroupInternal(group);
        }
    }
}
