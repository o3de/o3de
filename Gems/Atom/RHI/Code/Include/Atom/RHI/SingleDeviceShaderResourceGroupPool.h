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
#include <Atom/RHI/SingleDeviceBufferPool.h>
#include <Atom/RHI/SingleDeviceShaderResourceGroup.h>
#include <Atom/RHI/ShaderResourceGroupInvalidateRegistry.h>
#include <Atom/RHI/SingleDeviceResourcePool.h>

#include <AzCore/std/parallel/containers/concurrent_vector.h>

namespace AZ::RHI
{
    //! The platform-independent base class for ShaderResourceGroupPools. Platforms
    //! should inherit from this class to implement platform-dependent pooling of
    //! shader resource groups.
    class SingleDeviceShaderResourceGroupPool
        : public SingleDeviceResourcePool
    {
        friend class SingleDeviceShaderResourceGroup;
    public:
        AZ_RTTI(SingleDeviceShaderResourceGroupPool, "{9AAB5A85-4063-4BAE-9A9C-E25640F18FFA}", SingleDeviceResourcePool);
        virtual ~SingleDeviceShaderResourceGroupPool() override;

        //! Initializes the shader resource group pool.
        ResultCode Init(Device& device, const ShaderResourceGroupPoolDescriptor& descriptor);

        //! Initializes the resource group and associates it with the pool. The resource
        //! group must be updated on this pool.
        ResultCode InitGroup(SingleDeviceShaderResourceGroup& srg);
            
        //! Compile Shader Resource Group with the associated SingleDeviceShaderResourceGroupData
        ResultCode CompileGroup(SingleDeviceShaderResourceGroup& shaderResourceGroup,
                                const SingleDeviceShaderResourceGroupData& shaderResourceGroupData);

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

    protected:
        SingleDeviceShaderResourceGroupPool();

        //////////////////////////////////////////////////////////////////////////
        // SingleDeviceResourcePool overrides
        void ShutdownInternal() override;
        void ShutdownResourceInternal(SingleDeviceResource& resource) override;
        void ComputeFragmentation() const override
        {
            // Fragmentation for SRG descriptors not currently measured
        }
        //////////////////////////////////////////////////////////////////////////

    private:
        // Queues the shader resource group for compile and provides a new data packet (takes a lock).
        void QueueForCompile(SingleDeviceShaderResourceGroup& group, const SingleDeviceShaderResourceGroupData& groupData);

        // Queues the shader resource group for compile. Legal to call on a queued group. Takes a lock.
        void QueueForCompile(SingleDeviceShaderResourceGroup& group);

        // Queues the shader resource group for compile. Legal to call on a queued group. Does NOT take a lock.
        void QueueForCompileNoLock(SingleDeviceShaderResourceGroup& group);

        // Un-queues the shader resource group for compile. Legal to call on an un-queued group. Takes a lock.
        void UnqueueForCompile(SingleDeviceShaderResourceGroup& shaderResourceGroup);

        // Compiles an SRG synchronously. 
        void Compile(SingleDeviceShaderResourceGroup& group, const SingleDeviceShaderResourceGroupData& groupData);

        // Calculate diffs for updating the resource registry.
        void CalculateGroupDataDiff(SingleDeviceShaderResourceGroup& shaderResourceGroup, const SingleDeviceShaderResourceGroupData& groupData);

        // Calculate the hash for all the views passed in
        template<typename T>
        HashValue64 GetViewHash(AZStd::span<const RHI::ConstPtr<T>> views);

        // Modify the m_rhiUpdateMask of a Srg if a view was modified in the current frame. This
        // will ensure that the view will be compiled by the back end
        template<typename T>
        void UpdateMaskBasedOnViewHash(
            SingleDeviceShaderResourceGroup& shaderResourceGroup,
            Name entryName,
            AZStd::span<const RHI::ConstPtr<T>> views,
            SingleDeviceShaderResourceGroupData::ResourceType resourceType);

        // Check all the resource types in order to ensure none of the views were invalidated or modified
        void ResetUpdateMaskForModifiedViews(
            SingleDeviceShaderResourceGroup& shaderResourceGroup, const SingleDeviceShaderResourceGroupData& shaderResourceGroupData);

        //////////////////////////////////////////////////////////////////////////
        // Platform API

        // Called when the pool initializes.
        virtual ResultCode InitInternal(Device& device, const ShaderResourceGroupPoolDescriptor& descriptor);

        // Initializes backing resources for the resource group.
        virtual ResultCode InitGroupInternal(SingleDeviceShaderResourceGroup& shaderResourceGroup);

        // Compiles a SingleDeviceShaderResourceGroup within the pool. Return true if the resource group was also resolved in
        // this method. If false, it will be queued for resolve in ResolveInternal.
        virtual ResultCode CompileGroupInternal(
            SingleDeviceShaderResourceGroup& shaderResourceGroup,
            const SingleDeviceShaderResourceGroupData& shaderResourceGroupData) = 0;

        //////////////////////////////////////////////////////////////////////////

        ShaderResourceGroupPoolDescriptor m_descriptor;
        bool m_hasConstants = false;
        bool m_hasBufferGroup = false;
        bool m_hasImageGroup = false;
        bool m_hasSamplerGroup = false;
        bool m_isCompiling = false;

        mutable AZStd::shared_mutex m_groupsToCompileMutex;
        AZStd::vector<SingleDeviceShaderResourceGroup*> m_groupsToCompile;

        AZStd::mutex m_invalidateRegistryMutex;
        ShaderResourceGroupInvalidateRegistry m_invalidateRegistry;
    };
}
