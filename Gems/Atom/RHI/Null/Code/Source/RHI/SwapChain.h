/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceSwapChain.h>

namespace AZ
{
    namespace Null
    {
        class SwapChain
            : public RHI::DeviceSwapChain
        {
            using Base = RHI::DeviceSwapChain;
        public:
            AZ_RTTI(SwapChain, "{FD1CC898-684A-46A5-92C3-519CD8E490D7}", Base);
            AZ_CLASS_ALLOCATOR(SwapChain, AZ::SystemAllocator);

            static RHI::Ptr<SwapChain> Create();
            
        private:
            SwapChain() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceSwapChain
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::SwapChainDescriptor& descriptor, [[maybe_unused]] RHI::SwapChainDimensions* nativeDimensions) override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override {}
            uint32_t PresentInternal() override {return 0;}
            RHI::ResultCode InitImageInternal([[maybe_unused]] const InitImageRequest& request) override { return RHI::ResultCode::Success;}
            void ShutdownResourceInternal([[maybe_unused]] RHI::DeviceResource& resourceBase) override {}
            RHI::ResultCode ResizeInternal([[maybe_unused]] const RHI::SwapChainDimensions& dimensions, [[maybe_unused]] RHI::SwapChainDimensions* nativeDimensions) override { return RHI::ResultCode::Success;}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
