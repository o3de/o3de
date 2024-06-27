/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/FrameGraphExecuteGroup.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/DeviceImage.h>

namespace AZ::RHI
{
    void FrameGraphExecuteGroup::Init(const InitMergedRequest& request)
    {
        AZ_Assert(request.m_scopeCount, "Must have at least one scope.");

        m_jobPolicy = JobPolicy::Serial;

        FrameGraphExecuteContext::Descriptor descriptor;
        descriptor.m_deviceIndex = request.m_deviceIndex;
        descriptor.m_commandList = request.m_commandList;
        descriptor.m_commandListCount = 1;
        descriptor.m_commandListIndex = 0;

        for (uint32_t i = 0; i < request.m_scopeCount; ++i)
        {
            descriptor.m_scopeId = request.m_scopeEntries[i].m_scopeId;
            descriptor.m_submitRange = { 0, request.m_scopeEntries[i].m_submitCount };
            m_contexts.emplace_back(descriptor);
        }
    }

    void FrameGraphExecuteGroup::Init(const InitRequest& request)
    {
        AZ_Assert(request.m_commandListCount > 0, "Must have at least one command list.");

        m_jobPolicy = request.m_jobPolicy;

        FrameGraphExecuteContext::Descriptor descriptor;
        descriptor.m_scopeId = request.m_scopeId;
        descriptor.m_deviceIndex = request.m_deviceIndex;
        descriptor.m_commandListCount = request.m_commandListCount;

        // build the execute contexts
        // Note: each context includes a submission range, with the number of items in range equal to (submitCount / commandListCount)
        uint32_t submitCount = request.m_submitCount;
        uint32_t commandListCount = request.m_commandListCount;
        for (uint32_t i = 0; i < request.m_commandListCount; ++i)
        {
            descriptor.m_commandListIndex = i;
            descriptor.m_commandList = request.m_commandLists[i];
            descriptor.m_submitRange = { (i * submitCount) / commandListCount, ((i + 1) * submitCount) / commandListCount };
            m_contexts.emplace_back(descriptor);
        }
    }

    uint32_t FrameGraphExecuteGroup::GetContextCount() const
    {
        return static_cast<uint32_t>(m_contexts.size());
    }

    FrameGraphExecuteContext* FrameGraphExecuteGroup::BeginContext(uint32_t contextIndex)
    {
        const int32_t activeCount = ++m_contextCountActive;
        if (activeCount > 1)
        {
            AZ_Assert(
                m_jobPolicy == JobPolicy::Parallel,
                "Multiple FrameSchedulerExecuteContexts in this batch are being recorded simultaneously, but the job policy forbids it.");
        }
        BeginContextInternal(m_contexts[contextIndex], contextIndex);
        return &m_contexts[contextIndex];
    }

    void FrameGraphExecuteGroup::EndContext(uint32_t contextIndex)
    {
        EndContextInternal(m_contexts[contextIndex], contextIndex);

        [[maybe_unused]] const int32_t activeCount = --m_contextCountActive;
        AZ_Assert(activeCount >= 0, "Asymmetric calls to FrameSchedulerExecuteContext:: Begin / End.");
        ++m_contextCountCompleted;
    }

    JobPolicy FrameGraphExecuteGroup::GetJobPolicy() const
    {
        return m_jobPolicy;
    }

    bool FrameGraphExecuteGroup::IsComplete() const
    {
        return m_contextCountCompleted == static_cast<int32_t>(m_contexts.size());
    }

    bool FrameGraphExecuteGroup::IsSubmittable() const
    {
        return m_isSubmittable;
    }

    void FrameGraphExecuteGroup::BeginInternal()
    {
    }

    void FrameGraphExecuteGroup::BeginContextInternal(
        FrameGraphExecuteContext& context,
        uint32_t contextIndex)
    {
        AZ_UNUSED(context);
        AZ_UNUSED(contextIndex);
    }

    // Called when a context in the group has ended recording.
    void FrameGraphExecuteGroup::EndContextInternal(
        FrameGraphExecuteContext& context,
        uint32_t contextIndex)
    {
        AZ_UNUSED(context);
        AZ_UNUSED(contextIndex);
    }

    void FrameGraphExecuteGroup::EndInternal() {}
}
