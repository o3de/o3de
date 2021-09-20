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
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/ShaderResourceGroupInvalidateRegistry.h>
#include <Atom/RHI/ResourcePool.h>

#include <AzCore/std/parallel/containers/concurrent_vector.h>

namespace AZ
{
    namespace RHI
    {
         //! The platform-independent base class for ShaderResourceGroupPools. Platforms
         //! should inherit from this class to implement platform-dependent pooling of
         //! shader resource groups.
        class ShaderResourceGroupPool
            : public ResourcePool
        {
            friend class ShaderResourceGroup;
        public:
            AZ_RTTI(ShaderResourceGroupPool, "{9AAB5A85-4063-4BAE-9A9C-E25640F18FFA}", ResourcePool);
            virtual ~ShaderResourceGroupPool() override;

            //! Initializes the shader resource group pool.
            ResultCode Init(Device& device, const ShaderResourceGroupPoolDescriptor& descriptor);

            //! Initializes the resource group and associates it with the pool. The resource
            //! group must be updated on this pool.
            ResultCode InitGroup(ShaderResourceGroup& srg);

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
            ShaderResourceGroupPool();

            //////////////////////////////////////////////////////////////////////////
            // ResourcePool overrides
            void ShutdownInternal() override;
            void ShutdownResourceInternal(Resource& resource) override;
            //////////////////////////////////////////////////////////////////////////

        private:
            // Queues the shader resource group for compile and provides a new data packet (takes a lock).
            void QueueForCompile(ShaderResourceGroup& group, const ShaderResourceGroupData& groupData);

            // Queues the shader resource group for compile. Legal to call on a queued group. Takes a lock.
            void QueueForCompile(ShaderResourceGroup& group);

            // Queues the shader resource group for compile. Legal to call on a queued group. Does NOT take a lock.
            void QueueForCompileNoLock(ShaderResourceGroup& group);

            // Un-queues the shader resource group for compile. Legal to call on an un-queued group. Takes a lock.
            void UnqueueForCompile(ShaderResourceGroup& shaderResourceGroup);

            // Compiles an SRG synchronously. 
            void Compile(ShaderResourceGroup& group, const ShaderResourceGroupData& groupData);

            // Calculate diffs for updating the resource registry.
            void CalculateGroupDataDiff(ShaderResourceGroup& shaderResourceGroup, const ShaderResourceGroupData& groupData);
          
            //////////////////////////////////////////////////////////////////////////
            // Platform API

            // Called when the pool initializes.
            virtual ResultCode InitInternal(Device& device, const ShaderResourceGroupPoolDescriptor& descriptor);

            // Initializes backing resources for the resource group.
            virtual ResultCode InitGroupInternal(ShaderResourceGroup& shaderResourceGroup);

            // Compiles a ShaderResourceGroup within the pool. Return true if the resource group was also resolved in
            // this method. If false, it will be queued for resolve in ResolveInternal.
            virtual ResultCode CompileGroupInternal(
                ShaderResourceGroup& shaderResourceGroup,
                const ShaderResourceGroupData& shaderResourceGroupData) = 0;

            //////////////////////////////////////////////////////////////////////////

            ShaderResourceGroupPoolDescriptor m_descriptor;
            bool m_hasConstants = false;
            bool m_hasBufferGroup = false;
            bool m_hasImageGroup = false;
            bool m_hasSamplerGroup = false;
            bool m_isCompiling = false;

            mutable AZStd::shared_mutex m_groupsToCompileMutex;
            AZStd::vector<ShaderResourceGroup*> m_groupsToCompile;

            AZStd::mutex m_invalidateRegistryMutex;
            ShaderResourceGroupInvalidateRegistry m_invalidateRegistry;
        };
    }
}
