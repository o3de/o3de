/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <Atom/RHI/TagRegistry.h>
#include <Atom/RHI/TagBitRegistry.h>
#include <Atom/RHI.Reflect/Handle.h>

namespace UnitTest
{
    using namespace AZ;

    class TagRegistryTest
        : public RHITestFixture
    {
    public:
        TagRegistryTest()
            : RHITestFixture()
        {}

        static constexpr uint8_t Count = 4;
        using IndexType = uint16_t;
        using TagType = RHI::Handle<IndexType>;

        // Make sure values in two arrays of tags are unique
        void CheckUniqueness(AZStd::array<TagType, Count>& tags)
        {
            for (uint8_t i = 0; i < Count; ++i)
            {
                for (uint8_t j = i + 1; j < Count; ++j)
                {
                    EXPECT_NE(tags[i], tags[j]);
                }
            }
        }

        template <typename T>
        void CheckNamesAndtags(AZStd::array<Name, Count>& names, AZStd::array<TagType, Count>& tags, T& registry)
        {
            // Check to see if they can be retrieved after acquired
            for (uint8_t i = 0; i < Count; ++i)
            {
                EXPECT_EQ(tags[i], registry->FindTag(names[i]));
            }

            // Make sure getting names of the tags works correctly.
            for (uint8_t i = 0; i < Count; ++i)
            {
                EXPECT_EQ(registry->GetName(tags[i]), names[i]);
            }
        }
    };


    TEST_F(TagRegistryTest, ConstructResetTagRegistry)
    {
        auto tagRegistry = RHI::TagRegistry<IndexType, 32>::Create();
        EXPECT_EQ(tagRegistry->GetAllocatedTagCount(), 0);
        [[maybe_unused]] auto tagA = tagRegistry->AcquireTag(Name("A"));
        EXPECT_EQ(tagRegistry->GetAllocatedTagCount(), 1);
        tagRegistry->Reset();
        EXPECT_EQ(tagRegistry->GetAllocatedTagCount(), 0);
    }

    TEST_F(TagRegistryTest, TagValues)
    {
        auto tagRegistry = RHI::TagRegistry<IndexType, Count>::Create();

        AZStd::array<Name, Count> names = { Name("A"), Name("B"), Name("C"), Name("D"), };
        AZStd::array<TagType, Count> tags;
        for (uint8_t i = 0; i < Count; ++i)
        {
            tags[i] = tagRegistry->AcquireTag(names[i]);
        }

        CheckUniqueness(tags);

        // Make sure they're less than max
        for (uint8_t i = 0; i < Count; ++i)
        {
            EXPECT_LT(tags[i].GetIndex(), Count);
        }

        CheckNamesAndtags(names, tags, tagRegistry);

        // Check to make sure releasing frees tag
        tagRegistry->ReleaseTag(tags[1]);
        EXPECT_EQ(tagRegistry->FindTag(names[1]), TagType::Null);

        // Check acquiring new tag after release keeps uniqueness property
        tags[1] = tagRegistry->AcquireTag(names[1]);
        CheckUniqueness(tags);

        // Release all tags
        for (uint8_t i = 0; i < Count; ++i)
        {
            tagRegistry->ReleaseTag(tags[i]);
        }
    }

    TEST_F(TagRegistryTest, RefCounting)
    {
        auto tagRegistry = RHI::TagRegistry<IndexType, Count>::Create();

        const uint32_t refCount = 4;
        TagType tagA;
        TagType tagB;

        // acquire refs
        for (uint32_t i = 0; i < refCount; ++i)
        {
            tagA = tagRegistry->AcquireTag(Name("A"));
            tagB = tagRegistry->AcquireTag(Name("B"));
        }

        // release refs, but make sure they exist until all refs released
        for (uint32_t i = 0; i < refCount; ++i)
        {
            EXPECT_EQ(tagA, tagRegistry->FindTag(Name("A")));
            EXPECT_EQ(tagB, tagRegistry->FindTag(Name("B")));
            tagRegistry->ReleaseTag(tagA);
            tagRegistry->ReleaseTag(tagB);
        }

        // should no longer exist.
        EXPECT_EQ(tagRegistry->FindTag(Name("A")), TagType::Null);
        EXPECT_EQ(tagRegistry->FindTag(Name("B")), TagType::Null);
    }

