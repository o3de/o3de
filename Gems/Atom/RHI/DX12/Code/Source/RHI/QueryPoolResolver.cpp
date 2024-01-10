/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Device.h>
#include <RHI/Buffer.h>
#include <RHI/CommandList.h>
#include <RHI/QueryPool.h>
#include <RHI/QueryPoolResolver.h>
#include <RHI/Scope.h>

#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/unordered_set.h>

namespace AZ
{
    namespace DX12
    {
        QueryPoolResolver::QueryPoolResolver(Device& device, QueryPool& queryPool)
            : m_queryPool(queryPool)
            , m_device(device)
        {
            m_resolveFence = FenceImpl::Create();
            m_resolveFence->Init(m_device, RHI::FenceState::Reset);

        }

        uint64_t QueryPoolResolver::QueueResolveRequest(const ResolveRequest& request)
        {
            m_resolveRequests.push_back(request);
            return m_resolveFence->Get().Increment();
        }

        bool QueryPoolResolver::IsResolveFinished(uint64_t fenceValue)
        {
            return fenceValue <= m_resolveFence->Get().GetCompletedValue();
        }

        void QueryPoolResolver::WaitForResolve(uint64_t fenceValue)
        {
            FenceEvent event("WaitForResolve");
            m_resolveFence->Get().Wait(event, fenceValue);
        }

        void QueryPoolResolver::Compile(Scope& scope)
        {
            if (!m_resolveRequests.empty())
            {
                scope.AddFenceToSignal(AZStd::static_pointer_cast<RHI::SingleDeviceFence>(m_resolveFence));
            }
        }

        void QueryPoolResolver::Resolve(CommandList& commandList) const
        {
            for (auto& request : m_resolveRequests)
            {
                commandList.GetCommandList()->ResolveQueryData(
                    m_queryPool.GetHeap(), 
                    request.m_queryType,
                    request.m_firstQuery,
                    request.m_queryCount,
                    request.m_resolveBuffer, 
                    request.m_offset);
            }
        }

        void QueryPoolResolver::Deactivate()
        {
            m_resolveRequests.clear();
        }
    }
}
