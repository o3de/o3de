/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/FrameGraph.h>
#include <Atom/RHI/FrameGraphExecuter.h>
#include <RHI/CommandQueue.h>
#include <RHI/FrameGraphExecuteGroupHandler.h>

#include <AzCore/std/containers/unordered_map.h>
#include <Atom/RHI.Reflect/Vulkan/PlatformLimitsDescriptor.h>
namespace AZ
{
    namespace Vulkan
    {
        class FrameGraphExecuter final
            : public RHI::FrameGraphExecuter
        {
            using Base = RHI::FrameGraphExecuter;

        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuter, AZ::SystemAllocator);
            AZ_RTTI(FrameGraphExecuter, "22B6E224-9469-4D8B-828F-A81C83B6EEEC", Base);

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
            //////////////////////////////////////////////////////////////////////////

            // Adds a handler for a list of execute groups.
            void AddExecuteGroupHandler(const RHI::GraphGroupId& groupId, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& groups);

            // List of handlers for execute groups.
            AZStd::unordered_map<RHI::GraphGroupId, AZStd::unique_ptr<FrameGraphExecuteGroupHandler>> m_groupHandlers;
            AZStd::unordered_map<int, FrameGraphExecuterData> m_frameGraphExecuterData;
        };
    }
}
