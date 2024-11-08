/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/FrameGraphExecuteGroupMerged.h>
#include <RHI/Device.h>
#include <RHI/SwapChain.h>

namespace AZ
{
    namespace DX12
    {
        void FrameGraphExecuteGroupMerged::Init(
            Device& device,
            AZStd::vector<const Scope*>&& scopes,
            const RHI::ScopeId& mergedScopeId)
        {
            SetDevice(device);

            m_scopes = AZStd::move(scopes);

            // Constructing a new name is slow but copying one is fast, so we copy one passed in from the FrameGraphExecuter
            m_mergedScopeId = mergedScopeId;

            m_hardwareQueueClass = m_scopes.back()->GetHardwareQueueClass();
            m_workRequest.m_waitFences = m_scopes.front()->GetWaitFences();
            m_workRequest.m_signalFence = m_scopes.back()->GetSignalFenceValue();
            m_workRequest.m_commandLists.resize(1);

            AZStd::vector<InitMergedRequest::ScopeEntry> scopeEntries;
            scopeEntries.reserve(m_scopes.size());
            for (const Scope* scope : m_scopes)
            {
                scopeEntries.push_back({ scope->GetId(), scope->GetEstimatedItemCount() });

                {
                    auto& swapChainsToPresent = m_workRequest.m_swapChainsToPresent;

                    swapChainsToPresent.reserve(swapChainsToPresent.size() + scope->GetSwapChainsToPresent().size());
                    for (RHI::DeviceSwapChain* swapChain : scope->GetSwapChainsToPresent())
                    {
                        swapChainsToPresent.push_back(static_cast<SwapChain*>(swapChain));
                    }
                }

                {
                    auto& fencesToSignal = m_workRequest.m_userFencesToSignal;

                    fencesToSignal.reserve(fencesToSignal.size() + scope->GetFencesToSignal().size());
                    for (const RHI::Ptr<RHI::Fence>& fence : scope->GetFencesToSignal())
                    {
                        fencesToSignal.push_back(&static_cast<FenceImpl&>(*fence->GetDeviceFence(scope->GetDeviceIndex())).Get());
                    }
                }

                {
                    auto& fencesToWaitFor = m_workRequest.m_userFencesToWaitFor;

                    fencesToWaitFor.reserve(fencesToWaitFor.size() + scope->GetFencesToWaitFor().size());
                    for (const RHI::Ptr<RHI::Fence>& fence : scope->GetFencesToWaitFor())
                    {
                        fencesToWaitFor.push_back(&static_cast<FenceImpl&>(*fence->GetDeviceFence(scope->GetDeviceIndex())).Get());
                    }
                }
            }

            InitMergedRequest request;
            request.m_deviceIndex = device.GetDeviceIndex();
            request.m_scopeEntries = scopeEntries.data();
            request.m_scopeCount = static_cast<uint32_t>(scopeEntries.size());
            Base::Init(request);
        }

        void FrameGraphExecuteGroupMerged::BeginInternal()
        {
            CommandList* commandList = AcquireCommandList();
            commandList->Open(m_mergedScopeId);
            m_workRequest.m_commandLists.back() = commandList;
        }

        void FrameGraphExecuteGroupMerged::EndInternal()
        {
            static_cast<CommandList*>(m_workRequest.m_commandLists.back())->Close();
        }

        void FrameGraphExecuteGroupMerged::BeginContextInternal(
            RHI::FrameGraphExecuteContext& context,
            uint32_t contextIndex)
        {
            AZ_Assert(static_cast<uint32_t>(m_lastCompletedScope + 1) == contextIndex, "Contexts must be recorded in order!");

            const Scope* scope = m_scopes[contextIndex];

            CommandList* commandList = static_cast<CommandList*>(m_workRequest.m_commandLists.back());
            context.SetCommandList(*commandList);

            scope->Begin(*commandList, context.GetCommandListIndex(), context.GetCommandListCount());
        }

        void FrameGraphExecuteGroupMerged::EndContextInternal(
            RHI::FrameGraphExecuteContext& context,
            uint32_t contextIndex)
        {
            m_lastCompletedScope = contextIndex;

            const Scope* scope = m_scopes[contextIndex];
            CommandList* commandList = static_cast<CommandList*>(context.GetCommandList());
            scope->End(*commandList, context.GetCommandListIndex(), context.GetCommandListCount());
        }
    }
}
