/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/ShaderResourceGroupPool.h>
#include <RHI/MemoryView.h>

namespace AZ
{
    namespace Metal
    {
        class ShaderResourceGroupPool final
            : public RHI::ShaderResourceGroupPool
        {
            using Base = RHI::ShaderResourceGroupPool;
        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroupPool, AZ::SystemAllocator, 0);

            static RHI::Ptr<ShaderResourceGroupPool> Create();
            
        private:
            ShaderResourceGroupPool() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // Platform API
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::ShaderResourceGroupPoolDescriptor& descriptor) override;
            RHI::ResultCode InitGroupInternal(RHI::ShaderResourceGroup& groupBase) override;
            void ShutdownInternal() override;
            RHI::ResultCode CompileGroupInternal(RHI::ShaderResourceGroup& groupBase, const RHI::ShaderResourceGroupData& groupData) override;
            void ShutdownResourceInternal(RHI::Resource& resourceBase) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // FrameSchedulerEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////
                        
            Device* m_device = nullptr;
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> m_srgLayout;
        };
    }
}
