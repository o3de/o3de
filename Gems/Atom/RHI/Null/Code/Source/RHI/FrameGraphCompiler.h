/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
