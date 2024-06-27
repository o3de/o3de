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
    namespace Metal
    {
        class FrameGraphCompiler final
            : public RHI::FrameGraphCompiler
        {
            using Base = RHI::FrameGraphCompiler;
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphCompiler, AZ::SystemAllocator);

            static RHI::Ptr<FrameGraphCompiler> Create();

        private:
            FrameGraphCompiler() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::FrameGraphCompiler
            RHI::ResultCode InitInternal() override;
            void ShutdownInternal() override;
            RHI::MessageOutcome CompileInternal(const RHI::FrameGraphCompileRequest& request) override;
            //////////////////////////////////////////////////////////////////////////
            
            void CompileAsyncQueueFences(const RHI::FrameGraph& frameGraph);
        };
    }
}
