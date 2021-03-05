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
#pragma once

#include <Atom/RHI/FrameGraphCompiler.h>
#include <Atom/RHI/FrameGraphExecuter.h>
#include <Atom/RHI/FrameGraphExecuteGroup.h>
#include <Atom/RHI/FrameGraph.h>
#include <Atom/RHI/Scope.h>
#include <AzCore/std/containers/unordered_set.h>

namespace UnitTest
{
    class FrameGraphCompiler
        : public AZ::RHI::FrameGraphCompiler
    {
    public:
        AZ_CLASS_ALLOCATOR(FrameGraphCompiler, AZ::SystemAllocator, 0);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device&) override;

        void ShutdownInternal() override;

        AZ::RHI::MessageOutcome CompileInternal(const AZ::RHI::FrameGraphCompileRequest& request) override;
    };

    class FrameGraphExecuteGroup
        : public AZ::RHI::FrameGraphExecuteGroup
    {
        using Base = AZ::RHI::FrameGraphExecuteGroup;
    public:
        AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroup, AZ::SystemAllocator, 0);

        void Init(const AZ::RHI::ScopeId& scopeId);

        const AZ::RHI::ScopeId& GetId() const;

    private:
        AZ::RHI::ScopeId m_scopeId;
        AZ::RHI::CommandList* m_commandList = nullptr;
    };

    class FrameGraphExecuter
        : public AZ::RHI::FrameGraphExecuter
    {
    public:
        AZ_CLASS_ALLOCATOR(FrameGraphExecuter, AZ::SystemAllocator, 0);

    private:
        AZ::RHI::ResultCode InitInternal(const AZ::RHI::FrameGraphExecuterDescriptor&) override;

        void ShutdownInternal() override;

        void BeginInternal(const AZ::RHI::FrameGraph&) override;

        void ExecuteGroupInternal(AZ::RHI::FrameGraphExecuteGroup& group) override;

        void EndInternal() override;

        AZStd::unordered_set<AZ::RHI::ScopeId> m_scopeIds;
    };
}
