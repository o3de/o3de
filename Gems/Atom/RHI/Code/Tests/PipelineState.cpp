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

    RHI::ResultCode PipelineLibrary::MergeIntoInternal([[maybe_unused]] AZStd::array_view<const RHI::PipelineLibrary*> libraries)
    {
        return RHI::ResultCode::Success;
    }

    RHI::ResultCode PipelineState::InitInternal(RHI::Device&, [[maybe_unused]] const RHI::PipelineStateDescriptorForDraw& descriptor, [[maybe_unused]] RHI::PipelineLibrary* pipelineLibrary)
    {
        // Performs 'work' to simulate compiling a pso.

        uint64_t value = Fibonacci(22);

        return value > 0 ? RHI::ResultCode::Success : RHI::ResultCode::Fail;
    }

    RHI::ResultCode PipelineState::InitInternal(RHI::Device&, [[maybe_unused]] const RHI::PipelineStateDescriptorForDispatch& descriptor, [[maybe_unused]] RHI::PipelineLibrary* pipelineLibrary)
    {
        // Performs 'work' to simulate compiling a pso.

        uint64_t value = Fibonacci(22);

        return value > 0 ? RHI::ResultCode::Success : RHI::ResultCode::Fail;
    }

    RHI::ResultCode PipelineState::InitInternal(RHI::Device&, [[maybe_unused]] const RHI::PipelineStateDescriptorForRayTracing& descriptor, [[maybe_unused]] RHI::PipelineLibrary* pipelineLibrary)
    {
        // Performs 'work' to simulate compiling a pso.

        uint64_t value = Fibonacci(22);

        return value > 0 ? RHI::ResultCode::Success : RHI::ResultCode::Fail;
    }

}
