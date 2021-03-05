/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <Tests/FrameGraph.h>

namespace UnitTest
{
    using namespace AZ;

    RHI::ResultCode FrameGraphCompiler::InitInternal(RHI::Device&)
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

            const bool wasInserted = m_scopeIds.emplace(scope->GetId()).second;
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
