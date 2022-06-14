/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/PropertyEditor/PropertyIntCtrlCommon.h>
#include <AzToolsFramework/UI/PropertyEditor/QtWidgetLimits.h>
#include <Tests/IntegerPrimtitiveTestConfig.h>

namespace UnitTest
{
    using namespace AzToolsFramework;

    template<typename ValueType>
    struct PropertyIntCtrlCommonFixture
        : public ToolsApplicationFixture
    {

    };

    TYPED_TEST_CASE(PropertyIntCtrlCommonFixture, IntegerPrimtitiveTestConfigs);

    TYPED_TEST(PropertyIntCtrlCommonFixture, ValidMinValue_ExpectSafeValueEqualToOriginalValue)
    {
        // Given a valid value for the minimum attribute
        AZ::s64 value = aznumeric_cast<AZ::s64>(QtWidgetLimits<TypeParam>::Min());

        // Attempt to get a safe value in the Qt range
        AZ::s64 result = GetSafeAttributeValue<TypeParam>(value, "Test Property", "Test Attribute");

        // Expect the result to be equal the original value
        EXPECT_EQ(result, value);
    }

    TYPED_TEST(PropertyIntCtrlCommonFixture, InvalidMinValue_ExpectSafeValueEqualToValueTypeMinLimit)
    {
        // Given an invalid value for the minimum attribute
        AZ::s64 value = aznumeric_cast<AZ::s64>(QtWidgetLimits<TypeParam>::Min()) - 1;

        // Attempt to get a safe value in the Qt range
        AZ::s64 result = GetSafeAttributeValue<TypeParam>(value, "Test Property", "Test Attribute");

        // Expect the result to be equal to the limit for this value type
        EXPECT_EQ(result, QtWidgetLimits<TypeParam>::Min());
    }

    TYPED_TEST(PropertyIntCtrlCommonFixture, ValidMaxValue_ExpectSafeValueEqualToOriginalValue)
    {
        // Given a valid value for the maximum attribute
        AZ::s64 value = aznumeric_cast<AZ::s64>(QtWidgetLimits<TypeParam>::Max());

        // Attempt to get a safe value in the Qt range
        AZ::s64 result = GetSafeAttributeValue<TypeParam>(value, "Test Property", "Test Attribute");

        // Expect the result to be equal the original value
        EXPECT_EQ(result, value);
    }

    TYPED_TEST(PropertyIntCtrlCommonFixture, InvalidMaxValue_ExpectSafeValueEqualToValueTypeMinLimit)
    {
        // Given an invalid value for the maximum attribute
        AZ::s64 value = aznumeric_cast<AZ::s64>(QtWidgetLimits<TypeParam>::Min()) - 1;

        // Attempt to get a safe value in the Qt range
        AZ::s64 result = GetSafeAttributeValue<TypeParam>(value, "Test Property", "Test Attribute");

        // Expect the result to be equal to the limit for this value type
        EXPECT_EQ(result, QtWidgetLimits<TypeParam>::Min());
    }
} // namespace UnitTest
