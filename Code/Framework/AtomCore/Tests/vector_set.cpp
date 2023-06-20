/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomCore/std/containers/vector_set.h>
#include <AtomCore/std/containers/fixed_vector_set.h>

#include <AzCore/UnitTest/TestTypes.h>

using namespace AZStd;

namespace UnitTest
{
    class VectorSets
        : public LeakDetectionFixture
    {
    };

    class FixedVectorSets
        : public LeakDetectionFixture
    {
    };

    template <typename SetType>
    struct VectorSetTester
    {
        using this_type = VectorSetTester<SetType>;

        const AZStd::vector<int32_t> m_expected = { 0, 1, 4, 9, 11, 14, 21, 23, 25, 27, 31 };
        const AZStd::vector<int32_t> m_unexpected = { 5, -2 };
        const SetType m_vectorSet = { 25, 0, 9, 21, 27, 1, 9, 23, 4, 14, 31, 0, 11 };

        void TestFindConst() const
        {
            for (int32_t value : m_expected)
            {
                auto it = m_vectorSet.find(value);
                EXPECT_EQ(*it, value);
            }

            for (int32_t value : m_unexpected)
            {
                auto it = m_vectorSet.find(value);
                EXPECT_EQ(it, m_vectorSet.end());
            }

            EXPECT_EQ(m_vectorSet.size(), m_expected.size());

            for (size_t i = 0; i < m_vectorSet.size(); ++i)
            {
                EXPECT_EQ(m_vectorSet[i], m_expected[i]);
            }
        }

        void TestFind()
        {
            for (int32_t value : m_expected)
            {
                auto it = m_vectorSet.find(value);
                EXPECT_EQ(*it, value);
            }

            for (int32_t value : m_unexpected)
            {
                auto it = m_vectorSet.find(value);
                EXPECT_EQ(it, m_vectorSet.end());
            }

            EXPECT_EQ(m_vectorSet.size(), m_expected.size());

            for (size_t i = 0; i < m_vectorSet.size(); ++i)
            {
                EXPECT_EQ(m_vectorSet[i], m_expected[i]);
            }
        }

        void TestInsertion()
        {
            auto vectorSet = m_vectorSet;

            EXPECT_EQ(vectorSet.erase(9), 1);
            EXPECT_EQ(vectorSet.erase(8), 0);
            EXPECT_EQ(vectorSet.find(9), vectorSet.end());

            EXPECT_EQ(vectorSet.insert(9).second, true);
            EXPECT_EQ(*vectorSet.find(9), 9);

            EXPECT_EQ(vectorSet.erase(25), 1);
            EXPECT_EQ(vectorSet.find(25), vectorSet.end());
            EXPECT_EQ(*vectorSet.lower_bound(25), 27);
            EXPECT_EQ(*vectorSet.upper_bound(25), 27);

            auto iterBoolPair = vectorSet.emplace(25);
            EXPECT_EQ(*iterBoolPair.first, 25);
            EXPECT_TRUE(iterBoolPair.second);

            iterBoolPair = vectorSet.insert(25);
            EXPECT_EQ(*iterBoolPair.first, 25);
            EXPECT_FALSE(iterBoolPair.second);
        }

        void TestCompare()
        {
            auto vectorSet = m_vectorSet;

            EXPECT_FALSE(vectorSet.empty());

            SetType intSet2 = vectorSet;
            EXPECT_EQ(vectorSet, intSet2);

            intSet2.erase(9);
            EXPECT_NE(vectorSet, intSet2);

            intSet2.clear();
            EXPECT_EQ(intSet2.size(), 0);
            EXPECT_TRUE(intSet2.empty());
        }

        void TestAssignment()
        {
            auto vectorSet = m_vectorSet;

            vectorSet.assign(m_expected.begin(), m_expected.end());
            vectorSet.insert(m_expected.begin(), m_expected.end());

            for (size_t i = 0; i < vectorSet.size(); ++i)
            {
                EXPECT_EQ(vectorSet[i], m_expected[i]);
            }
        }

