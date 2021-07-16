/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ShaderResourceGroupPool.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/DescriptorSetAllocator.h>
#include <RHI/DescriptorSetLayout.h>


namespace AZ
{
    namespace Vulkan
    {
        class ShaderResourceGroupPool
            : public RHI::ShaderResourceGroupPool
        {
            using Base = RHI::ShaderResourceGroupPool;
            friend class ShaderResourceGroup;

        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroupPool, AZ::SystemAllocator, 0);
            AZ_RTTI(ShaderResourceGroupPool, "593C5F3B-67FD-415C-8305-8C5D272649B7", Base);

            static RHI::Ptr<ShaderResourceGroupPool> Create();

        protected:
            //////////////////////////////////////////////////////////////////////////
            // Platform API
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::ShaderResourceGroupPoolDescriptor& descriptor) override;
            RHI::ResultCode InitGroupInternal(RHI::ShaderResourceGroup& groupBase) override;
            void ShutdownInternal() override;
            RHI::ResultCode CompileGroupInternal(RHI::ShaderResourceGroup& groupBase, const RHI::ShaderResourceGroupData& groupData) override;
            void ShutdownResourceInternal(RHI::Resource& resourceBase) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // FrameEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////

        private:
            RHI::Ptr<BufferPool> m_constantBufferPool;
            uint32_t m_descriptorSetCount = 0;
            RHI::Ptr<DescriptorSetAllocator> m_descriptorSetAllocator;
            RHI::Ptr<DescriptorSetLayout> m_descriptorSetLayout;
            uint64_t m_currentIteration = 0;
        };
    }
}
