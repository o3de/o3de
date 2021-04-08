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

#include <Atom/RHI/BufferView.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/Buffer.h>

namespace AZ
{
    namespace Null
    {
        class BufferView final
            : public RHI::BufferView
        {
            using Base = RHI::BufferView;
        public:
            AZ_CLASS_ALLOCATOR(BufferView, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(BufferView, "{FE5E5E22-FC6C-4BDF-BD9B-427072C3C43F}", Base);

            static RHI::Ptr<BufferView> Create();

        private:
            BufferView() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::BufferView
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::Resource& resourceBase) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InvalidateInternal() override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override{};
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
