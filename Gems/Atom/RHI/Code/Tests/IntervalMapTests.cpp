/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RHITestFixture.h"
#include <Atom/RHI/interval_map.h>

namespace UnitTest
{
    using namespace AZ;

    class IntervalMapTests
        : public RHITestFixture
    {
    public:
        IntervalMapTests()
            : RHITestFixture()
        {}

    protected:
        RHI::interval_map<uint32_t, uint32_t> m_intervalMap;
    };

    TEST_F(IntervalMapTests, TestEmpty)
    {
        EXPECT_TRUE(m_intervalMap.empty());
    }

    TEST_F(IntervalMapTests, TestAssign)
    {
        RHI::Interval interval(232, 12312);
        auto it = m_intervalMap.assign(interval.m_min, interval.m_max, 1337);
        EXPECT_FALSE(m_intervalMap.empty());
        EXPECT_EQ(it, m_intervalMap.begin());
        EXPECT_EQ(m_intervalMap.begin().value(), 1337);
    }

    TEST_F(IntervalMapTests, TestAssignInvalidInterval)
    {
        RHI::Interval interval(12312, 232);
        auto it = m_intervalMap.assign(interval.m_min, interval.m_max, 1337);
        EXPECT_TRUE(m_intervalMap.empty());
        EXPECT_EQ(it, m_intervalMap.end());
    }

    TEST_F(IntervalMapTests, TestAt)
    {
        RHI::Interval interval(232, 12312);
        m_intervalMap.assign(interval.m_min, interval.m_max, 1337);
        EXPECT_FALSE(m_intervalMap.empty());
        EXPECT_EQ(m_intervalMap.begin().value(), 1337);
        EXPECT_EQ(m_intervalMap.at(interval.m_min), m_intervalMap.begin());
        EXPECT_EQ(m_intervalMap.at(interval.m_max), m_intervalMap.end());
        EXPECT_EQ(m_intervalMap.at(interval.m_min + 1), m_intervalMap.begin());
        EXPECT_EQ(m_intervalMap.at(interval.m_max + 1), m_intervalMap.end());
    }

    TEST_F(IntervalMapTests, TestAtMultipleIntervals)
    {
        RHI::Interval interval1(0, 500);
        RHI::Interval interval2(600, 1000);
        m_intervalMap.assign(interval1.m_min, interval1.m_max, 1337);
        m_intervalMap.assign(interval2.m_min, interval2.m_max, 1338);
        auto iterator1 = m_intervalMap.at(interval1.m_min);
        auto iterator2 = m_intervalMap.at(interval2.m_min);
        EXPECT_EQ(iterator1.value(), 1337);
        EXPECT_EQ(iterator1.interval_begin(), interval1.m_min);
        EXPECT_EQ(iterator1.interval_end(), interval1.m_max);
        EXPECT_EQ(iterator2.value(), 1338);
        EXPECT_EQ(iterator2.interval_begin(), interval2.m_min);
        EXPECT_EQ(iterator2.interval_end(), interval2.m_max);
        EXPECT_EQ(m_intervalMap.at(interval1.m_max), m_intervalMap.end());
    }

    TEST_F(IntervalMapTests, TestMergeIntervals)
    {
        RHI::Interval interval1(0, 500);
        RHI::Interval interval2(500, 1000);
        m_intervalMap.assign(interval1.m_min, interval1.m_max, 1337);
        m_intervalMap.assign(interval2.m_min, interval2.m_max, 1337);
        auto iter = m_intervalMap.begin();
        EXPECT_FALSE(m_intervalMap.empty());
        EXPECT_EQ(iter.value(), 1337);
        EXPECT_EQ(iter.interval_begin(), interval1.m_min);
        EXPECT_EQ(iter.interval_end(), interval2.m_max);
        iter++;
        EXPECT_EQ(iter, m_intervalMap.end());
    }

