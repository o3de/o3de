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
    };


    TEST_F(TagRegistryTest, ConstructResetTagRegistry)
    {
        auto tagRegistry = RHI::TagRegistry<RHI::Handle<uint16_t>, 32>::Create();
        EXPECT_EQ(tagRegistry->GetAllocatedTagCount(), 0);
        auto tagA = tagRegistry->AcquireTag(Name("A"));
        EXPECT_EQ(tagRegistry->GetAllocatedTagCount(), 1);
        tagRegistry->Reset();
        EXPECT_EQ(tagRegistry->GetAllocatedTagCount(), 0);
    }

    TEST_F(TagRegistryTest, TagValues)
    {
        auto tagRegistry = RHI::TagRegistry<RHI::Handle<uint16_t>, 32>::Create();
        auto tagA = tagRegistry->AcquireTag(Name("A"));
        auto tagB = tagRegistry->AcquireTag(Name("B"));
        auto tagC = tagRegistry->AcquireTag(Name("C"));
        auto tagD = tagRegistry->AcquireTag(Name("D"));

        // Make sure values increment
        EXPECT_EQ(tagA.GetIndex(), 0);
        EXPECT_EQ(tagB.GetIndex(), 1);
        EXPECT_EQ(tagC.GetIndex(), 2);
        EXPECT_EQ(tagD.GetIndex(), 3);

        // Check to see if they can be retrieved after acquired
        EXPECT_EQ(tagA, tagRegistry->FindTag(Name("A")));
        EXPECT_EQ(tagB, tagRegistry->FindTag(Name("B")));
        EXPECT_EQ(tagC, tagRegistry->FindTag(Name("C")));
        EXPECT_EQ(tagD, tagRegistry->FindTag(Name("D")));

        // Make sure getting names of the tags works correctly.
        EXPECT_EQ(tagRegistry->GetName(tagA), Name("A"));
        EXPECT_EQ(tagRegistry->GetName(tagB), Name("B"));
        EXPECT_EQ(tagRegistry->GetName(tagC), Name("C"));
        EXPECT_EQ(tagRegistry->GetName(tagD), Name("D"));

        // Check to make sure releasing and acquiring fills a hole
        tagRegistry->ReleaseTag(tagB);
        EXPECT_EQ(tagRegistry->FindTag(Name("B")), AZ::RHI::Handle<uint16_t>::Null);
        auto tagBNew = tagRegistry->AcquireTag(Name("B"));
        EXPECT_EQ(tagB, tagBNew);

        // Check the next after all holes are filled is the next valid integer.
        auto tagE = tagRegistry->AcquireTag(Name("E"));
        EXPECT_EQ(tagE.GetIndex(), 4);

    }

    TEST_F(TagRegistryTest, RefCounting)
    {
        auto tagRegistry = RHI::TagRegistry<RHI::Handle<uint16_t>, 32>::Create();

        const uint32_t refCount = 4;
        AZ::RHI::Handle<uint16_t> tagA;
        AZ::RHI::Handle<uint16_t> tagB;

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
        EXPECT_EQ(tagRegistry->FindTag(Name("A")), AZ::RHI::Handle<uint16_t>::Null);
        EXPECT_EQ(tagRegistry->FindTag(Name("B")), AZ::RHI::Handle<uint16_t>::Null);
    }

    TEST_F(TagRegistryTest, ConstructResetTagBitRegistry)
    {
        auto tagBitRegistry = RHI::TagBitRegistry<RHI::Handle<uint16_t>>::Create();
        EXPECT_EQ(tagBitRegistry->GetAllocatedTagCount(), 0);
        auto tagA = tagBitRegistry->AcquireTag(Name("A"));
        EXPECT_EQ(tagBitRegistry->GetAllocatedTagCount(), 1);
        tagBitRegistry->Reset();
        EXPECT_EQ(tagBitRegistry->GetAllocatedTagCount(), 0);
    }

    TEST_F(TagRegistryTest, TagBitValues)
    {
        auto tagBitRegistry = RHI::TagBitRegistry<RHI::Handle<uint16_t>>::Create();
        auto tagA = tagBitRegistry->AcquireTag(Name("A"));
        auto tagB = tagBitRegistry->AcquireTag(Name("B"));
        auto tagC = tagBitRegistry->AcquireTag(Name("C"));
        auto tagD = tagBitRegistry->AcquireTag(Name("D"));

        // Make sure values increment bit positions
        EXPECT_EQ(tagA.GetIndex(), 0b0001);
        EXPECT_EQ(tagB.GetIndex(), 0b0010);
        EXPECT_EQ(tagC.GetIndex(), 0b0100);
        EXPECT_EQ(tagD.GetIndex(), 0b1000);

        // Check to see if they can be retrieved after acquired
        EXPECT_EQ(tagA, tagBitRegistry->FindTag(Name("A")));
        EXPECT_EQ(tagB, tagBitRegistry->FindTag(Name("B")));
        EXPECT_EQ(tagC, tagBitRegistry->FindTag(Name("C")));
        EXPECT_EQ(tagD, tagBitRegistry->FindTag(Name("D")));

        // Make sure getting names of the tags works correctly.
        EXPECT_EQ(tagBitRegistry->GetName(tagA), Name("A"));
        EXPECT_EQ(tagBitRegistry->GetName(tagB), Name("B"));
        EXPECT_EQ(tagBitRegistry->GetName(tagC), Name("C"));
        EXPECT_EQ(tagBitRegistry->GetName(tagD), Name("D"));

        // Check to make sure releasing and acquiring fills a hole
        tagBitRegistry->ReleaseTag(tagB);
        EXPECT_EQ(tagBitRegistry->FindTag(Name("B")), AZ::RHI::Handle<uint16_t>::Null);
        auto tagBNew = tagBitRegistry->AcquireTag(Name("B"));
        EXPECT_EQ(tagB, tagBNew);

        // Check the next after all holes are filled is the next valid integer.
        auto tagE = tagBitRegistry->AcquireTag(Name("E"));
        EXPECT_EQ(tagE.GetIndex(), 0b10000);
    }
}
