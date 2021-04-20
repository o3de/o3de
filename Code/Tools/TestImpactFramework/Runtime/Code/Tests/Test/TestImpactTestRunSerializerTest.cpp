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

#include <TestImpactTestUtils.h>

#include <Test/Run/TestImpactTestRun.h>
#include <Test/Run/TestImpactTestRunSerializer.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    const TestImpact::TestRun EmptyTestRun(AZStd::vector<TestImpact::TestRunSuite>{}, AZStd::chrono::milliseconds{0});

    TEST(TestRunSerializer, EmptyTestRunSuites_ExpectTemptyTestRun)
    {
        // Given an empty set of test run suites
        // When the test run is serialized and deserialized back again
        const auto serializedString = TestImpact::SerializeTestRun(EmptyTestRun);
        const auto actualRun = TestImpact::DeserializeTestRun(serializedString);

        // Expect the actual test run to match the expected test run
        EXPECT_TRUE(actualRun == EmptyTestRun);
    }

    TEST(TestRunSerializer, SerializeAndDeserializeSuitesForTestTargetA_ActualSuiteDataMatchesExpectedSuiteData)
    {
        // Given the test of test run suites for Test target A
        const auto expectedRun = TestImpact::TestRun(GetTestTargetATestRunSuites(), AZStd::chrono::milliseconds{500});

        // When the test run is serialized and deserialized back again
        const auto serializedString = TestImpact::SerializeTestRun(expectedRun);
        const auto actualRun = TestImpact::DeserializeTestRun(serializedString);

        // Expect the actual test run to match the expected test run
        EXPECT_TRUE(actualRun == expectedRun);

        // Do not expect the actual test run to match the empty test run
        EXPECT_FALSE(actualRun == EmptyTestRun);
    }

    TEST(TestRunSerializer, SerializeAndDeserializeSuitesForTestTargetB_ActualSuiteDataMatchesExpectedSuiteData)
    {
        // Given the test of test run suites for Test target B
        const auto expectedRun = TestImpact::TestRun(GetTestTargetBTestRunSuites(), AZStd::chrono::milliseconds{500});

        // When the test run is serialized and deserialized back again
        const auto serializedString = TestImpact::SerializeTestRun(expectedRun);
        const auto actualRun = TestImpact::DeserializeTestRun(serializedString);

        // Expect the actual test run to match the expected test run
        EXPECT_TRUE(actualRun == expectedRun);

        // Do not expect the actual test run to match the empty test run
        EXPECT_FALSE(actualRun == EmptyTestRun);
    }

    TEST(TestRunSerializer, SerializeAndDeserializeSuitesForTestTargetC_ActualSuiteDataMatchesExpectedSuiteData)
    {
        // Given the test of test run suites for Test target B
        const auto expectedRun = TestImpact::TestRun(GetTestTargetCTestRunSuites(), AZStd::chrono::milliseconds{500});

        // When the test run is serialized and deserialized back again
        const auto serializedString = TestImpact::SerializeTestRun(expectedRun);
        const auto actualRun = TestImpact::DeserializeTestRun(serializedString);

        // Expect the actual test run to match the expected test run
        EXPECT_TRUE(actualRun == expectedRun);

        // Do not expect the actual test run to match the empty test run
        EXPECT_FALSE(actualRun == EmptyTestRun);
    }

    TEST(TestRunSerializer, SerializeAndDeserializeSuitesForTestTargetD_ActualSuiteDataMatchesExpectedSuiteData)
    {
        // Given the test of test run suites for Test target B
        const auto expectedRun = TestImpact::TestRun(GetTestTargetDTestRunSuites(), AZStd::chrono::milliseconds{500});

        // When the test run is serialized and deserialized back again
        const auto serializedString = TestImpact::SerializeTestRun(expectedRun);
        const auto actualRun = TestImpact::DeserializeTestRun(serializedString);

        // Expect the actual test run to match the expected test run
        EXPECT_TRUE(actualRun == expectedRun);

        // Do not expect the actual test run to match the empty test run
        EXPECT_FALSE(actualRun == EmptyTestRun);
    }
} // namespace UnitTest
