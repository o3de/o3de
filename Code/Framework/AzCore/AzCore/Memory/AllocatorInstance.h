/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Environment.h>
#include <AzCore/Memory/IAllocator.h>

namespace AZ::AllocatorStorage
{
    /**
    * A base class for all storage policies. This exists to provide access to private IAllocator methods via template friends.
    */
    template<class Allocator>
    class StoragePolicyBase
    {
    protected:
        static void Create(Allocator& allocator, const typename Allocator::Descriptor& desc, bool lazilyCreated)
        {
            allocator.Create(desc);
            allocator.PostCreate();
            allocator.SetLazilyCreated(lazilyCreated);
        }

        static void SetLazilyCreated(Allocator& allocator, bool lazilyCreated)
        {
            allocator.SetLazilyCreated(lazilyCreated);
        }

        static void Destroy(IAllocator& allocator)
        {
            allocator.PreDestroy();
            allocator.Destroy();
        }
    };

    /**
    * EnvironmentStoragePolicy stores the allocator singleton in the shared Environment.
    * This is the default, preferred method of storing allocators.
    */
    template<class Allocator>
    class EnvironmentStoragePolicy : public StoragePolicyBase<Allocator>
    {
    public:
        static IAllocator& GetAllocator()
        {
            if (!s_allocator)
            {
                // Assert here before attempting to resolve. Otherwise a module-local
                // environment will be created which will result in a much more difficult to
                // locate problem
                s_allocator = Environment::FindVariable<Allocator>(AzTypeInfo<Allocator>::Name());
                AZ_Assert(s_allocator, "Allocator '%s' NOT ready for use! Call Create first!", AzTypeInfo<Allocator>::Name());
            }
            return *s_allocator;
        }

        static void Create(const typename Allocator::Descriptor& desc)
        {
            if (!s_allocator)
            {
                s_allocator = Environment::CreateVariable<Allocator>(AzTypeInfo<Allocator>::Name());
                if (s_allocator->IsReady()) // already created in a different module
                {
                    return;
                }
            }
            else
            {
                AZ_Assert(s_allocator->IsReady(), "Allocator '%s' already created!", AzTypeInfo<Allocator>::Name());
            }

            StoragePolicyBase<Allocator>::Create(*s_allocator, desc, false);
        }

        static void Destroy()
        {
            if (s_allocator)
            {
                if (s_allocator.IsOwner())
                {
                    StoragePolicyBase<Allocator>::Destroy(*s_allocator);
                }
                s_allocator = nullptr;
            }
            else
            {
                AZ_Assert(false, "Allocator '%s' NOT ready for use! Call Create first!", AzTypeInfo<Allocator>::Name());
            }
        }

        AZ_FORCE_INLINE static bool IsReady()
        {
            if (!s_allocator)
            {
                s_allocator = Environment::FindVariable<Allocator>(AzTypeInfo<Allocator>::Name());
            }
            return s_allocator && s_allocator->IsReady();
        }

        static EnvironmentVariable<Allocator> s_allocator;
    };

    template<class Allocator>
    EnvironmentVariable<Allocator> EnvironmentStoragePolicy<Allocator>::s_allocator;
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
        using Descriptor = typename Allocator::Descriptor;

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

        static void Create(const Descriptor& desc = Descriptor())
        {
            StoragePolicy::Create(desc);
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