    TEST_F(IntervalMapTests, TestNoMergeIntervals)
    {
        RHI::Interval interval1(0, 500);
        RHI::Interval interval2(500, 1000);
        m_intervalMap.assign(interval1.m_min, interval1.m_max, 1337);
        m_intervalMap.assign(interval2.m_min, interval2.m_max, 1338);
        auto iter = m_intervalMap.begin();
        EXPECT_FALSE(m_intervalMap.empty());
        EXPECT_EQ(iter.value(), 1337);
        EXPECT_EQ(iter.interval_begin(), interval1.m_min);
        EXPECT_EQ(iter.interval_end(), interval1.m_max);
        iter++;
        EXPECT_EQ(iter.value(), 1338);
        EXPECT_EQ(iter.interval_begin(), interval2.m_min);
        EXPECT_EQ(iter.interval_end(), interval2.m_max);
        iter++;
        EXPECT_EQ(iter, m_intervalMap.end());
    }

    TEST_F(IntervalMapTests, TestOverlapSingleInterval)
    {
        RHI::Interval interval(0, 500);
        m_intervalMap.assign(interval.m_min, interval.m_max, 1337);
        auto overlap = m_intervalMap.overlap(interval.m_min, interval.m_max);
        EXPECT_EQ(overlap.first, m_intervalMap.begin());
        EXPECT_EQ(overlap.second, m_intervalMap.end());
    }

    TEST_F(IntervalMapTests, TestOverlapMultipleIntervals)
    {
        RHI::Interval interval1(0, 500);
        RHI::Interval interval2(600, 1000);
        m_intervalMap.assign(interval1.m_min, interval1.m_max, 1337);
        m_intervalMap.assign(interval2.m_min, interval2.m_max, 1338);
        auto overlap = m_intervalMap.overlap(interval1.m_min, interval2.m_max);
        auto iter = overlap.first;
        EXPECT_EQ(iter.value(), 1337);
        EXPECT_EQ(iter.interval_begin(), interval1.m_min);
        EXPECT_EQ(iter.interval_end(), interval1.m_max);
        iter++;
        EXPECT_EQ(iter.value(), 1338);
        EXPECT_EQ(iter.interval_begin(), interval2.m_min);
        EXPECT_EQ(iter.interval_end(), interval2.m_max);
        iter++;
        EXPECT_EQ(iter, m_intervalMap.end());
        overlap = m_intervalMap.overlap(interval1.m_min, interval2.m_min);
        iter = overlap.first;
        EXPECT_EQ(iter.value(), 1337);
        EXPECT_EQ(iter.interval_begin(), interval1.m_min);
        EXPECT_EQ(iter.interval_end(), interval1.m_max);
        iter++;
        EXPECT_EQ(iter, overlap.second);
        overlap = m_intervalMap.overlap(interval1.m_min, interval2.m_min + 1);
        iter = overlap.first;
        EXPECT_EQ(iter.value(), 1337);
        EXPECT_EQ(iter.interval_begin(), interval1.m_min);
        EXPECT_EQ(iter.interval_end(), interval1.m_max);
        iter++;
        EXPECT_EQ(iter.value(), 1338);
        EXPECT_EQ(iter.interval_begin(), interval2.m_min);
        EXPECT_EQ(iter.interval_end(), interval2.m_max);
        iter++;
        EXPECT_EQ(iter, m_intervalMap.end());
    }

    TEST_F(IntervalMapTests, TestNoOverlap)
    {
        RHI::Interval interval(0, 500);
        m_intervalMap.assign(interval.m_min, interval.m_max, 1337);
        auto overlap = m_intervalMap.overlap(interval.m_max, interval.m_max);
        EXPECT_EQ(overlap.first, m_intervalMap.end());
        EXPECT_EQ(overlap.second, m_intervalMap.end());
    }

