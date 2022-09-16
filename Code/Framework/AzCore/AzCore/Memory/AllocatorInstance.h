/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Environment.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ { class IAllocator; }

namespace AZ::AllocatorStorage
{
    /**
    * EnvironmentStoragePolicy stores the allocator singleton in the shared Environment.
    * This is the default, preferred method of storing allocators.
    */
    template<class Allocator>
    class EnvironmentStoragePolicy
    {
        class AllocatorEnvironmentVariable
        {
        public:
            AllocatorEnvironmentVariable()
                : m_allocator(Environment::FindVariable<Allocator>(AzTypeInfo<Allocator>::Name()))
            {
                if (!m_allocator)
                {
                    m_allocator = Environment::CreateVariable<Allocator>(AzTypeInfo<Allocator>::Name());
                }
            }

            Allocator& operator*() const
            {
                return *m_allocator;
            }

        private:
            EnvironmentVariable<Allocator> m_allocator;
        };

    public:
        static IAllocator& GetAllocator()
        {
            static AllocatorEnvironmentVariable s_allocator;
            return *s_allocator;
        }

        static void Create()
        {
            GetAllocator();
        }

        static void Destroy()
        {
        }

        AZ_FORCE_INLINE static bool IsReady()
        {
            return true;
        }
    };
} // namespace AZ::AllocatorStorage

namespace AZ::Internal
{
    /**
    * The main class that provides access to the allocator singleton, with a customizable storage policy for that allocator.
    */
    template<class Allocator, typename StoragePolicy = AllocatorStorage::EnvironmentStoragePolicy<Allocator>>
    class AllocatorInstanceBase
    {
    public:
        // Maintained for backwards compatibility, prefer to use Get() instead.
        // Get was previously used to get the the schema, however, that bypasses what the allocators are doing.
        // If the schema is needed, call Get().GetSchema()
        AZ_FORCE_INLINE static IAllocator& GetAllocator()
        {
            return StoragePolicy::GetAllocator();
        }

        AZ_FORCE_INLINE static IAllocator& Get()
        {
            return StoragePolicy::GetAllocator();
        }

        static void Create()
        {
            StoragePolicy::Create();
        }

        static void Destroy()
        {
            StoragePolicy::Destroy();
        }

        AZ_FORCE_INLINE static bool IsReady()
        {
            return StoragePolicy::IsReady();
        }
    };
} // namespace AZ::Internal

namespace AZ
{
    /**
     * Standard allocator singleton, using Environment storage. Specialize this for your
     * allocator if you need to control storage or lifetime, by changing the policy class
     * used in AllocatorInstanceBase.
     *
     * It is preferred that you don't do a complete specialization of AllocatorInstance,
     * as the logic governing creation and destruction of allocators is complicated and
     * susceptible to edge cases across all platforms and build types, and it is best to
     * keep the allocator code flowing through a consistent codepath.
     */
    template<class Allocator>
    class AllocatorInstance : public Internal::AllocatorInstanceBase<Allocator, AllocatorStorage::EnvironmentStoragePolicy<Allocator>>
    {
    };
} // namespace AZ
