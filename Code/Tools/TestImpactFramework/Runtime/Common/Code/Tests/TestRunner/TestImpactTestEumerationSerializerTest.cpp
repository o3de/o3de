/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactTestUtils.h>

#include <TestRunner/Common/Enumeration/TestImpactTestEnumeration.h>
#include <TestRunner/Common/Enumeration/TestImpactTestEnumerationSerializer.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    const TestImpact::TestEnumeration EmptyTestEnum(AZStd::vector<TestImpact::TestEnumerationSuite>{});

    TEST(TestEnumerationSerializer, EmptyTestEnumerationSuites_ExpectTemptyTestEnumeration)
    {
        // Given an empty set of test enumeration suites
        // When the test enumeration is serialized and deserialized back again
        const AZStd::string serializedString = TestImpact::SerializeTestEnumeration(EmptyTestEnum);
        const TestImpact::TestEnumeration actualEnumeration = TestImpact::DeserializeTestEnumeration(serializedString);

        // Expect the actual test enumeration to match the expected test enumeration
        EXPECT_TRUE(actualEnumeration == EmptyTestEnum);
    }

    TEST(TestEnumerationSerializer, SerializeAndDeserializeSuitesForTestTargetA_ActualSuiteDataMatchesExpectedSuiteData)
    {
        // Given the test of test enumeration suites for Test target A
        const TestImpact::TestEnumeration expectedEnumeration = TestImpact::TestEnumeration(GetTestTargetATestEnumerationSuites());

        // When the test enumeration is serialized and deserialized back again
        const AZStd::string serializedString = TestImpact::SerializeTestEnumeration(expectedEnumeration);
        const TestImpact::TestEnumeration actualEnumeration = TestImpact::DeserializeTestEnumeration(serializedString);

        // Expect the actual test enumeration to match the expected test enumeration
        EXPECT_TRUE(actualEnumeration == expectedEnumeration);

        // Do not expect the actual test enumeration to match the empty test enumeration
        EXPECT_FALSE(actualEnumeration == EmptyTestEnum);
    }

    TEST(TestEnumerationSerializer, SerializeAndDeserializeSuitesForTestTargetB_ActualSuiteDataMatchesExpectedSuiteData)
    {
        // Given the test of test enumeration suites for Test target B
        const TestImpact::TestEnumeration expectedEnumeration = TestImpact::TestEnumeration(GetTestTargetBTestEnumerationSuites());

        // When the test enumeration is serialized and deserialized back again
        const AZStd::string serializedString = TestImpact::SerializeTestEnumeration(expectedEnumeration);
        const TestImpact::TestEnumeration actualEnumeration = TestImpact::DeserializeTestEnumeration(serializedString);

        // Expect the actual test enumeration to match the expected test enumeration
        EXPECT_TRUE(actualEnumeration == expectedEnumeration);

        // Do not expect the actual test enumeration to match the empty test enumeration
        EXPECT_FALSE(actualEnumeration == EmptyTestEnum);
    }

    TEST(TestEnumerationSerializer, SerializeAndDeserializeSuitesForTestTargetC_ActualSuiteDataMatchesExpectedSuiteData)
    {
        // Given the test of test enumeration suites for Test target B
        const TestImpact::TestEnumeration expectedEnumeration = TestImpact::TestEnumeration(GetTestTargetCTestEnumerationSuites());

        // When the test enumeration is serialized and deserialized back again
        const AZStd::string serializedString = TestImpact::SerializeTestEnumeration(expectedEnumeration);
        const TestImpact::TestEnumeration actualEnumeration = TestImpact::DeserializeTestEnumeration(serializedString);

        // Expect the actual test enumeration to match the expected test enumeration
        EXPECT_TRUE(actualEnumeration == expectedEnumeration);

        // Do not expect the actual test enumeration to match the empty test enumeration
        EXPECT_FALSE(actualEnumeration == EmptyTestEnum);
    }

    TEST(TestEnumerationSerializer, SerializeAndDeserializeSuitesForTestTargetD_ActualSuiteDataMatchesExpectedSuiteData)
    {
        // Given the test of test enumeration suites for Test target B
        const TestImpact::TestEnumeration expectedEnumeration = TestImpact::TestEnumeration(GetTestTargetDTestEnumerationSuites());

        // When the test enumeration is serialized and deserialized back again
        const AZStd::string serializedString = TestImpact::SerializeTestEnumeration(expectedEnumeration);
        const TestImpact::TestEnumeration actualEnumeration = TestImpact::DeserializeTestEnumeration(serializedString);

        // Expect the actual test enumeration to match the expected test enumeration
        EXPECT_TRUE(actualEnumeration == expectedEnumeration);

        // Do not expect the actual test enumeration to match the empty test enumeration
        EXPECT_FALSE(actualEnumeration == EmptyTestEnum);
    }
} // namespace UnitTest
