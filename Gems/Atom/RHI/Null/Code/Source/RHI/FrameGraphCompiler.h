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
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace Null
    {
        class FrameGraphCompiler final
            : public RHI::FrameGraphCompiler
        {
            using Base = RHI::FrameGraphCompiler;
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphCompiler, AZ::SystemAllocator, 0);

            static RHI::Ptr<FrameGraphCompiler> Create();

        private:
            FrameGraphCompiler() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::FrameGraphCompiler
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device) override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override {}
            RHI::MessageOutcome CompileInternal([[maybe_unused]] const RHI::FrameGraphCompileRequest& request) override { return AZ::Success();}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
