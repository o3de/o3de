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
            AZ_CLASS_ALLOCATOR(FrameGraphExecuter, AZ::SystemAllocator, 0);

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