    TEST_F(TagRegistryTest, VisitTagRegistry)
    {
        auto tagRegistry = RHI::TagRegistry<IndexType, Count>::Create();

        AZStd::array<Name, Count> names = { Name("A"), Name("B"), Name("C"), Name("D"), };
        AZStd::array<TagType, Count> tags;
        for (uint8_t i = 0; i < Count; ++i)
        {
            tags[i] = tagRegistry->AcquireTag(names[i]);
        }

        uint32_t visitCount = 0;
        auto visitor = [&](const AZ::Name& name, TagType tag)
        {
            ++visitCount;
            EXPECT_EQ(tagRegistry->GetName(tag), name);
        };

        tagRegistry->VisitTags(visitor);
        EXPECT_EQ(visitCount, Count);

        tagRegistry->ReleaseTag(tags[1]);
        visitCount = 0;
        tagRegistry->VisitTags(visitor);
        EXPECT_EQ(visitCount, Count - 1);
    }

    TEST_F(TagRegistryTest, ConstructResetTagBitRegistry)
    {
        auto tagBitRegistry = RHI::TagBitRegistry<IndexType>::Create();
        EXPECT_EQ(tagBitRegistry->GetAllocatedTagCount(), 0);
        [[maybe_unused]] auto tagA = tagBitRegistry->AcquireTag(Name("A"));
        EXPECT_EQ(tagBitRegistry->GetAllocatedTagCount(), 1);
        tagBitRegistry->Reset();
        EXPECT_EQ(tagBitRegistry->GetAllocatedTagCount(), 0);
    }

    TEST_F(TagRegistryTest, TagBitValues)
    {
        auto tagBitRegistry = RHI::TagBitRegistry<IndexType>::Create();

        AZStd::array<Name, Count> names = { Name("A"), Name("B"), Name("C"), Name("D"), };
        AZStd::array<TagType, Count> tags;
        for (uint8_t i = 0; i < Count; ++i)
        {
            tags[i] = tagBitRegistry->AcquireTag(names[i]);
        }

        CheckUniqueness(tags);

        // Make sure each tag is a single bit.
        for (uint8_t i = 0; i < Count; ++i)
        {
            EXPECT_EQ(az_popcnt_u32(tags[i].GetIndex()), 1);
        }

        CheckNamesAndtags(names, tags, tagBitRegistry);

        // Check to make sure releasing frees tag
        tagBitRegistry->ReleaseTag(tags[1]);
        EXPECT_EQ(tagBitRegistry->FindTag(names[1]), TagType::Null);

        // Check acquiring new tag after release keeps uniqueness property
        tags[1] = tagBitRegistry->AcquireTag(names[1]);
        CheckUniqueness(tags);

        // Release all tags
        for (uint8_t i = 0; i < Count; ++i)
        {
            tagBitRegistry->ReleaseTag(tags[i]);
        }
    }

    TEST_F(TagRegistryTest, VisitTagBitRegistry)
    {
        auto tagBitRegistry = RHI::TagBitRegistry<IndexType>::Create();

        AZStd::array<Name, Count> names = { Name("A"), Name("B"), Name("C"), Name("D"), };
        AZStd::array<TagType, Count> tags;
        for (uint8_t i = 0; i < Count; ++i)
        {
            tags[i] = tagBitRegistry->AcquireTag(names[i]);
        }

        uint32_t visitCount = 0;
        auto visitor = [&](const AZ::Name& name, TagType tag)
        {
            ++visitCount;
            EXPECT_EQ(tagBitRegistry->GetName(tag), name);
        };

        tagBitRegistry->VisitTags(visitor);
        EXPECT_EQ(visitCount, Count);

        tagBitRegistry->ReleaseTag(tags[1]);
        visitCount = 0;
        tagBitRegistry->VisitTags(visitor);
        EXPECT_EQ(visitCount, Count - 1);
    }

}
