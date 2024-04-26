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
#if defined(CARBONATED)
            ~AllocatorEnvironmentVariable()
            {
                m_allocator.Reset();
            }
#endif
            
            Allocator& operator*() const
            {
                return *m_allocator;
            }

            EnvironmentVariable<Allocator> m_allocator;
        };

    public:
#if defined(CARBONATED)
        static bool HasAllocator()
        {
            return GetAllocatorPtr() != nullptr;
        }
        static IAllocator* GetAllocatorPtr()
        {
            static AllocatorEnvironmentVariable s_allocator;
            if (s_allocator.m_allocator.IsConstructed())
            {
                return &*s_allocator;
            }
            return nullptr;
        }
        static IAllocator& GetAllocator()
        {
            return *GetAllocatorPtr();
        }
#else
        static IAllocator& GetAllocator()
        {
            static AllocatorEnvironmentVariable s_allocator;
            return *s_allocator;
        }
#endif
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
        AZ_FORCE_INLINE static IAllocator& Get()
        {
            return StoragePolicy::GetAllocator();
        }
#if defined(CARBONATED)
        AZ_FORCE_INLINE static bool HasAllocator()
        {
            return StoragePolicy::HasAllocator();
        }
#endif
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
