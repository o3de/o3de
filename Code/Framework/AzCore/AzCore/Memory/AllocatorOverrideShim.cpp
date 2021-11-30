/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/AllocatorOverrideShim.h>

namespace AZ
{
    namespace Internal
    {
        AllocatorOverrideShim* AllocatorOverrideShim::Create(IAllocator* owningAllocator, IAllocatorAllocate* shimAllocationSource)
        {
            void* memory = shimAllocationSource->Allocate(sizeof(AllocatorOverrideShim), AZStd::alignment_of<AllocatorOverrideShim>::value, 0);
            auto result = new (memory) AllocatorOverrideShim(owningAllocator, shimAllocationSource);
            return result;
        }

        void AllocatorOverrideShim::Destroy(AllocatorOverrideShim* source)
        {
            auto shimAllocationSource = source->m_shimAllocationSource;
            source->~AllocatorOverrideShim();
            shimAllocationSource->DeAllocate(source);
        }

        AllocatorOverrideShim::AllocatorOverrideShim(IAllocator* owningAllocator, IAllocatorAllocate* shimAllocationSource)
            : m_owningAllocator(owningAllocator)
            , m_source(owningAllocator->GetOriginalAllocationSource())
            , m_overridingSource(owningAllocator->GetOriginalAllocationSource())
            , m_shimAllocationSource(shimAllocationSource)
            , m_records(typename AllocationSet::hasher(), typename AllocationSet::key_eq(), StdAllocationSrc(shimAllocationSource))
        {
        }

        void AllocatorOverrideShim::SetOverride(IAllocatorAllocate* source)
        {
            m_overridingSource = source;
        }

        IAllocatorAllocate* AllocatorOverrideShim::GetOverride() const
        {
            return m_overridingSource;
        }

        bool AllocatorOverrideShim::IsOverridden() const
        {
            return m_source != m_overridingSource;
        }

        bool AllocatorOverrideShim::HasOrphanedAllocations() const
        {
            return !m_records.empty();
        }

        void AllocatorOverrideShim::SetFinalizedConfiguration()
        {
            m_finalizedConfiguration = true;
        }

        typename AllocatorOverrideShim::pointer_type AllocatorOverrideShim::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
        {
            pointer_type ptr = m_overridingSource->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord);

            if (!IsOverridden())
            {
                lock_type lock(m_mutex);
                m_records.insert(ptr);  // Record in case we need to orphan this allocation later
            }

            return ptr;
        }

        void AllocatorOverrideShim::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
        {
            IAllocatorAllocate* source = m_overridingSource;
            bool destroy = false;

            {
                lock_type lock(m_mutex);

                // Check to see if this came from a prior allocation source
                if (m_records.erase(ptr) && IsOverridden())
                {
                    source = m_source;

                    if (m_records.empty() && m_finalizedConfiguration)
                    {
                        // All orphaned records are gone; we are no longer needed
                        m_owningAllocator->SetAllocationSource(m_overridingSource);
                        destroy = true;  // Must destroy outside the lock
                    }
                }
            }

            source->DeAllocate(ptr, byteSize, alignment);

            if (destroy)
            {
                Destroy(this);
            }
        }

        typename AllocatorOverrideShim::size_type AllocatorOverrideShim::Resize(pointer_type ptr, size_type newSize)
        {
            IAllocatorAllocate* source = m_overridingSource;

            if (IsOverridden())
            {
                // Determine who owns the allocation
                lock_type lock(m_mutex);

                if (m_records.count(ptr))
                {
                    source = m_source;
                }
            }

            size_t result = source->Resize(ptr, newSize);

            return result;
        }

        typename AllocatorOverrideShim::pointer_type AllocatorOverrideShim::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
        {
            pointer_type newPtr = nullptr;
            bool useOverride = true;
            bool destroy = false;

            if (IsOverridden())
            {
                lock_type lock(m_mutex);

                if (m_records.erase(ptr))
                {
                    // An old allocation needs to be transferred to the new, overriding allocator.
                    useOverride = false;  // We'll do the reallocation here
                    size_t oldSize = m_source->AllocationSize(ptr);

                    if (newSize)
                    {
                        newPtr = m_overridingSource->Allocate(newSize, newAlignment, 0);
                        memcpy(newPtr, ptr, AZStd::min(newSize, oldSize));
                    }

                    m_source->DeAllocate(ptr, oldSize);

                    if (m_records.empty() && m_finalizedConfiguration)
                    {
                        // All orphaned records are gone; we are no longer needed
                        m_owningAllocator->SetAllocationSource(m_overridingSource);
                        destroy = true;  // Must destroy outside the lock
                    }
                }
            }

            if (useOverride)
            {
                // Default behavior, we weren't deleting an old allocation
                newPtr = m_overridingSource->ReAllocate(ptr, newSize, newAlignment);

                if (!IsOverridden())
                {
                    // Still need to do bookkeeping if we haven't been overridden yet
                    lock_type lock(m_mutex);
                    m_records.erase(ptr);
                    m_records.insert(newPtr);
                }
            }

            if (destroy)
            {
                Destroy(this);
            }

            return newPtr;
        }

        typename AllocatorOverrideShim::size_type AllocatorOverrideShim::AllocationSize(pointer_type ptr)
        {
            IAllocatorAllocate* source = m_overridingSource;

            if (IsOverridden())
            {
                // Determine who owns the allocation
                lock_type lock(m_mutex);

                if (m_records.count(ptr))
                {
                    source = m_source;
                }
            }

            return source->AllocationSize(ptr);
        }

        void AllocatorOverrideShim::GarbageCollect()
        {
            m_source->GarbageCollect();
        }

        typename AllocatorOverrideShim::size_type AllocatorOverrideShim::NumAllocatedBytes() const
        {
            return m_source->NumAllocatedBytes();
        }

        typename AllocatorOverrideShim::size_type AllocatorOverrideShim::Capacity() const
        {
            return m_source->Capacity();
        }

        typename AllocatorOverrideShim::size_type AllocatorOverrideShim::GetMaxAllocationSize() const
        {
            return m_source->GetMaxAllocationSize();
        }

        auto AllocatorOverrideShim::GetMaxContiguousAllocationSize() const -> size_type
        {
            return m_source->GetMaxContiguousAllocationSize();
        }

        IAllocatorAllocate* AllocatorOverrideShim::GetSubAllocator()
        {
            return m_source->GetSubAllocator();
        }

    }
}
