/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/FrameGraphExecuteGroup.h>
#include <RHI/Scope.h>

namespace AZ
{
    namespace Metal
    {
        void FrameGraphExecuteGroup::Init(
            Device& device,
            const Scope& scope,
            AZ::u32 commandListCount,
            RHI::JobPolicy globalJobPolicy,
            uint32_t groupIndex)
        {
            SetDevice(device);
            m_scope = &scope;
            
            AZStd::string labelNameStr = AZStd::string::format("GroupCB_%i", groupIndex);
            m_cbLabel = [NSString stringWithCString:labelNameStr.c_str() encoding:NSUTF8StringEncoding];

            m_hardwareQueueClass = scope.GetHardwareQueueClass();
            m_workRequest.m_waitFenceValues = scope.GetWaitFences();
            m_workRequest.m_signalFenceValue = scope.GetSignalFenceValue();            
            m_workRequest.m_commandLists.resize(commandListCount);
            m_workRequest.m_swapChainsToPresent.reserve(scope.GetSwapChainsToPresent().size());
            for (RHI::SingleDeviceSwapChain* swapChain : scope.GetSwapChainsToPresent())
            {
                m_workRequest.m_swapChainsToPresent.push_back(static_cast<SwapChain*>(swapChain));
            }
            
            auto& fencesToSignal = m_workRequest.m_scopeFencesToSignal;
            fencesToSignal.reserve(scope.GetFencesToSignal().size());
            for (const RHI::Ptr<RHI::SingleDeviceFence>& fence : scope.GetFencesToSignal())
            {
                fencesToSignal.push_back(&static_cast<FenceImpl&>(*fence).Get());
            }
            
            InitRequest request;
            request.m_scopeId = scope.GetId();
            request.m_submitCount = scope.GetEstimatedItemCount();
            request.m_commandLists = reinterpret_cast<RHI::CommandList* const*>(m_workRequest.m_commandLists.data());
            request.m_commandListCount = commandListCount;
            request.m_jobPolicy = globalJobPolicy;
            
            m_commandBuffer.Init(device.GetCommandQueueContext().GetCommandQueue(m_hardwareQueueClass).GetPlatformQueue());
            
            Base::Init(request);
        }
        
        FrameGraphExecuteGroup::~FrameGraphExecuteGroup()
        {
            m_subRenderEncoders.clear();
        }
        
        void FrameGraphExecuteGroup::BeginInternal()
        {
            //Create the autorelease pool to ensure command buffer is not leaked
            CreateAutoreleasePool();
            
            m_groupCommandBuffer = m_commandBuffer.AcquireMTLCommandBuffer();
            AZ_Assert(m_groupCommandBuffer, "Metal command Buffer was not created");
            m_groupCommandBuffer.label = m_cbLabel;
            m_workRequest.m_commandBuffer = &m_commandBuffer;
            
            //Encode any wait events from the attached scope at the start of the group.
            EncodeWaitEvents();
                      
            //Wait on all the fences related to transient resources, before the
            //encoders are created as per driver specs
            m_scope->WaitOnAllResourceFences(m_groupCommandBuffer);
            
            //Create all the render encoders before in order to establish order.This is because MTLCommandBuffer
            //always match the execution order of the sub render encoders to the order in which they were created
            for(int i = 0 ; i < m_workRequest.m_commandLists.size(); i++)
            {
                CommandList* commandList = AcquireCommandList();
                id <MTLCommandEncoder> subRenderEncoder = m_commandBuffer.AcquireSubRenderEncoder(m_scope->GetRenderPassDescriptor(), m_scope->GetId().GetCStr());
                m_subRenderEncoders.push_back(SubEncoderData {commandList, subRenderEncoder});
            }
        }
        
        void FrameGraphExecuteGroup::EndInternal()
        {
            //End the encoding for ParallelRendercommandEncoder
            m_commandBuffer.FlushParallelEncoder();
            //Signal all the fences related to transient resources, after the encoders
            //are flushed as per driver specs
            m_scope->SignalAllResourceFences(m_groupCommandBuffer);
            FlushAutoreleasePool();
        }
        
        void FrameGraphExecuteGroup::BeginContextInternal(
            RHI::FrameGraphExecuteContext& context,
            AZ::u32 contextIndex)
        {
            CommandList* commandList = m_subRenderEncoders[contextIndex].m_commandList;
            commandList->Open(m_subRenderEncoders[contextIndex].m_subRenderEncoder, m_groupCommandBuffer);
            m_workRequest.m_commandLists[contextIndex] = commandList;
            context.SetCommandList(*commandList);

            m_scope->Begin(*static_cast<CommandList*>(context.GetCommandList()), context.GetCommandListIndex(), context.GetCommandListCount());
        }

        void FrameGraphExecuteGroup::EndContextInternal(
            RHI::FrameGraphExecuteContext& context,
            AZ::u32 contextIndex)
        {
            m_scope->End(*static_cast<CommandList*>(context.GetCommandList()), context.GetCommandListIndex(), context.GetCommandListCount());
            static_cast<CommandList*>(context.GetCommandList())->Close();
        }
    }
}
