/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceShaderResourceGroupPool.h>

namespace AZ
{
    namespace Null
    {
        class ShaderResourceGroupPool final
            : public RHI::SingleDeviceShaderResourceGroupPool
        {
            using Base = RHI::SingleDeviceShaderResourceGroupPool;
        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroupPool, AZ::SystemAllocator);

            static RHI::Ptr<ShaderResourceGroupPool> Create();
            
        private:
            ShaderResourceGroupPool() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // Platform API
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::ShaderResourceGroupPoolDescriptor& descriptor) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InitGroupInternal([[maybe_unused]] RHI::SingleDeviceShaderResourceGroup& groupBase) override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override;
            RHI::ResultCode CompileGroupInternal([[maybe_unused]] RHI::SingleDeviceShaderResourceGroup& groupBase, [[maybe_unused]] const RHI::SingleDeviceShaderResourceGroupData& groupData) override { return RHI::ResultCode::Success;}
            void ShutdownResourceInternal([[maybe_unused]] RHI::SingleDeviceResource& resourceBase) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // FrameSchedulerEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////         
        };
    }
}
