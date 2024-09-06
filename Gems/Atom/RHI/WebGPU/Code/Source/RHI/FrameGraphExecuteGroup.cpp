/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Device.h>
#include <RHI/FrameGraphExecuteGroup.h>
#include <RHI/SwapChain.h>

namespace AZ::WebGPU
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
        m_workRequest.m_commandLists.resize(commandListCount);

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
        [[maybe_unused]] RHI::FrameGraphExecuteContext& context, [[maybe_unused]] uint32_t contextIndex)
    {
    }

    void FrameGraphExecuteGroup::EndContextInternal(
        [[maybe_unused]] RHI::FrameGraphExecuteContext& context,
        [[maybe_unused]] uint32_t contextIndex)
    {
    }
}
