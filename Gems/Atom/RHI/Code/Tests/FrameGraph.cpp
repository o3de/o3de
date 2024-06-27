/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Tests/FrameGraph.h>

namespace UnitTest
{
    using namespace AZ;

    RHI::ResultCode FrameGraphCompiler::InitInternal()
    {
        return RHI::ResultCode::Success;
    }

    void FrameGraphCompiler::ShutdownInternal()
    {
    }

    RHI::MessageOutcome FrameGraphCompiler::CompileInternal(const RHI::FrameGraphCompileRequest& request)
    {
        (void)request;
        return AZ::Success();
    }

    void FrameGraphExecuteGroup::Init(const RHI::ScopeId& scopeId)
    {
        m_scopeId = scopeId;

        InitRequest request;
        request.m_scopeId = scopeId;
        request.m_commandListCount = 1;
        request.m_commandLists = &m_commandList;
        Base::Init(request);
    }

    const RHI::ScopeId& FrameGraphExecuteGroup::GetId() const
    {
        return m_scopeId;
    }

    RHI::ResultCode FrameGraphExecuter::InitInternal(const AZ::RHI::FrameGraphExecuterDescriptor&)
    {
        return RHI::ResultCode::Success;
    }

    void FrameGraphExecuter::ShutdownInternal()
    {
    }

    void FrameGraphExecuter::BeginInternal(const RHI::FrameGraph& graph)
    {
        for (const RHI::Scope* scope : graph.GetScopes())
        {
            FrameGraphExecuteGroup* group = AddGroup<FrameGraphExecuteGroup>();
            group->Init(scope->GetId());

            [[maybe_unused]] const bool wasInserted = m_scopeIds.emplace(scope->GetId()).second;
            AZ_Assert(wasInserted, "scope was inserted already");
        }
    }

    void FrameGraphExecuter::ExecuteGroupInternal(RHI::FrameGraphExecuteGroup& group)
    {
        m_scopeIds.erase(static_cast<FrameGraphExecuteGroup&>(group).GetId());
    }

    void FrameGraphExecuter::EndInternal()
    {
        AZ_Assert(m_scopeIds.empty(), "there are still scopes in the queue");
    }
}
