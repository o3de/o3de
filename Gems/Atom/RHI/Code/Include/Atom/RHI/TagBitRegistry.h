/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/MathIntrinsics.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <Atom/RHI/TagRegistry.h>

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
        template<typename TagType>
        class TagBitRegistry final
            : public AZStd::intrusive_base
        {
        public:
            AZ_CLASS_ALLOCATOR(TagBitRegistry, AZ::SystemAllocator, 0);
            AZ_DISABLE_COPY_MOVE(TagBitRegistry);

            static Ptr<TagBitRegistry> Create();

            //! Resets the registry back to an empty state. All references are released.
            void Reset() { m_tagRegistry.Reset(); };

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
            size_t GetAllocatedTagCount() const { return m_tagRegistry.GetAllocatedTagCount(); };

        private:
            TagBitRegistry() = default;
            static TagType ConvertToUnderlyingType(TagType tag);
            TagRegistry<typename TagType, sizeof(TagType) * 8> m_tagRegistry;
        };

        template<typename TagType>
        Ptr<TagBitRegistry<TagType>> TagBitRegistry<TagType>::Create()
        {
            return aznew TagBitRegistry<TagType>();
        }

        template<typename TagType>
        TagType TagBitRegistry<TagType>::AcquireTag(const Name& tagName)
        {
            if (TagType bitPos = m_tagRegistry.AcquireTag(tagName); bitPos.IsValid())
            {
                return TagType(1 << bitPos.GetIndex());
            }
            return {};
        }

        template<typename TagType>
        void TagBitRegistry<TagType>::ReleaseTag(TagType tag)
        {
            m_tagRegistry.ReleaseTag(ConvertToUnderlyingType(tag));
        }

        template<typename TagType>
        TagType TagBitRegistry<TagType>::FindTag(const Name& tagName) const
        {
            if (TagType bitPos = m_tagRegistry.FindTag(tagName); bitPos.IsValid())
            {
                return TagType(1 << m_tagRegistry.FindTag(tagName).GetIndex());
            }
            return {};
        }

        template<typename TagType>
        Name TagBitRegistry<TagType>::GetName(TagType tag) const
        {
            return m_tagRegistry.GetName(ConvertToUnderlyingType(tag));
        }

        template<typename TagType>
        TagType TagBitRegistry<TagType>::ConvertToUnderlyingType(TagType tag)
        {
            return TagType(az_ctz_u64(tag.GetIndex()));
        }
    }
}
