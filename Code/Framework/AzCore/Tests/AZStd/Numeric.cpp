/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/numeric.h>

namespace UnitTest
{
    class AccumulateFixture : public LeakDetectionFixture
    {
    };

    TEST_F(AccumulateFixture, AccumulateWithoutBinaryOperator)
    {
        using ::testing::Eq;

        AZStd::vector<int> numbers{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        const int total = AZStd::accumulate(AZStd::cbegin(numbers), AZStd::cend(numbers), 0);
        EXPECT_THAT(total, Eq(55));
    }

    TEST_F(AccumulateFixture, AccumulateWithBinaryOperator)
    {
        using ::testing::ElementsAre;
        using ::testing::Eq;

        const AZStd::vector<int> numbers{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        const AZStd::vector<int> evenNumbers = AZStd::accumulate(
            AZStd::cbegin(numbers), AZStd::cend(numbers), AZStd::vector<int>{}, [](AZStd::vector<int> acc, const int number) {
                if (number % 2 == 0)
                {
                    acc.push_back(number);
                }

                return acc;
            });

        EXPECT_THAT(evenNumbers.size(), Eq(5));
        EXPECT_THAT(evenNumbers, ElementsAre(2, 4, 6, 8, 10));
    }

    class InnerProductFixture : public LeakDetectionFixture
    {
    };

    TEST_F(InnerProductFixture, InnerProductWithoutBinaryOperator)
    {
        using ::testing::Eq;

        AZStd::vector<int> numbers1{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        AZStd::vector<int> numbers2{2, 4, 6, 8, 10, 12, 14, 16, 18, 20};

        const int total = AZStd::inner_product(AZStd::cbegin(numbers1), AZStd::cend(numbers1), AZStd::cbegin(numbers2), 0);
        EXPECT_THAT(total, Eq(770));
    }

    TEST_F(InnerProductFixture, InnerProductWithBinaryOperator)
    {
        using ::testing::ElementsAre;
        using ::testing::Eq;

        const AZStd::vector<int> number_values{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        const AZStd::vector<AZStd::string> number_names{"one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten"};

        struct NumberLabel
        {
            int m_value;
            AZStd::string m_name;
        };

        const AZStd::vector<NumberLabel> numberLabels = AZStd::inner_product(
            AZStd::cbegin(number_values), AZStd::cend(number_values), AZStd::cbegin(number_names), AZStd::vector<NumberLabel>{},
            [](AZStd::vector<NumberLabel> acc, const NumberLabel& numberLabel) {
                acc.push_back(numberLabel);
                return acc;
            },
            [](const int value, const AZStd::string& label) {
                return NumberLabel{value, label};
            });

        EXPECT_THAT(numberLabels.size(), Eq(10));

        for (size_t i = 0; i < numberLabels.size(); ++i)
        {
            EXPECT_THAT(numberLabels[i].m_value, Eq(number_values[i]));
            EXPECT_THAT(numberLabels[i].m_name, Eq(number_names[i]));
        }
    }
} // namespace UnitTest
