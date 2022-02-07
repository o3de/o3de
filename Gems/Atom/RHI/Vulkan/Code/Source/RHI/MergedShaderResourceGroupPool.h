/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/ShaderResourceGroupPool.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/DescriptorSetAllocator.h>
#include <RHI/DescriptorSetLayout.h>

#include <Atom/RHI/ObjectCache.h>

namespace AZ
{
    namespace Vulkan
    {
        class ShaderResourceGroup;
        class MergedShaderResourceGroup;

        //! ShaderResourceGroup resource pool for creating and managing MergedShaderResouceGroups.
        //! MergedShaderResourceGroups can only be created through this pool since we want to be able to
        //! reuse them as much as possible. The pool contains an object cache to carry a number of
        //! MergedShaderResourceGroups for re usability.
        //! A merged ShaderResourceGroupLayout must be provided at initialization time.
        class MergedShaderResourceGroupPool final
            : public ShaderResourceGroupPool
        {
            using Base = ShaderResourceGroupPool;

        public:
            AZ_CLASS_ALLOCATOR(MergedShaderResourceGroupPool, AZ::SystemAllocator, 0);
            AZ_RTTI(MergedShaderResourceGroupPool, "{9CBCF750-0BE7-410E-9828-ACA55ED828AD}", Base);

            static RHI::Ptr<MergedShaderResourceGroupPool> Create();

            using ShaderResourceGroupList = AZStd::fixed_vector<const ShaderResourceGroup*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax>;
            //! Finds or create a new instance of a MergedShaderResourceGroup.
            //! @param shaderResourceGroupList The list of ShaderResourceGroups that are being merged.
            MergedShaderResourceGroup* FindOrCreate(const ShaderResourceGroupList& shaderResourceGroupList);

        private:
            //////////////////////////////////////////////////////////////////////////
            // RHI::ResourcePool
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::ShaderResourceGroupPoolDescriptor& descriptor) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            AZStd::shared_mutex m_databaseMutex;
            static const uint32_t CacheDatabaseCapacity = 1000;
            RHI::ObjectCache<MergedShaderResourceGroup, MergedShaderResourceGroup::ShaderResourceGroupArray> m_cacheDatabase;
        };
    }
}
