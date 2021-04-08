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

#include <Atom/RHI/ShaderResourceGroupPool.h>

namespace AZ
{
    namespace Null
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
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::ShaderResourceGroupPoolDescriptor& descriptor) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InitGroupInternal([[maybe_unused]] RHI::ShaderResourceGroup& groupBase) override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override;
            RHI::ResultCode CompileGroupInternal([[maybe_unused]] RHI::ShaderResourceGroup& groupBase, [[maybe_unused]] const RHI::ShaderResourceGroupData& groupData) override { return RHI::ResultCode::Success;}
            void ShutdownResourceInternal([[maybe_unused]] RHI::Resource& resourceBase) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // FrameSchedulerEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////         
        };
    }
}
