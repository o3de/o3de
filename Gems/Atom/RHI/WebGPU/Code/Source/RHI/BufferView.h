/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceBufferView.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/Buffer.h>

namespace AZ::WebGPU
{
    class BufferView final
        : public RHI::DeviceBufferView
    {
        using Base = RHI::DeviceBufferView;
    public:
        AZ_CLASS_ALLOCATOR(BufferView, AZ::ThreadPoolAllocator);
        AZ_RTTI(BufferView, "{EB1B6718-AA10-41C8-A86E-D74633D7A3D8}", Base);

        static RHI::Ptr<BufferView> Create();

    private:
        BufferView() = default;

        //////////////////////////////////////////////////////////////////////////
        // RHI::DeviceBufferView
        RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::DeviceResource& resourceBase) override { return RHI::ResultCode::Success;}
        RHI::ResultCode InvalidateInternal() override { return RHI::ResultCode::Success;}
        void ShutdownInternal() override{};
        //////////////////////////////////////////////////////////////////////////
    };
}
