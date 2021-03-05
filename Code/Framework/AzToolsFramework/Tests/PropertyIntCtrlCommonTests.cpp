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

#include "IntegerPrimtitiveTestConfig.h"

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
