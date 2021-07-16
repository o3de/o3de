/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Name/Name.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/std/parallel/shared_mutex.h>

namespace AZ
{
    namespace RHI
    {
        //! 
        //! Allocates and registers tags by name, allowing the user to acquire and find tags from names.
        //! The class is designed to map user-friendly tag names defined through content or higher level code to
        //! low-level tags, which are simple handles.
        //! 
        //! Some notes about usage and design:
        //!   - TagType need to be a Handle<Integer> type.
        //!   - Tags are reference counted, which means multiple calls to 'Acquire' with the same name will increment 
        //!     the internal reference count on the tag. This allows shared ownership between systems, if necessary.
        //!   - FindTag is provided to search for a tag reference without taking ownership.
        //!   - Names are case sensitive.
        //!
        template<typename TagType, size_t MaxTagCount>
        class TagRegistry final
            : public AZStd::intrusive_base
        {
        public:
            AZ_CLASS_ALLOCATOR(TagRegistry, AZ::SystemAllocator, 0);
            AZ_DISABLE_COPY_MOVE(TagRegistry);

            static Ptr<TagRegistry> Create();

            //! Resets the registry back to an empty state. All references are released.
            void Reset();

            //! Acquires a tag from the provided name (case sensitive). If the tag already existed, it is ref-counted.
            //! Returns a valid tag on success; returns a null tag if the registry is at full capacity. You must
            //! call ReleaseTag() if successful.
            TagType AcquireTag(const Name& tagName);

            //! Releases a reference to a tag. Tags are ref-counted, so it's necessary to maintain ownership of the
            //! tag and release when its no longer needed.
            void ReleaseTag(TagType tagName);

           //! Finds the tag associated with the provided name (case sensitive). If a tag exists with that name, the tag
           //! is returned. The reference count is NOT incremented on success; ownership is not passed to the user. If
           //! the tag does not exist, a null tag is returned.
            TagType FindTag(const Name& tagName) const;

            //! Returns the name of the given tag, or empty string if the tag is not registered.
            Name GetName(TagType tag) const;

            //! Returns the number of allocated tags in the registry.
            size_t GetAllocatedTagCount() const;

        private:
            TagRegistry() = default;

            struct Entry
            {
                Name m_name;
                size_t m_refCount = 0;
            };

            mutable AZStd::shared_mutex m_mutex;
            AZStd::array<Entry, MaxTagCount> m_entriesByTag;
            size_t m_allocatedTagCount = 0;
        };

        template<typename TagType, size_t MaxTagCount>
        Ptr<TagRegistry<TagType, MaxTagCount>> TagRegistry<TagType, MaxTagCount>::Create()
        {
            return aznew TagRegistry<TagType, MaxTagCount>();
        }

        template<typename TagType, size_t MaxTagCount>
        void TagRegistry<TagType, MaxTagCount>::Reset()
        {
            AZStd::unique_lock<AZStd::shared_mutex> lock(m_mutex);
            m_entriesByTag.fill({});
            m_allocatedTagCount = 0;
        }

        template<typename TagType, size_t MaxTagCount>
        TagType TagRegistry<TagType, MaxTagCount>::AcquireTag(const Name& tagName)
        {
            if (tagName.IsEmpty())
            {
                return {};
            }

            TagType tag;
            Entry* foundEmptyEntry = nullptr;

            AZStd::unique_lock<AZStd::shared_mutex> lock(m_mutex);
            for (size_t i = 0; i < m_entriesByTag.size(); ++i)
            {
                Entry& entry = m_entriesByTag[i];

                // Found an empty entry. Cache off the tag and pointer, but keep searching to find if
                // another entry holds the same name.
                if (entry.m_refCount == 0 && !foundEmptyEntry)
                {
                    foundEmptyEntry = &entry;
                    tag = TagType(i);
                }
                else if (entry.m_name == tagName)
                {
                    entry.m_refCount++;
                    return TagType(i);
                }
            }

            // No other entry holds the name, so allocate the empty entry.
            if (foundEmptyEntry)
            {
                foundEmptyEntry->m_refCount = 1;
                foundEmptyEntry->m_name = tagName;
                ++m_allocatedTagCount;
            }

            return tag;
        }

        template<typename TagType, size_t MaxTagCount>
        void TagRegistry<TagType, MaxTagCount>::ReleaseTag(TagType tag)
        {
            if (tag.IsValid())
            {
                AZStd::unique_lock<AZStd::shared_mutex> lock(m_mutex);
                Entry& entry = m_entriesByTag[tag.GetIndex()];
                const size_t refCount = --entry.m_refCount;
                AZ_Assert(
                    refCount != static_cast<size_t>(-1), "Attempted to forfeit a tag that is not valid. Tag{%d},Name{'%s'}", tag,
                    entry.m_name.GetCStr());
                if (refCount == 0)
                {
                    entry.m_name = Name();
                    --m_allocatedTagCount;
                }
            }
        }

        template<typename TagType, size_t MaxTagCount>
        TagType TagRegistry<TagType, MaxTagCount>::FindTag(const Name& tagName) const
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_mutex);
            for (size_t i = 0; i < m_entriesByTag.size(); ++i)
            {
                if (m_entriesByTag[i].m_name == tagName)
                {
                    return TagType(i);
                }
            }
            return {};
        }

        template<typename TagType, size_t MaxTagCount>
        Name TagRegistry<TagType, MaxTagCount>::GetName(TagType tag) const
        {
            if (tag.GetIndex() < m_entriesByTag.size())
            {
                return m_entriesByTag[tag.GetIndex()].m_name;
            }
            else
            {
                return Name();
            }
        }

        template<typename TagType, size_t MaxTagCount>
        size_t TagRegistry<TagType, MaxTagCount>::GetAllocatedTagCount() const
        {
            return m_allocatedTagCount;
        }
    }
}
