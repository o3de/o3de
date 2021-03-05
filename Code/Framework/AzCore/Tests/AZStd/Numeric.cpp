/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <AzCore/std/numeric.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class AccumulateFixture
        : public AllocatorsTestFixture
    {
    };

    TEST_F(AccumulateFixture, AccumulateWithoutBinaryOperator)
    {
        using ::testing::Eq;

        AZStd::vector<int> numbers{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        const int total = AZStd::accumulate(AZStd::cbegin(numbers), AZStd::cend(numbers), 0);
        EXPECT_THAT(total, Eq(55));
    }

    TEST_F(AccumulateFixture, AccumulateWithBinaryOperator)
    {
        using ::testing::Eq;
        using ::testing::ElementsAre;

        const AZStd::vector<int> numbers{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        const AZStd::vector<int> evenNumbers =
            AZStd::accumulate(
                AZStd::cbegin(numbers), AZStd::cend(numbers), AZStd::vector<int>{},
                [](AZStd::vector<int> acc, const int number)
                {
                    if (number % 2 == 0)
                    {
                        acc.push_back(number);
                    }

                    return acc;
                });

        EXPECT_THAT(evenNumbers.size(), Eq(5));
        EXPECT_THAT(evenNumbers, ElementsAre(2, 4, 6, 8, 10));
    }
} // namespace UnitTest
