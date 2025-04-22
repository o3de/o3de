/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/FrameSchedulerEnums.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupPoolDescriptor.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/ResourcePool.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/ShaderResourceGroupInvalidateRegistry.h>
#include <Atom/RHI/DeviceShaderResourceGroupPool.h>

#include <AzCore/std/parallel/containers/concurrent_vector.h>

namespace AZ::RHI
{
    //! The platform-independent base class for ShaderResourceGroupPools. Platforms
    //! should inherit from this class to implement platform-dependent pooling of
    //! multi-device shader resource groups.
    class ShaderResourceGroupPool : public ResourcePool
    {
        friend class ShaderResourceGroup;

    public:
        AZ_CLASS_ALLOCATOR(ShaderResourceGroupPool, AZ::SystemAllocator, 0);
        AZ_RTTI(ShaderResourceGroupPool, "{5F10711E-C47A-40CC-8BEB-8AC161206A1E}", ResourcePool);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(ShaderResourceGroupPool);
        ShaderResourceGroupPool() = default;
        virtual ~ShaderResourceGroupPool() override = default;

        //! Initializes the shader resource group pool for all devices noted in the deviceMask.
        ResultCode Init(const ShaderResourceGroupPoolDescriptor& descriptor);

        //! Initializes the resource group and associates it with the pool. The resource
        //! group must be updated on this pool.
        ResultCode InitGroup(ShaderResourceGroup& srg);

        //! Compile Shader Resource Group with the associated ShaderResourceGroupData
        ResultCode CompileGroup(
            ShaderResourceGroup& shaderResourceGroup, const ShaderResourceGroupData& shaderResourceGroupData);

        //! Returns the descriptor passed at initialization time.
        const ShaderResourceGroupPoolDescriptor& GetDescriptor() const override;

        //! Returns the SRG layout used when initializing the pool.
        const ShaderResourceGroupLayout* GetLayout() const;

        //! Begins compilation of the pool. Cannot be called recursively.
        //! @param groupsToCompileCount if non-null, is assigned to the number of groups needing to be compiled.
        void CompileGroupsBegin();

        //! Ends compilation of the pool. Must be preceeded by a CompileGroupsBegin() call.
        void CompileGroupsEnd();

        //////////////////////////////////////////////////////////////////////////
        // The following methods must be called within a CompileGroups{Begin, End} region.

        //! Compiles an interval [min, max) of groups.
        void CompileGroupsForInterval(Interval interval);

        //! Returns the total number of groups that need to be compiled.
        uint32_t GetGroupsToCompileCount() const;

        //////////////////////////////////////////////////////////////////////////

        //! Returns whether layout in this pool has constants.
        bool HasConstants() const;

        //! Returns whether groups in this pool have an image table.
        bool HasImageGroup() const;

        //! Returns whether groups in this pool have a buffer table.
        bool HasBufferGroup() const;

        //! Returns whether groups in this pool have a sampler table.
        bool HasSamplerGroup() const;

        void Shutdown() override final;

    private:
        ShaderResourceGroupPoolDescriptor m_descriptor;
        bool m_hasConstants = false;
        bool m_hasBufferGroup = false;
        bool m_hasImageGroup = false;
        bool m_hasSamplerGroup = false;
    };
} // namespace AZ::RHI
