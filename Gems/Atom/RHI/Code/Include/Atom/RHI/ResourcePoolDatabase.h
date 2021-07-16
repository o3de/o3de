/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace RHI
    {
        class ResourcePool;
        class ResourcePoolResolver;
        class BufferPoolBase;
        class ImagePoolBase;
        class ShaderResourceGroupPool;

        /**
         * This class is a simple database of active resource pools. Resource pools
         * are attached and detached from the database when they initialize and shutdown,
         * respectively. The database provides a way to iterate over active pools in
         * a thread-safe way using a reader-writer lock.
         *
         * ResourcePool is friended to the class in order to allow it to control
         * attachment / detachment from the database.
         */
        class ResourcePoolDatabase final
        {
        public:
            ResourcePoolDatabase() = default;
            ResourcePoolDatabase(const ResourcePoolDatabase&) = delete;
            ~ResourcePoolDatabase();

            /**
             * Provides a read-locked loop over the set of buffer pools.
             * @param predicate The predicate to call for each instance. Expected signature: void(const BufferPoolBase*).
             */
            template <typename Predicate>
            void ForEachBufferPool(Predicate predicate) const;

            /**
             * Provides a read-locked loop over the set of buffer pools.
             * @param predicate The predicate to call for each instance. Expected signature: void(BufferPoolBase*).
             */
            template <typename Predicate>
            void ForEachBufferPool(Predicate predicate);

            /**
             * Provides a read-locked loop over the set of image pools.
             * @param predicate The predicate to call for each instance. Expected signature: void(const ImagePoolBase*).
             */
            template <typename Predicate>
            void ForEachImagePool(Predicate predicate) const;

            /**
             * Provides a read-locked loop over the set of image pools.
             * @param predicate The predicate to call for each instance. Expected signature: void(ImagePoolBase*).
             */
            template <typename Predicate>
            void ForEachImagePool(Predicate predicate);

            /**
             * Provides a read-locked loop over the set of shader resource group pools.
             * @param predicate The predicate to call for each instance. Expected signature: void(const ShaderResourceGroupPool*).
             */
            template <typename Predicate>
            void ForEachShaderResourceGroupPool(Predicate predicate) const;

            /**
             * Provides a read-locked loop over the set of shader resource group pools.
             * @param predicate The predicate to call for each instance. Expected signature: void(ShaderResourceGroupPool*).
             */
            template <typename Predicate>
            void ForEachShaderResourceGroupPool(Predicate predicate);

            /**
             * Provides a read-locked loop over the set resource pools.
             * @param predicate The predicate to call for each instance. Expected signature: void(const ResourcePool*).
             */
            template <typename Predicate>
            void ForEachPool(Predicate predicate) const;

            /**
             * Provides a read-locked loop over the set of resource pools.
             * @param predicate The predicate to call for each instance. Expected signature: void(ResourcePool*).
             */
            template <typename Predicate>
            void ForEachPool(Predicate predicate);

            /**
             * Provides a read-locked loop over the set resource pool resolvers.
             * @param predicate The predicate to call for each instance. Expected signature: void(const ResourcePoolResolver*).
             */
            template <typename Predicate>
            void ForEachPoolResolver(Predicate predicate) const;

            /**
             * Provides a read-locked loop over the set of resource pool resolvers.
             * @param predicate The predicate to call for each instance. Expected signature: void(ResourcePoolResolver*).
             */
            template <typename Predicate>
            void ForEachPoolResolver(Predicate predicate);

        private:
            friend class ResourcePool;
            void AttachPool(ResourcePool* resourcePool);
            void DetachPool(ResourcePool* resourcePool);

            mutable AZStd::shared_mutex m_mutex;
            AZStd::vector<ResourcePool*> m_pools;
            AZStd::vector<BufferPoolBase*> m_bufferPools;
            AZStd::vector<ImagePoolBase*> m_imagePools;
            AZStd::vector<ShaderResourceGroupPool*> m_shaderResourceGroupPools;
            AZStd::vector<ResourcePoolResolver*> m_poolResolvers;
        };

        template <typename Predicate>
        void ResourcePoolDatabase::ForEachBufferPool(Predicate predicate) const
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_mutex);
            AZStd::for_each(m_bufferPools.begin(), m_bufferPools.end(), predicate);
        }

        template <typename Predicate>
        void ResourcePoolDatabase::ForEachBufferPool(Predicate predicate)
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_mutex);
            AZStd::for_each(m_bufferPools.begin(), m_bufferPools.end(), predicate);
        }

        template <typename Predicate>
        void ResourcePoolDatabase::ForEachImagePool(Predicate predicate) const
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_mutex);
            AZStd::for_each(m_imagePools.begin(), m_imagePools.end(), predicate);
        }

        template <typename Predicate>
        void ResourcePoolDatabase::ForEachImagePool(Predicate predicate)
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_mutex);
            AZStd::for_each(m_imagePools.begin(), m_imagePools.end(), predicate);
        }

        template <typename Predicate>
        void ResourcePoolDatabase::ForEachShaderResourceGroupPool(Predicate predicate) const
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_mutex);
            AZStd::for_each(m_shaderResourceGroupPools.begin(), m_shaderResourceGroupPools.end(), predicate);
        }

        template <typename Predicate>
        void ResourcePoolDatabase::ForEachShaderResourceGroupPool(Predicate predicate)
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_mutex);
            AZStd::for_each(m_shaderResourceGroupPools.begin(), m_shaderResourceGroupPools.end(), predicate);
        }

        template <typename Predicate>
        void ResourcePoolDatabase::ForEachPool(Predicate predicate) const
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_mutex);
            AZStd::for_each(m_pools.begin(), m_pools.end(), predicate);
        }

        template <typename Predicate>
        void ResourcePoolDatabase::ForEachPool(Predicate predicate)
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_mutex);
            AZStd::for_each(m_pools.begin(), m_pools.end(), predicate);
        }

        template <typename Predicate>
        void ResourcePoolDatabase::ForEachPoolResolver(Predicate predicate) const
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_mutex);
            AZStd::for_each(m_poolResolvers.begin(), m_poolResolvers.end(), predicate);
        }

        template <typename Predicate>
        void ResourcePoolDatabase::ForEachPoolResolver(Predicate predicate)
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_mutex);
            AZStd::for_each(m_poolResolvers.begin(), m_poolResolvers.end(), predicate);
        }
    }
}
