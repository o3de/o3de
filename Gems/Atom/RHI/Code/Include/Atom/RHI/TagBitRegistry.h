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

namespace AZ::RHI
{
    //!
    //! A variant of TagRegistry that stores bit masks directly. The maximum number of tags is inferred from the number
    //! of bits available in TagType. Tags will always have a single bit flipped to 1. See TagRegistry for more details.
    //!
    template<typename IndexType>
    class TagBitRegistry final
        : public AZStd::intrusive_base
    {
    public:
        AZ_CLASS_ALLOCATOR(TagBitRegistry, AZ::SystemAllocator);
        AZ_DISABLE_COPY_MOVE(TagBitRegistry);

        using TagType = Handle<IndexType>;

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

        template <class TagVisitor>
        void VisitTags(TagVisitor visitor);

    private:
        TagBitRegistry() = default;
        static TagType ConvertToUnderlyingType(TagType tag);
        static TagType ConvertFromUnderlyingType(TagType tag);
        TagRegistry<IndexType, sizeof(IndexType) * 8> m_tagRegistry;
    };

    template<typename IndexType>
    Ptr<TagBitRegistry<IndexType>> TagBitRegistry<IndexType>::Create()
    {
        return aznew TagBitRegistry<IndexType>();
    }

    template<typename IndexType>
    auto TagBitRegistry<IndexType>::AcquireTag(const Name& tagName) -> TagType
    {
        return ConvertFromUnderlyingType(m_tagRegistry.AcquireTag(tagName));
    }

    template<typename IndexType>
    void TagBitRegistry<IndexType>::ReleaseTag(TagType tag)
    {
        m_tagRegistry.ReleaseTag(ConvertToUnderlyingType(tag));
    }

    template<typename IndexType>
    auto TagBitRegistry<IndexType>::FindTag(const Name& tagName) const -> TagType
    {
        return ConvertFromUnderlyingType(m_tagRegistry.FindTag(tagName));
    }

    template<typename IndexType>
    Name TagBitRegistry<IndexType>::GetName(TagType tag) const
    {
        return m_tagRegistry.GetName(ConvertToUnderlyingType(tag));
    }

    template<typename IndexType>
    auto TagBitRegistry<IndexType>::ConvertToUnderlyingType(TagType tag) -> TagType
    {
        return tag.IsValid() ? TagType(az_ctz_u64(tag.GetIndex())) : tag;
    }

    template<typename IndexType>
    auto TagBitRegistry<IndexType>::ConvertFromUnderlyingType(TagType tag) -> TagType
    {
        return tag.IsValid() ? TagType(1 << tag.GetIndex()) : tag;
    }

    template<typename IndexType>
    template<class TagVisitor>
    void TagBitRegistry<IndexType>::VisitTags(TagVisitor visitor)
    {
        auto tagConverter = [&](const Name& name, TagType tag)
        {
            visitor(name, ConvertFromUnderlyingType(tag));
        };
        m_tagRegistry.VisitTags(tagConverter);
    }
}
