/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(FrameGraphCompiler, AZ::SystemAllocator);

    private:
        AZ::RHI::ResultCode InitInternal() override;

        void ShutdownInternal() override;

        AZ::RHI::MessageOutcome CompileInternal(const AZ::RHI::FrameGraphCompileRequest& request) override;
    };

    class FrameGraphExecuteGroup
        : public AZ::RHI::FrameGraphExecuteGroup
    {
        using Base = AZ::RHI::FrameGraphExecuteGroup;
    public:
        AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroup, AZ::SystemAllocator);

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
        AZ_CLASS_ALLOCATOR(FrameGraphExecuter, AZ::SystemAllocator);

    private:
        AZ::RHI::ResultCode InitInternal(const AZ::RHI::FrameGraphExecuterDescriptor&) override;

        void ShutdownInternal() override;

        void BeginInternal(const AZ::RHI::FrameGraph&) override;

        void ExecuteGroupInternal(AZ::RHI::FrameGraphExecuteGroup& group) override;

        void EndInternal() override;

        AZStd::unordered_set<AZ::RHI::ScopeId> m_scopeIds;
    };
}