    TEST_F(IntervalMapTests, TestOverlapContinuousIntervals)
    {
        RHI::Interval interval1(0, 500);
        RHI::Interval interval2(500, 1000);
        m_intervalMap.assign(interval1.m_min, interval1.m_max, 1337);
        m_intervalMap.assign(interval2.m_min, interval2.m_max, 1338);
        auto overlap = m_intervalMap.overlap(interval1.m_min, interval1.m_max);
        auto iter = overlap.first;
        EXPECT_EQ(iter.value(), 1337);
        EXPECT_EQ(iter.interval_begin(), interval1.m_min);
        EXPECT_EQ(iter.interval_end(), interval1.m_max);
        iter++;
        EXPECT_EQ(iter, overlap.second);

        overlap = m_intervalMap.overlap(interval2.m_min, interval2.m_max);
        iter = overlap.first;
        EXPECT_EQ(iter.value(), 1338);
        EXPECT_EQ(iter.interval_begin(), interval2.m_min);
        EXPECT_EQ(iter.interval_end(), interval2.m_max);
        iter++;
        EXPECT_EQ(iter, overlap.second);
    }

    TEST_F(IntervalMapTests, TestErase)
    {
        RHI::Interval interval1(0, 500);
        RHI::Interval interval2(500, 1000);
        RHI::Interval interval3(1000, 1500);
        m_intervalMap.assign(interval2.m_min, interval2.m_max, 1338);
        m_intervalMap.assign(interval3.m_min, interval3.m_max, 1339);
        m_intervalMap.assign(interval1.m_min, interval1.m_max, 1337);



        auto iter = m_intervalMap.at(interval2.m_min);
        EXPECT_EQ(iter.value(), 1338);
        EXPECT_EQ(iter.interval_begin(), interval2.m_min);
        EXPECT_EQ(iter.interval_end(), interval2.m_max);

        auto iter2 = m_intervalMap.erase(iter);
        EXPECT_EQ(iter2.value(), 1339);
        EXPECT_EQ(iter2.interval_begin(), interval3.m_min);
        EXPECT_EQ(iter2.interval_end(), interval3.m_max);

        auto iter3 = m_intervalMap.erase(iter2);
        EXPECT_EQ(iter3, m_intervalMap.end());

        EXPECT_EQ(m_intervalMap.begin().value(), 1337);
        EXPECT_EQ(m_intervalMap.begin().interval_begin(), interval1.m_min);
        EXPECT_EQ(m_intervalMap.begin().interval_end(), interval1.m_max);

        m_intervalMap.erase(m_intervalMap.begin());
        EXPECT_TRUE(m_intervalMap.empty());
    }

    TEST_F(IntervalMapTests, TestEraseComplex)
    {
        RHI::Interval interval1(0, 500);
        RHI::Interval interval2(600, 1000);
        RHI::Interval interval3(1100, 1500);
        m_intervalMap.assign(interval2.m_min, interval2.m_max, 1338);
        m_intervalMap.assign(interval3.m_min, interval3.m_max, 1339);
        m_intervalMap.assign(interval1.m_min, interval1.m_max, 1337);

        auto iter = m_intervalMap.at(interval2.m_min);
        EXPECT_EQ(iter.value(), 1338);
        EXPECT_EQ(iter.interval_begin(), interval2.m_min);
        EXPECT_EQ(iter.interval_end(), interval2.m_max);

        auto iter2 = m_intervalMap.erase(iter);
        EXPECT_EQ(iter2.value(), 1339);
        EXPECT_EQ(iter2.interval_begin(), interval3.m_min);
        EXPECT_EQ(iter2.interval_end(), interval3.m_max);

        auto iter3 = m_intervalMap.erase(iter2);
        EXPECT_EQ(iter3, m_intervalMap.end());

        EXPECT_EQ(m_intervalMap.begin().value(), 1337);
        EXPECT_EQ(m_intervalMap.begin().interval_begin(), interval1.m_min);
        EXPECT_EQ(m_intervalMap.begin().interval_end(), interval1.m_max);

        m_intervalMap.erase(m_intervalMap.begin());
        EXPECT_TRUE(m_intervalMap.empty());
    }
}
