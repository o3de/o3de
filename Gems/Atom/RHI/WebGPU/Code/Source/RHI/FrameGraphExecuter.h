/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/FrameGraphExecuter.h>
#include <Atom/RHI/FrameGraphExecuteGroup.h>
#include <Atom/RHI.Reflect/WebGPU/PlatformLimitsDescriptor.h>

namespace AZ::WebGPU
{
    class FrameGraphExecuter final
        : public RHI::FrameGraphExecuter
    {
        using Base = RHI::FrameGraphExecuter;
    public:
        AZ_CLASS_ALLOCATOR(FrameGraphExecuter, AZ::SystemAllocator);

        static RHI::Ptr<FrameGraphExecuter> Create();

    private:

        FrameGraphExecuter();

        //////////////////////////////////////////////////////////////////////////
        // RHI::FrameGraphExecuter
        RHI::ResultCode InitInternal(const RHI::FrameGraphExecuterDescriptor& descriptor) override;
        void ShutdownInternal() override;
        void BeginInternal(const RHI::FrameGraph& frameGraph) override;
        void ExecuteGroupInternal(RHI::FrameGraphExecuteGroup& group) override;
        void EndInternal() override;
        ///////////////////////////////////////////////////////////////////////////

        const RHI::ScopeId m_mergedScopeId{ "Merged" };
        AZStd::unordered_map<int, FrameGraphExecuterData> m_frameGraphExecuterData;
    };
}
