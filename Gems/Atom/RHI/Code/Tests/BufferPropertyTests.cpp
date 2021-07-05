/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <Atom/RHI/BufferProperty.h>

namespace UnitTest
{
    using namespace AZ;

    class BufferPropertyTests
        : public RHITestFixture
    {
    public:
        BufferPropertyTests()
            : RHITestFixture()
        {}

        void SetUp() override
        {
            RHITestFixture::SetUp();
            m_bufferDescriptor.m_byteCount = 2 * 1024;
        }

    protected:
        RHI::BufferProperty<int> m_property;
        RHI::BufferDescriptor m_bufferDescriptor;
    };

    TEST_F(BufferPropertyTests, TestNoop)
    {
        RHI::BufferProperty<double> noopProp;
    }

    TEST_F(BufferPropertyTests, TestInitialization)
    {
        EXPECT_FALSE(m_property.IsInitialized());
        m_property.Init(m_bufferDescriptor);
        EXPECT_TRUE(m_property.IsInitialized());
    }

    TEST_F(BufferPropertyTests, TesNoInit)
    {
        EXPECT_FALSE(m_property.IsInitialized());
        auto range = RHI::BufferSubresourceRange(m_bufferDescriptor);
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_property.Set(range, 1337);
        auto overlapInterval = m_property.Get(range);
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);
    }

    TEST_F(BufferPropertyTests, TestFullRange)
    {
        auto range = RHI::BufferSubresourceRange(m_bufferDescriptor);
        m_property.Init(m_bufferDescriptor);
        m_property.Set(range, 1337);
        auto overlapInterval = m_property.Get(range);
        EXPECT_EQ(overlapInterval.size(), 1);
        const auto& resultRange = overlapInterval.front();
        EXPECT_EQ(resultRange.m_range, range);
        EXPECT_EQ(resultRange.m_property, 1337);
    }

    TEST_F(BufferPropertyTests, TestFullRangeOver)
    {
        auto range = RHI::BufferSubresourceRange(m_bufferDescriptor);
        m_property.Init(m_bufferDescriptor);
        m_property.Set(range, 1337);
        auto newRange = range;
        newRange.m_byteSize += 1024;
        auto overlapInterval = m_property.Get(newRange);
        EXPECT_EQ(overlapInterval.size(), 1);
        const auto& resultRange = overlapInterval.front();
        EXPECT_EQ(resultRange.m_range, range);
        EXPECT_EQ(resultRange.m_property, 1337);
    }

    TEST_F(BufferPropertyTests, TestSetPartialRange)
    {
        auto fullRange = RHI::BufferSubresourceRange(m_bufferDescriptor);
        m_property.Init(m_bufferDescriptor);
        auto range = fullRange;
        range.m_byteOffset += 1;
        range.m_byteSize -= 1;
        m_property.Set(range, 1337);
        auto overlapInterval = m_property.Get(fullRange);
        EXPECT_EQ(overlapInterval.size(), 1);
        const auto& resultRange = overlapInterval.front();
        EXPECT_EQ(resultRange.m_range, range);
        EXPECT_EQ(resultRange.m_property, 1337);
    }

    TEST_F(BufferPropertyTests, TestGetPartialRange)
    {
        auto fullRange = RHI::BufferSubresourceRange(m_bufferDescriptor);
        m_property.Init(m_bufferDescriptor);
        m_property.Set(fullRange, 1337);
        auto range = fullRange;
        range.m_byteOffset += 1;
        range.m_byteSize -= 1;
        auto overlapInterval = m_property.Get(range);
        EXPECT_EQ(overlapInterval.size(), 1);
        const auto& resultRange = overlapInterval.front();
        EXPECT_EQ(resultRange.m_range, range);
        EXPECT_EQ(resultRange.m_property, 1337);
    }

    TEST_F(BufferPropertyTests, TestMerge)
    {
        m_property.Init(m_bufferDescriptor);
        auto fullRange = RHI::BufferSubresourceRange(m_bufferDescriptor);

        auto range1 = fullRange;
        range1.m_byteSize = range1.m_byteSize / 2;
        m_property.Set(range1, 1337);

        auto range2 = fullRange;
        range2.m_byteOffset = range1.m_byteOffset + range1.m_byteSize;
        range2.m_byteSize = m_bufferDescriptor.m_byteCount - range2.m_byteOffset;
        m_property.Set(range2, 1337);

        auto overlapInterval = m_property.Get(fullRange);
        EXPECT_EQ(overlapInterval.size(), 1);
        const auto& resultRange = overlapInterval.front();
        EXPECT_EQ(resultRange.m_range, fullRange);
        EXPECT_EQ(resultRange.m_property, 1337);
    }

    TEST_F(BufferPropertyTests, TestNoMergeDifferentProperty)
    {
        m_property.Init(m_bufferDescriptor);
        auto fullRange = RHI::BufferSubresourceRange(m_bufferDescriptor);

        auto range1 = fullRange;
        range1.m_byteSize = range1.m_byteSize / 2;
        m_property.Set(range1, 1337);

        auto range2 = fullRange;
        range2.m_byteOffset = range1.m_byteOffset + range1.m_byteSize;
        range2.m_byteSize = m_bufferDescriptor.m_byteCount - range2.m_byteOffset;
        m_property.Set(range2, 1338);

        auto overlapInterval = m_property.Get(fullRange);
        EXPECT_EQ(overlapInterval.size(), 2);
        const auto& resultRange1 = overlapInterval.front();
        EXPECT_EQ(resultRange1.m_range, range1);
        EXPECT_EQ(resultRange1.m_property, 1337);
        const auto& resultRange2 = overlapInterval.back();
        EXPECT_EQ(resultRange2.m_range, range2);
        EXPECT_EQ(resultRange2.m_property, 1338);
    }

    TEST_F(BufferPropertyTests, TestNoMergeNoContinous)
    {
        m_property.Init(m_bufferDescriptor);
        auto fullRange = RHI::BufferSubresourceRange(m_bufferDescriptor);

        auto range1 = fullRange;
        range1.m_byteSize = range1.m_byteSize / 2;
        m_property.Set(range1, 1337);

        auto range2 = fullRange;
        range2.m_byteOffset = range1.m_byteSize + 2;
        range2.m_byteSize = m_bufferDescriptor.m_byteCount - range2.m_byteOffset;
        m_property.Set(range2, 1337);

        auto overlapInterval = m_property.Get(fullRange);
        EXPECT_EQ(overlapInterval.size(), 2);
        const auto& resultRange1 = overlapInterval.front();
        EXPECT_EQ(resultRange1.m_range, range1);
        EXPECT_EQ(resultRange1.m_property, 1337);
        const auto& resultRange2 = overlapInterval.back();
        EXPECT_EQ(resultRange2.m_range, range2);
        EXPECT_EQ(resultRange2.m_property, 1337);
    }

    TEST_F(BufferPropertyTests, TestNoOverlap)
    {
        auto range1 = RHI::BufferSubresourceRange(m_bufferDescriptor);
        range1.m_byteOffset = m_bufferDescriptor.m_byteCount / 2;
        range1.m_byteSize = m_bufferDescriptor.m_byteCount - range1.m_byteOffset;
        m_property.Init(m_bufferDescriptor);
        m_property.Set(range1, 1337);

        auto range2 = range1;
        range2.m_byteOffset = 0;
        range2.m_byteSize = range1.m_byteOffset - 1;
        auto overlapInterval = m_property.Get(range2);
        EXPECT_TRUE(overlapInterval.empty());
    }

    TEST_F(BufferPropertyTests, TestMergeDifferentProperty)
    {
        auto fullRange = RHI::BufferSubresourceRange(m_bufferDescriptor);
        m_property.Init(m_bufferDescriptor);
        m_property.Set(fullRange, 1337);

        auto range1 = fullRange;
        range1.m_byteSize = fullRange.m_byteSize / 2;
        m_property.Set(range1, 1338);

        auto range2 = fullRange;
        range2.m_byteOffset = range1.m_byteOffset + range1.m_byteSize;
        m_property.Set(range2, 1338);

        auto overlapInterval = m_property.Get(fullRange);
        EXPECT_EQ(overlapInterval.size(), 1);
        const auto& resultRange = overlapInterval.front();
        EXPECT_EQ(resultRange.m_range, fullRange);
        EXPECT_EQ(resultRange.m_property, 1338);
    }

    TEST_F(BufferPropertyTests, TestPartialMergeDifferentProperty)
    {
        auto fullRange = RHI::BufferSubresourceRange(m_bufferDescriptor);
        m_property.Init(m_bufferDescriptor);
        m_property.Set(fullRange, 1337);

        auto range1 = fullRange;
        range1.m_byteSize = 1;
        m_property.Set(range1, 1338);

        auto range2 = fullRange;
        range2.m_byteOffset = fullRange.m_byteSize - 1;
        range2.m_byteSize = fullRange.m_byteSize - range2.m_byteOffset;
        m_property.Set(range2, 1338);

        auto newRange = fullRange;
        newRange.m_byteOffset = range1.m_byteSize;
        newRange.m_byteSize = range2.m_byteOffset - newRange.m_byteOffset;

        auto overlapInterval = m_property.Get(fullRange);
        EXPECT_EQ(overlapInterval.size(), 3);
        const auto& resultRange1 = overlapInterval[0];
        const auto& resultRange2 = overlapInterval[1];
        const auto& resultRange3 = overlapInterval[2];
        EXPECT_EQ(resultRange1.m_range, range1);
        EXPECT_EQ(resultRange1.m_property, 1338);
        EXPECT_EQ(resultRange2.m_range, newRange);
        EXPECT_EQ(resultRange2.m_property, 1337);
        EXPECT_EQ(resultRange3.m_range, range2);
        EXPECT_EQ(resultRange3.m_property, 1338);
    }

    TEST_F(BufferPropertyTests, TestReset)
    {
        auto range = RHI::BufferSubresourceRange(m_bufferDescriptor);
        m_property.Init(m_bufferDescriptor);
        m_property.Set(range, 1337);
        auto overlapInterval = m_property.Get(range);
        EXPECT_FALSE(overlapInterval.empty());
        m_property.Reset();
        overlapInterval = m_property.Get(range);
        EXPECT_TRUE(overlapInterval.empty());
    }
}
