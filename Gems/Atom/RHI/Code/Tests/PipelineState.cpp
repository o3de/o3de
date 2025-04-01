/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/PipelineState.h>

namespace UnitTest
{
    using namespace AZ;

    namespace
    {
        uint64_t Fibonacci(uint64_t n)
        {
            if (n <= 1)
            {
                return n;
            }
            return Fibonacci(n - 1) + Fibonacci(n - 2);
        }
    }

    void PipelineLibrary::ShutdownInternal()
    {
    }

    RHI::ResultCode PipelineLibrary::MergeIntoInternal([[maybe_unused]] AZStd::span<const RHI::DevicePipelineLibrary* const> libraries)
    {
        return RHI::ResultCode::Success;
    }

    RHI::ResultCode PipelineState::InitInternal(RHI::Device&, [[maybe_unused]] const RHI::PipelineStateDescriptorForDraw& descriptor, [[maybe_unused]] RHI::DevicePipelineLibrary* pipelineLibrary)
    {
        // Performs 'work' to simulate compiling a pso.

        uint64_t value = Fibonacci(22);

        return value > 0 ? RHI::ResultCode::Success : RHI::ResultCode::Fail;
    }

    RHI::ResultCode PipelineState::InitInternal(RHI::Device&, [[maybe_unused]] const RHI::PipelineStateDescriptorForDispatch& descriptor, [[maybe_unused]] RHI::DevicePipelineLibrary* pipelineLibrary)
    {
        // Performs 'work' to simulate compiling a pso.

        uint64_t value = Fibonacci(22);

        return value > 0 ? RHI::ResultCode::Success : RHI::ResultCode::Fail;
    }

    RHI::ResultCode PipelineState::InitInternal(RHI::Device&, [[maybe_unused]] const RHI::PipelineStateDescriptorForRayTracing& descriptor, [[maybe_unused]] RHI::DevicePipelineLibrary* pipelineLibrary)
    {
        // Performs 'work' to simulate compiling a pso.

        uint64_t value = Fibonacci(22);

        return value > 0 ? RHI::ResultCode::Success : RHI::ResultCode::Fail;
    }

}