        void TestIterators()
        {
            EXPECT_EQ(m_expected.size(), m_vectorSet.size());

            {
                auto it1 = m_expected.begin();
                auto it2 = m_vectorSet.begin();
                for (; it1 != m_expected.end(); ++it1, ++it2)
                {
                    EXPECT_NE(it2, m_vectorSet.end());
                    EXPECT_EQ(*it1, *it2);
                }
            }

            {
                auto it1 = m_expected.rbegin();
                auto it2 = m_vectorSet.rbegin();
                for (; it1 != m_expected.rend(); ++it1, ++it2)
                {
                    EXPECT_NE(it2, m_vectorSet.rend());
                    EXPECT_EQ(*it1, *it2);
                }
            }
        }

        void TestIteratorsConst() const
        {
            EXPECT_EQ(m_expected.size(), m_vectorSet.size());

            {
                auto it1 = m_expected.begin();
                auto it2 = m_vectorSet.begin();
                for (; it1 != m_expected.end(); ++it1, ++it2)
                {
                    EXPECT_NE(it2, m_vectorSet.end());
                    EXPECT_EQ(*it1, *it2);
                }
            }

            {
                auto it1 = m_expected.rbegin();
                auto it2 = m_vectorSet.rbegin();
                for (; it1 != m_expected.rend(); ++it1, ++it2)
                {
                    EXPECT_NE(it2, m_vectorSet.rend());
                    EXPECT_EQ(*it1, *it2);
                }
            }
        }

    };

    TEST_F(VectorSets, Find)
    {
        VectorSetTester<AZStd::vector_set<int32_t>> tester;
        tester.TestFind();
    }

    TEST_F(VectorSets, FindConst)
    {
        VectorSetTester<AZStd::vector_set<int32_t>> tester;
        tester.TestFindConst();
    }

    TEST_F(VectorSets, Insertion)
    {
        VectorSetTester<AZStd::vector_set<int32_t>> tester;
        tester.TestInsertion();
    }

    TEST_F(VectorSets, Compare)
    {
        VectorSetTester<AZStd::vector_set<int32_t>> tester;
        tester.TestCompare();
    }

    TEST_F(VectorSets, Assignment)
    {
        VectorSetTester<AZStd::vector_set<int32_t>> tester;
        tester.TestAssignment();
    }

    TEST_F(VectorSets, Iterators)
    {
        VectorSetTester<AZStd::vector_set<int32_t>> tester;
        tester.TestIterators();
    }

    TEST_F(VectorSets, IteratorsConst)
    {
        VectorSetTester<AZStd::vector_set<int32_t>> tester;
        tester.TestIteratorsConst();
    }

    TEST_F(FixedVectorSets, Find)
    {
        VectorSetTester<AZStd::fixed_vector_set<int32_t, 64>> tester;
        tester.TestFind();
    }

    TEST_F(FixedVectorSets, FindConst)
    {
        VectorSetTester<AZStd::fixed_vector_set<int32_t, 64>> tester;
        tester.TestFindConst();
    }

    TEST_F(FixedVectorSets, Insertion)
    {
        VectorSetTester<AZStd::fixed_vector_set<int32_t, 64>> tester;
        tester.TestInsertion();
    }

    TEST_F(FixedVectorSets, Compare)
    {
        VectorSetTester<AZStd::fixed_vector_set<int32_t, 64>> tester;
        tester.TestCompare();
    }

    TEST_F(FixedVectorSets, Assignment)
    {
        VectorSetTester<AZStd::fixed_vector_set<int32_t, 64>> tester;
        tester.TestAssignment();
    }

    TEST_F(FixedVectorSets, Iterators)
    {
        VectorSetTester<AZStd::fixed_vector_set<int32_t, 64>> tester;
        tester.TestIterators();
    }

    TEST_F(FixedVectorSets, IteratorsConst)
    {
        VectorSetTester<AZStd::fixed_vector_set<int32_t, 64>> tester;
        tester.TestIteratorsConst();
    }
}
