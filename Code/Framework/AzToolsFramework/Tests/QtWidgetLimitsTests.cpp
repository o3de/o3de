/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/PropertyEditor/QtWidgetLimits.h>
#include "IntegerPrimtitiveTestConfig.h"

namespace UnitTest
{
    using namespace AzToolsFramework;

    template<typename ValueType>
    struct QtWidgetLimitsFixture
        : public ToolsApplicationFixture<>
    {

    };

    TYPED_TEST_CASE(QtWidgetLimitsFixture, IntegerPrimtitiveTestConfigs);

    TYPED_TEST(QtWidgetLimitsFixture, MinRange)
    {
        switch (AZ::IntegralTypeCompare<TypeParam, QtWidgetValueType>())
        {
            // Given an LY widget value type of equal signedness and size to QtWidgetValueType
        case AZ::IntegralTypeDiff::LSignedRSignedEqSize:
        {
            // Expect the minimum range of widget type to equal QtWidgetValueType
            EXPECT_EQ(QtWidgetLimits<TypeParam>::Min(), std::numeric_limits<TypeParam>::min());
            EXPECT_EQ(QtWidgetLimits<TypeParam>::Min(), std::numeric_limits<QtWidgetValueType>::min());
            break;
        }

        // Given an LY widget type of equal signedness but wider than QtWidgetValueType
        case AZ::IntegralTypeDiff::LSignedRSignedLWider:
        {
            // Expect the minimum range of widget type to be clamped to the range of QtWidgetValueType
            EXPECT_NE(QtWidgetLimits<TypeParam>::Min(), std::numeric_limits<TypeParam>::min());
            EXPECT_EQ(QtWidgetLimits<TypeParam>::Min(), std::numeric_limits<QtWidgetValueType>::min());
            break;
        }

        // Given an LY widget type with a minimum range greater than the range of QtWidgetValueType
        case AZ::IntegralTypeDiff::LSignedRSignedRWider:
        case AZ::IntegralTypeDiff::LUnsignedRSignedLWider:
        case AZ::IntegralTypeDiff::LUnsignedRSignedEqSize:
        case AZ::IntegralTypeDiff::LUnsignedRSignedRWider:
        {
            // Expect the minimum range of widget type to be greater than the minimum range of QtWidgetValueType
            EXPECT_EQ(QtWidgetLimits<TypeParam>::Min(), std::numeric_limits<TypeParam>::min());
            EXPECT_NE(QtWidgetLimits<TypeParam>::Min(), std::numeric_limits<QtWidgetValueType>::min());
            break;
        }

        default:
            FAIL();
        }
    }

    TYPED_TEST(QtWidgetLimitsFixture, MaxRange)
    {
        switch (AZ::IntegralTypeCompare<TypeParam, QtWidgetValueType>())
        {
            // Given an LY widget value type of equal signedness and size to QtWidgetValueType
        case AZ::IntegralTypeDiff::LSignedRSignedEqSize:
        {
            // Expect the maximum range of widget type to equal QtWidgetValueType
            EXPECT_EQ(QtWidgetLimits<TypeParam>::Max(), std::numeric_limits<TypeParam>::max());
            EXPECT_EQ(QtWidgetLimits<TypeParam>::Max(), std::numeric_limits<QtWidgetValueType>::max());
            break;
        }

        // Given an LY widget type with a maximum range greater than the range of QtWidgetValueType
        case AZ::IntegralTypeDiff::LSignedRSignedLWider:
        case AZ::IntegralTypeDiff::LUnsignedRSignedLWider:
        case AZ::IntegralTypeDiff::LUnsignedRSignedEqSize:
        {
            // Expect the maximum range of widget type to be clamped to the range of QtWidgetValueType
            EXPECT_NE(QtWidgetLimits<TypeParam>::Max(), std::numeric_limits<TypeParam>::max());
            EXPECT_EQ(QtWidgetLimits<TypeParam>::Max(), std::numeric_limits<QtWidgetValueType>::max());
            break;
        }

        // Given an LY widget type with a maximum range less than the range of QtWidgetValueType
        case AZ::IntegralTypeDiff::LUnsignedRSignedRWider:
        case AZ::IntegralTypeDiff::LSignedRSignedRWider:
        {
            // Expect the maximum range of widget type to be less than the minimum range of QtWidgetValueType
            EXPECT_EQ(QtWidgetLimits<TypeParam>::Max(), std::numeric_limits<TypeParam>::max());
            EXPECT_NE(QtWidgetLimits<TypeParam>::Max(), std::numeric_limits<QtWidgetValueType>::max());
            break;
        }

        default:
            FAIL();
        }
    }
} // namespace UnitTest
