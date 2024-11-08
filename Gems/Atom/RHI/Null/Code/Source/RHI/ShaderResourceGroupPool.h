/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceShaderResourceGroupPool.h>

namespace AZ
{
    namespace Null
    {
        class ShaderResourceGroupPool final
            : public RHI::DeviceShaderResourceGroupPool
        {
            using Base = RHI::DeviceShaderResourceGroupPool;
        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroupPool, AZ::SystemAllocator);

            static RHI::Ptr<ShaderResourceGroupPool> Create();
            
        private:
            ShaderResourceGroupPool() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // Platform API
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::ShaderResourceGroupPoolDescriptor& descriptor) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InitGroupInternal([[maybe_unused]] RHI::DeviceShaderResourceGroup& groupBase) override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override;
            RHI::ResultCode CompileGroupInternal([[maybe_unused]] RHI::DeviceShaderResourceGroup& groupBase, [[maybe_unused]] const RHI::DeviceShaderResourceGroupData& groupData) override { return RHI::ResultCode::Success;}
            void ShutdownResourceInternal([[maybe_unused]] RHI::DeviceResource& resourceBase) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // FrameSchedulerEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////         
        };
    }
}
