/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <RHI/Device.h>
#include <Atom/RHI/FrameGraphExecuter.h>
#include <Atom/RHI/FrameGraphExecuteGroup.h>

namespace AZ
{
    namespace Null
    {
        class FrameGraphExecuter final
            : public RHI::FrameGraphExecuter
        {
            using Base = RHI::FrameGraphExecuter;
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuter, AZ::SystemAllocator);

            static RHI::Ptr<FrameGraphExecuter> Create();

        private:

            //////////////////////////////////////////////////////////////////////////
            // RHI::FrameGraphExecuter
            RHI::ResultCode InitInternal([[maybe_unused]] const RHI::FrameGraphExecuterDescriptor& descriptor) override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override {}
            void BeginInternal([[maybe_unused]] const RHI::FrameGraph& frameGraph) override {}
            void ExecuteGroupInternal([[maybe_unused]] RHI::FrameGraphExecuteGroup& group) override {}
            void EndInternal() override {}
            ///////////////////////////////////////////////////////////////////////////
        };
    }
}
