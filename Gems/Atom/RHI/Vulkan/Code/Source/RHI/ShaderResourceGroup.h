/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceShaderResourceGroup.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/mutex.h>
#include <RHI/Buffer.h>
#include <RHI/DescriptorPool.h>
#include <RHI/DescriptorSetLayout.h>
#include <RHI/DescriptorSet.h>

namespace AZ
{
    namespace Vulkan
    {
        class ShaderResourceGroupPool;

        class ShaderResourceGroup
            : public RHI::DeviceShaderResourceGroup
        {
            using Base = RHI::DeviceShaderResourceGroup;
            friend class ShaderResourceGroupPool;

        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroup, AZ::SystemAllocator);
            AZ_RTTI(ShaderResourceGroup, "DB59214E-57B4-4F7B-B273-CB5210826A57", Base);

            static RHI::Ptr<ShaderResourceGroup> Create();
            ~ShaderResourceGroup() = default;

            void UpdateCompiledDataIndex(uint64_t frameIteration);
            const DescriptorSet& GetCompiledData() const;
            uint32_t GetCompileDataIndex() const;
            uint64_t GetLastCompileFrameIteration() const;

        protected:
            ShaderResourceGroup() = default;

        private:
            /// The current index into the compiled data array.
            uint32_t m_compiledDataIndex = 0;
            uint64_t m_lastCompileFrameIteration = 0;
            AZStd::vector<RHI::Ptr<DescriptorSet>> m_compiledData;
        };
    }
}
