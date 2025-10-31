/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/FrameGraphExecuteGroup.h>
#include <RHI/SwapChain.h>

namespace AZ
{
    namespace DX12
    {
        void FrameGraphExecuteGroup::Init(
            Device& device,
            const Scope& scope,
            uint32_t commandListCount,
            RHI::JobPolicy globalJobPolicy)
        {
            SetDevice(device);
            m_scope = &scope;

            m_hardwareQueueClass = scope.GetHardwareQueueClass();
            m_workRequest.m_waitFences = scope.GetWaitFences();
            m_workRequest.m_signalFence = scope.GetSignalFenceValue();
            m_workRequest.m_commandLists.resize(commandListCount);

            {
                auto& fencesToSignal = m_workRequest.m_userFencesToSignal;

                fencesToSignal.reserve(scope.GetFencesToSignal().size());
                for (const RHI::Ptr<RHI::Fence>& fence : scope.GetFencesToSignal())
                {
                    fencesToSignal.push_back(&static_cast<FenceImpl&>(*fence->GetDeviceFence(scope.GetDeviceIndex())).Get());
                }
            }

            {
                auto& swapChainsToPresent = m_workRequest.m_swapChainsToPresent;

                swapChainsToPresent.reserve(scope.GetSwapChainsToPresent().size());
                for (RHI::DeviceSwapChain* swapChain : scope.GetSwapChainsToPresent())
                {
                    swapChainsToPresent.push_back(static_cast<SwapChain*>(swapChain));
                }
            }

            InitRequest request;
            request.m_scopeId = scope.GetId();
            request.m_deviceIndex = scope.GetDeviceIndex();
            request.m_submitCount = scope.GetEstimatedItemCount();
            request.m_commandLists = reinterpret_cast<RHI::CommandList* const*>(m_workRequest.m_commandLists.data());
            request.m_commandListCount = commandListCount;
            request.m_jobPolicy = globalJobPolicy;
            Base::Init(request);
        }

        void FrameGraphExecuteGroup::BeginContextInternal(
            RHI::FrameGraphExecuteContext& context,
            uint32_t contextIndex)
        {
            CommandList* commandList = AcquireCommandList();
            const RHI::ScopeId& scopeId = context.GetScopeId();

            commandList->Open(scopeId);
            m_workRequest.m_commandLists[contextIndex] = commandList;
            context.SetCommandList(*commandList);

            m_scope->Begin(*commandList, context.GetCommandListIndex(), context.GetCommandListCount());
        }

        void FrameGraphExecuteGroup::EndContextInternal(
            RHI::FrameGraphExecuteContext& context,
            uint32_t contextIndex)
        {
            (void)contextIndex;
            CommandList* commandList = static_cast<CommandList*>(context.GetCommandList());
            m_scope->End(*commandList, context.GetCommandListIndex(), context.GetCommandListCount());
            commandList->Close();
        }
    }
}
