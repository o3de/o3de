/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <Atom/RHI/ImageProperty.h>

namespace UnitTest
{
    using namespace AZ;

    class ImagePropertyTests
        : public RHITestFixture
    {
    public:
        ImagePropertyTests()
            : RHITestFixture()
        {}

        void SetUp() override
        {
            RHITestFixture::SetUp();
            m_imageDescriptor.m_arraySize = 10;
            m_imageDescriptor.m_mipLevels = 5;
            m_imageDescriptor.m_format = RHI::Format::D32_FLOAT_S8X24_UINT;
        }

    protected:
        RHI::ImageProperty<int> m_property;
        RHI::ImageDescriptor m_imageDescriptor;
    };

    TEST_F(ImagePropertyTests, TestNoop)
    {
        RHI::ImageProperty<double> noopProp;
    }

    TEST_F(ImagePropertyTests, TestInitialization)
    {
        EXPECT_FALSE(m_property.IsInitialized());
        m_property.Init(m_imageDescriptor);
        EXPECT_TRUE(m_property.IsInitialized());
    }

    TEST_F(ImagePropertyTests, TesNoInit)
    {
        EXPECT_FALSE(m_property.IsInitialized());
        auto range = RHI::ImageSubresourceRange(m_imageDescriptor);
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_property.Set(range, 1337);
        auto overlapInterval = m_property.Get(range);
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);
    }

    TEST_F(ImagePropertyTests, TestFullRange)
    {
        auto range = RHI::ImageSubresourceRange(m_imageDescriptor);
        m_property.Init(m_imageDescriptor);
        m_property.Set(range, 1337);
        auto overlapInterval = m_property.Get(range);
        EXPECT_EQ(overlapInterval.size(), 1);
        const auto& resultRange = overlapInterval.front();
        EXPECT_EQ(resultRange.m_range, range);
        EXPECT_EQ(resultRange.m_property, 1337);
    }

    TEST_F(ImagePropertyTests, TestFullRangeOver)
    {
        auto range = RHI::ImageSubresourceRange(m_imageDescriptor);
        m_property.Init(m_imageDescriptor);
        m_property.Set(range, 1337);
        auto newRange = range;
        newRange.m_arraySliceMax += 10;
        newRange.m_mipSliceMax += 10;
        auto overlapInterval = m_property.Get(newRange);
        EXPECT_EQ(overlapInterval.size(), 1);
        const auto& resultRange = overlapInterval.front();
        EXPECT_EQ(resultRange.m_range, range);
        EXPECT_EQ(resultRange.m_property, 1337);
    }

    TEST_F(ImagePropertyTests, TestPartialRange)
    {
        auto fullRange = RHI::ImageSubresourceRange(m_imageDescriptor);
        m_property.Init(m_imageDescriptor);
        auto range = fullRange;
        range.m_mipSliceMax -= 1;
        m_property.Set(range, 1337);
        auto overlapInterval = m_property.Get(fullRange);
        EXPECT_EQ(overlapInterval.size(), 1);
        const auto& resultRange = overlapInterval.front();
        EXPECT_EQ(resultRange.m_range, range);
        EXPECT_EQ(resultRange.m_property, 1337);
    }

    TEST_F(ImagePropertyTests, TestPerArrayRange)
    {
        auto fullRange = RHI::ImageSubresourceRange(m_imageDescriptor);
        m_property.Init(m_imageDescriptor);
        m_property.Set(fullRange, 1337);
        auto range = fullRange;
        range.m_arraySliceMax -= 1;
        auto overlapInterval = m_property.Get(range);
        EXPECT_EQ(overlapInterval.size(), m_imageDescriptor.m_mipLevels);
        for (uint16_t i = 0; i < overlapInterval.size(); ++i)
        {
            RHI::ImageSubresourceRange mipRange = range;
            mipRange.m_mipSliceMin = i;
            mipRange.m_mipSliceMax = i;
            const auto& intervalProperty = overlapInterval[i];
            EXPECT_EQ(intervalProperty.m_property, 1337);
            EXPECT_EQ(intervalProperty.m_range, mipRange);
        }
    }

    TEST_F(ImagePropertyTests, TestMerge)
    {
        m_property.Init(m_imageDescriptor);
        auto fullRange = RHI::ImageSubresourceRange(m_imageDescriptor);

        auto range1 = fullRange;
        range1.m_mipSliceMax = range1.m_mipSliceMax / 2;
        m_property.Set(range1, 1337);

        auto range2 = fullRange;
        range2.m_mipSliceMin = range1.m_mipSliceMax + 1;
        m_property.Set(range2, 1337);

        auto overlapInterval = m_property.Get(fullRange);
        EXPECT_EQ(overlapInterval.size(), 1);
        const auto& resultRange = overlapInterval.front();
        EXPECT_EQ(resultRange.m_range, fullRange);
        EXPECT_EQ(resultRange.m_property, 1337);
    }

    TEST_F(ImagePropertyTests, TestNoMergeDifferentProperty)
    {
        m_property.Init(m_imageDescriptor);
        auto fullRange = RHI::ImageSubresourceRange(m_imageDescriptor);

        auto range1 = fullRange;
        range1.m_mipSliceMax = range1.m_mipSliceMax / 2;
        m_property.Set(range1, 1337);

        auto range2 = fullRange;
        range2.m_mipSliceMin = range1.m_mipSliceMax + 1;
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

    TEST_F(ImagePropertyTests, TestNoMergeNoContinous)
    {
        m_property.Init(m_imageDescriptor);
        auto fullRange = RHI::ImageSubresourceRange(m_imageDescriptor);

        auto range1 = fullRange;
        range1.m_mipSliceMax = range1.m_mipSliceMax / 2;
        m_property.Set(range1, 1337);

        auto range2 = fullRange;
        range2.m_mipSliceMin = range1.m_mipSliceMax + 2;
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

    TEST_F(ImagePropertyTests, TestPartialRangeImageAspect)
    {
        auto fullRange = RHI::ImageSubresourceRange(m_imageDescriptor);
        m_property.Init(m_imageDescriptor);
        m_property.Set(fullRange, 1337);
        auto range = fullRange;
        range.m_aspectFlags = RHI::ResetBits(range.m_aspectFlags, RHI::ImageAspectFlags::Depth);

        auto overlapInterval = m_property.Get(range);
        EXPECT_EQ(overlapInterval.size(), 1);
        const auto& resultRange = overlapInterval.front();
        EXPECT_EQ(resultRange.m_range, range);
        EXPECT_EQ(resultRange.m_property, 1337);
    }

    TEST_F(ImagePropertyTests, TestNoOverlap)
    {
        auto range1 = RHI::ImageSubresourceRange(m_imageDescriptor);
        range1.m_mipSliceMin = m_imageDescriptor.m_mipLevels / 2;
        range1.m_mipSliceMax = m_imageDescriptor.m_mipLevels - 1;
        m_property.Init(m_imageDescriptor);
        m_property.Set(range1, 1337);

        auto range2 = range1;
        range2.m_mipSliceMin = 0;
        range2.m_mipSliceMax = range1.m_mipSliceMin - 1;
        auto overlapInterval = m_property.Get(range2);
        EXPECT_TRUE(overlapInterval.empty());
    }

    TEST_F(ImagePropertyTests, TestNoOverlapImageAspect)
    {
        auto range = RHI::ImageSubresourceRange(m_imageDescriptor);
        range.m_aspectFlags = RHI::ImageAspectFlags::Depth;
        m_property.Init(m_imageDescriptor);
        m_property.Set(range, 1337);

        range.m_aspectFlags = RHI::ImageAspectFlags::Stencil;
        auto overlapInterval = m_property.Get(range);
        EXPECT_TRUE(overlapInterval.empty());
    }


    TEST_F(ImagePropertyTests, TestMergeDifferentProperty)
    {
        auto fullRange = RHI::ImageSubresourceRange(m_imageDescriptor);
        m_property.Init(m_imageDescriptor);
        m_property.Set(fullRange, 1337);

        auto range1 = fullRange;
        range1.m_mipSliceMax = fullRange.m_mipSliceMax / 2;
        m_property.Set(range1, 1338);

        auto range2 = fullRange;
        range2.m_mipSliceMin = range1.m_mipSliceMax + 1;
        m_property.Set(range2, 1338);

        auto overlapInterval = m_property.Get(fullRange);
        EXPECT_EQ(overlapInterval.size(), 1);
        const auto& resultRange = overlapInterval.front();
        EXPECT_EQ(resultRange.m_range, fullRange);
        EXPECT_EQ(resultRange.m_property, 1338);
    }

    TEST_F(ImagePropertyTests, TestPartialMergeDifferentProperty)
    {
        auto fullRange = RHI::ImageSubresourceRange(m_imageDescriptor);
        m_property.Init(m_imageDescriptor);
        m_property.Set(fullRange, 1337);

        auto range1 = fullRange;
        range1.m_mipSliceMax = 1;
        m_property.Set(range1, 1338);

        auto range2 = fullRange;
        range2.m_mipSliceMin = fullRange.m_mipSliceMax - 1;
        m_property.Set(range2, 1338);

        auto newRange = fullRange;
        newRange.m_mipSliceMin = range1.m_mipSliceMax + 1;
        newRange.m_mipSliceMax = range2.m_mipSliceMin - 1;

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

    TEST_F(ImagePropertyTests, TestReset)
    {
        auto range = RHI::ImageSubresourceRange(m_imageDescriptor);
        m_property.Init(m_imageDescriptor);
        m_property.Set(range, 1337);
        auto overlapInterval = m_property.Get(range);
        EXPECT_FALSE(overlapInterval.empty());
        m_property.Reset();
        overlapInterval = m_property.Get(range);
        EXPECT_TRUE(overlapInterval.empty());
    }
}
