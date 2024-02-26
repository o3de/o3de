/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/FrameGraphExecuteGroupMerged.h>
#include <RHI/Scope.h>

namespace AZ
{
    namespace Metal
    {
        void FrameGraphExecuteGroupMerged::Init(
            Device& device, AZStd::vector<const Scope*>&& scopes, uint32_t groupIndex)
        {
            SetDevice(device);
            
            AZStd::string labelNameStr = AZStd::string::format("MergedGroupCB_%i", groupIndex);
            m_cbLabel = [NSString stringWithCString:labelNameStr.c_str() encoding:NSUTF8StringEncoding];
            
            m_scopes = AZStd::move(scopes);
            m_workRequest.m_commandLists.resize(1);
            m_hardwareQueueClass = m_scopes.back()->GetHardwareQueueClass();
            
            //We split scopes based on fence boundary and hence we only neeed to wait for any relevant
            //fences on the top scope and signal fence for the last scope.
            m_workRequest.m_waitFenceValues = m_scopes.front()->GetWaitFences();
            m_workRequest.m_signalFenceValue = m_scopes.back()->GetSignalFenceValue();
            
            AZStd::vector<InitMergedRequest::ScopeEntry> scopeEntries;
            scopeEntries.reserve(m_scopes.size());
            for (const Scope* scope : m_scopes)
            {
                scopeEntries.push_back({ scope->GetId(), scope->GetEstimatedItemCount() });
                
                m_workRequest.m_swapChainsToPresent.reserve(m_workRequest.m_swapChainsToPresent.size() + scope->GetSwapChainsToPresent().size());
                for (RHI::SingleDeviceSwapChain* swapChain : scope->GetSwapChainsToPresent())
                {
                    m_workRequest.m_swapChainsToPresent.push_back(static_cast<SwapChain*>(swapChain));
                }

                auto& fencesToSignal = m_workRequest.m_scopeFencesToSignal;
                fencesToSignal.reserve(fencesToSignal.size() + scope->GetFencesToSignal().size());
                for (const RHI::Ptr<RHI::SingleDeviceFence>& fence : scope->GetFencesToSignal())
                {
                    fencesToSignal.push_back(&static_cast<FenceImpl&>(*fence).Get());
                }
            }

            InitMergedRequest request;
            request.m_scopeEntries = scopeEntries.data();
            request.m_scopeCount = static_cast<uint32_t>(scopeEntries.size());
            
            m_commandBuffer.Init(device.GetCommandQueueContext().GetCommandQueue(m_hardwareQueueClass).GetPlatformQueue());
            
            Base::Init(request);
        }

        void FrameGraphExecuteGroupMerged::BeginInternal()
        {
            //Create the autorelease pool to ensure command buffer is not leaked
            CreateAutoreleasePool();
            
            id <MTLCommandBuffer> mtlCommandBuffer = m_commandBuffer.AcquireMTLCommandBuffer();
            mtlCommandBuffer.label = m_cbLabel;
            m_workRequest.m_commandBuffer = &m_commandBuffer;
            
            //Encode any wait events at the start of the group. This should grab the wait fence across all queues from the top
            //scope and encode it here.
            EncodeWaitEvents();

            CommandList* commandList = AcquireCommandList();
            commandList->Open(mtlCommandBuffer);
            m_workRequest.m_commandLists.back() = commandList;
        }

        void FrameGraphExecuteGroupMerged::EndInternal()
        {
            AZ_Assert(!m_workRequest.m_commandLists.empty(), "commandlist is empty.");
            static_cast<CommandList*>(m_workRequest.m_commandLists.back())->Close();
            
            FlushAutoreleasePool();
        }

        void FrameGraphExecuteGroupMerged::BeginContextInternal(
            RHI::FrameGraphExecuteContext& context,
            AZ::u32 contextIndex)
        {
            AZ_Assert((m_lastCompletedScope + 1) == contextIndex, "Contexts must be recorded in order!");

            const Scope* scope = m_scopes[contextIndex];

            AZ_Assert(!m_workRequest.m_commandLists.empty(), "commandlist is empty.");
            CommandList* commandList = static_cast<CommandList*>(m_workRequest.m_commandLists.back());
            context.SetCommandList(*commandList);
            scope->Begin(*commandList, context.GetCommandListIndex(), context.GetCommandListCount());
        }

        void FrameGraphExecuteGroupMerged::EndContextInternal(
            RHI::FrameGraphExecuteContext& context,
            AZ::u32 contextIndex)
        {
            m_lastCompletedScope = contextIndex;
            
            const Scope* scope = m_scopes[contextIndex];
            CommandList* commandList = static_cast<CommandList*>(context.GetCommandList());
            scope->End(*commandList, context.GetCommandListIndex(), context.GetCommandListCount());
        }


    }
}
