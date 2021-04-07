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

#include <Atom/RHI/SwapChain.h>

namespace AZ
{
    namespace Null
    {
        class SwapChain
            : public RHI::SwapChain
        {
            using Base = RHI::SwapChain;
        public:
            AZ_RTTI(SwapChain, "{FD1CC898-684A-46A5-92C3-519CD8E490D7}", Base);
            AZ_CLASS_ALLOCATOR(SwapChain, AZ::SystemAllocator, 0);

            static RHI::Ptr<SwapChain> Create();
            
        private:
            SwapChain() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::SwapChain
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::SwapChainDescriptor& descriptor, [[maybe_unused]] RHI::SwapChainDimensions* nativeDimensions) override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override {}
            uint32_t PresentInternal() override {return 0;}
            RHI::ResultCode InitImageInternal([[maybe_unused]] const InitImageRequest& request) override { return RHI::ResultCode::Success;}
            void ShutdownResourceInternal([[maybe_unused]] RHI::Resource& resourceBase) override {}
            RHI::ResultCode ResizeInternal([[maybe_unused]] const RHI::SwapChainDimensions& dimensions, [[maybe_unused]] RHI::SwapChainDimensions* nativeDimensions) override { return RHI::ResultCode::Success;}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
