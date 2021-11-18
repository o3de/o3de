/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactTestUtils.h>

#include <Artifact/Factory/TestImpactTestRunSuiteFactory.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/tuple.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    TEST(GTestRunSuiteFactory, ParseEmptyString_ThrowsArtifactException)
    {
        // Given an empty string
        const AZStd::string rawRun;

        try
        {
            // When attempting to parse the empty suite
            const AZStd::vector<TestImpact::TestRunSuite> suites = TestImpact::GTest::TestRunSuitesFactory(rawRun);

            // Do not expect the parsing to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::ArtifactException& e)
        {
            // Expect a artifact exception
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(GTestRunSuiteFactory, ParseInvalidString_ThrowsArtifactException)
    {
        // Given an invalid string
        const AZStd::string rawRun = "!@?";

        try
        {
            // When attempting to parse the invalid suite
            const AZStd::vector<TestImpact::TestRunSuite> suites = TestImpact::GTest::TestRunSuitesFactory(rawRun);

            // Do not expect the parsing to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::ArtifactException& e)
        {
            // Expect a artifact exception
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(GTestRunSuiteFactory, ParseTestTargetA_ReturnsValidSuitesAndTests)
    {
        const AZStd::vector<TestImpact::TestRunSuite> expectedSuites = GetTestTargetATestRunSuites();

        // Given the raw run output of TestTargetA
        const AZStd::string rawRun =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<testsuites tests=\"10\" failures=\"1\" disabled=\"0\" errors=\"0\" timestamp=\"2021-03-26T19:02:37\" time=\"0.051\" name=\"AllTests\">"
            "  <testsuite name=\"TestCase\" tests=\"7\" failures=\"1\" disabled=\"0\" errors=\"0\" time=\"0.003\">"
            "    <testcase name=\"Test1_WillPass\" status=\"run\" time=\"0\" classname=\"TestCase\" />"
            "    <testcase name=\"Test2_WillPass\" status=\"run\" time=\"0\" classname=\"TestCase\" />"
            "    <testcase name=\"Test3_WillPass\" status=\"run\" time=\"0\" classname=\"TestCase\" />"
            "    <testcase name=\"Test4_WillPass\" status=\"run\" time=\"0\" classname=\"TestCase\" />"
            "    <testcase name=\"Test5_WillPass\" status=\"run\" time=\"0.001\" classname=\"TestCase\" />"
            "    <testcase name=\"Test6_WillPass\" status=\"run\" time=\"0\" classname=\"TestCase\" />"
            "    <testcase name=\"Test7_WillFail\" status=\"run\" time=\"0.001\" classname=\"TestCase\">"
            "      <failure message=\"C:\\Lumberyard\\Code\\Tools\\TestImpactFramework\\Runtime\\Code\\Tests\\TestTargetA\\Code\\Tests\\TestImpactTestTargetA.cpp:54&#x0A;Failed\" type=\"\"><![CDATA[C:\\Lumberyard\\Code\\Tools\\TestImpactFramework\\Runtime\\Code\\Tests\\TestTargetA\\Code\\Tests\\TestImpactTestTargetA.cpp:54"
            "Failed]]></failure>"
            "    </testcase>"
            "  </testsuite>"
            "  <testsuite name=\"TestFixture\" tests=\"3\" failures=\"0\" disabled=\"0\" errors=\"0\" time=\"0.038\">"
            "    <testcase name=\"Test1_WillPass\" status=\"run\" time=\"0.004\" classname=\"TestFixture\" />"
            "    <testcase name=\"Test2_WillPass\" status=\"run\" time=\"0\" classname=\"TestFixture\" />"
            "    <testcase name=\"Test3_WillPass\" status=\"run\" time=\"0.001\" classname=\"TestFixture\" />"
            "  </testsuite>"
            "</testsuites>"
            "";

        // When the raw run text is parsed
        const AZStd::vector<TestImpact::TestRunSuite> suites = TestImpact::GTest::TestRunSuitesFactory(rawRun);

        // Expect the generated suite data to match that of the raw run text
        EXPECT_TRUE(suites == expectedSuites);
    }

    TEST(GTestRunSuiteFactory, ParseTestTargetB_ReturnsValidSuitesAndTests)
    {
        const AZStd::vector<TestImpact::TestRunSuite> expectedSuites = GetTestTargetBTestRunSuites();

        // Given the raw run output of TestTargetB
        const AZStd::string rawRun =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<testsuites tests=\"112\" failures=\"0\" disabled=\"0\" errors=\"0\" timestamp=\"2021-03-27T11:56:14\" time=\"7.155\" name=\"AllTests\">"
            "  <testsuite name=\"TestCase\" tests=\"3\" failures=\"0\" disabled=\"0\" errors=\"0\" time=\"0.202\">"
            "    <testcase name=\"Test1_WillPass\" status=\"run\" time=\"0.003\" classname=\"TestCase\" />"
            "    <testcase name=\"Test2_WillPass\" status=\"run\" time=\"0\" classname=\"TestCase\" />"
            "    <testcase name=\"Test3_WillPass\" status=\"run\" time=\"0\" classname=\"TestCase\" />"
            "  </testsuite>"
            "  <testsuite name=\"TestFixture\" tests=\"1\" failures=\"0\" disabled=\"0\" errors=\"0\" time=\"0.062\">"
            "    <testcase name=\"Test1_WillPass\" status=\"run\" time=\"0.005\" classname=\"TestFixture\" />"
            "  </testsuite>"
            "  <testsuite name=\"PermutationA/TestFixtureWithParams\" tests=\"54\" failures=\"0\" disabled=\"0\" errors=\"0\" time=\"3.203\">"
            "    <testcase name=\"Test1_WillPass/0\" value_param=\"(1, &apos;\\x3&apos; (3), -0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/1\" value_param=\"(1, &apos;\\x3&apos; (3), 0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/2\" value_param=\"(1, &apos;\\x3&apos; (3), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/3\" value_param=\"(1, &apos;\\x5&apos; (5), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/4\" value_param=\"(1, &apos;\\x5&apos; (5), 0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/5\" value_param=\"(1, &apos;\\x5&apos; (5), 1)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/6\" value_param=\"(1, &apos;\\a&apos; (7), -0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/7\" value_param=\"(1, &apos;\\a&apos; (7), 0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/8\" value_param=\"(1, &apos;\\a&apos; (7), 1)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/9\" value_param=\"(2, &apos;\\x3&apos; (3), -0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/10\" value_param=\"(2, &apos;\\x3&apos; (3), 0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/11\" value_param=\"(2, &apos;\\x3&apos; (3), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/12\" value_param=\"(2, &apos;\\x5&apos; (5), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/13\" value_param=\"(2, &apos;\\x5&apos; (5), 0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/14\" value_param=\"(2, &apos;\\x5&apos; (5), 1)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/15\" value_param=\"(2, &apos;\\a&apos; (7), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/16\" value_param=\"(2, &apos;\\a&apos; (7), 0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/17\" value_param=\"(2, &apos;\\a&apos; (7), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/18\" value_param=\"(4, &apos;\\x3&apos; (3), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/19\" value_param=\"(4, &apos;\\x3&apos; (3), 0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/20\" value_param=\"(4, &apos;\\x3&apos; (3), 1)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/21\" value_param=\"(4, &apos;\\x5&apos; (5), -0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/22\" value_param=\"(4, &apos;\\x5&apos; (5), 0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/23\" value_param=\"(4, &apos;\\x5&apos; (5), 1)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/24\" value_param=\"(4, &apos;\\a&apos; (7), -0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/25\" value_param=\"(4, &apos;\\a&apos; (7), 0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/26\" value_param=\"(4, &apos;\\a&apos; (7), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/0\" value_param=\"(1, &apos;\\x3&apos; (3), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/1\" value_param=\"(1, &apos;\\x3&apos; (3), 0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/2\" value_param=\"(1, &apos;\\x3&apos; (3), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/3\" value_param=\"(1, &apos;\\x5&apos; (5), -0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/4\" value_param=\"(1, &apos;\\x5&apos; (5), 0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/5\" value_param=\"(1, &apos;\\x5&apos; (5), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/6\" value_param=\"(1, &apos;\\a&apos; (7), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/7\" value_param=\"(1, &apos;\\a&apos; (7), 0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/8\" value_param=\"(1, &apos;\\a&apos; (7), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/9\" value_param=\"(2, &apos;\\x3&apos; (3), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/10\" value_param=\"(2, &apos;\\x3&apos; (3), 0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/11\" value_param=\"(2, &apos;\\x3&apos; (3), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/12\" value_param=\"(2, &apos;\\x5&apos; (5), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/13\" value_param=\"(2, &apos;\\x5&apos; (5), 0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/14\" value_param=\"(2, &apos;\\x5&apos; (5), 1)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/15\" value_param=\"(2, &apos;\\a&apos; (7), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/16\" value_param=\"(2, &apos;\\a&apos; (7), 0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/17\" value_param=\"(2, &apos;\\a&apos; (7), 1)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/18\" value_param=\"(4, &apos;\\x3&apos; (3), -0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/19\" value_param=\"(4, &apos;\\x3&apos; (3), 0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/20\" value_param=\"(4, &apos;\\x3&apos; (3), 1)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/21\" value_param=\"(4, &apos;\\x5&apos; (5), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/22\" value_param=\"(4, &apos;\\x5&apos; (5), 0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/23\" value_param=\"(4, &apos;\\x5&apos; (5), 1)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/24\" value_param=\"(4, &apos;\\a&apos; (7), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/25\" value_param=\"(4, &apos;\\a&apos; (7), 0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/26\" value_param=\"(4, &apos;\\a&apos; (7), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams\" />"
            "  </testsuite>"
            "  <testsuite name=\"TestFixtureWithParams\" tests=\"54\" failures=\"0\" disabled=\"0\" errors=\"0\" time=\"3.36\">"
            "    <testcase name=\"Test1_WillPass/0\" value_param=\"(8, &apos;\\t&apos; (9), -10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/1\" value_param=\"(8, &apos;\\t&apos; (9), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/2\" value_param=\"(8, &apos;\\t&apos; (9), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/3\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/4\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/5\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/6\" value_param=\"(8, &apos;\\x11&apos; (17), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/7\" value_param=\"(8, &apos;\\x11&apos; (17), 0.05)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/8\" value_param=\"(8, &apos;\\x11&apos; (17), 10)\" status=\"run\" time=\"0.002\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/9\" value_param=\"(16, &apos;\\t&apos; (9), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/10\" value_param=\"(16, &apos;\\t&apos; (9), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/11\" value_param=\"(16, &apos;\\t&apos; (9), 10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/12\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/13\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/14\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/15\" value_param=\"(16, &apos;\\x11&apos; (17), -10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/16\" value_param=\"(16, &apos;\\x11&apos; (17), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/17\" value_param=\"(16, &apos;\\x11&apos; (17), 10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/18\" value_param=\"(32, &apos;\\t&apos; (9), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/19\" value_param=\"(32, &apos;\\t&apos; (9), 0.05)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/20\" value_param=\"(32, &apos;\\t&apos; (9), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/21\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), -10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/22\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/23\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/24\" value_param=\"(32, &apos;\\x11&apos; (17), -10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/25\" value_param=\"(32, &apos;\\x11&apos; (17), 0.05)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test1_WillPass/26\" value_param=\"(32, &apos;\\x11&apos; (17), 10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/0\" value_param=\"(8, &apos;\\t&apos; (9), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/1\" value_param=\"(8, &apos;\\t&apos; (9), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/2\" value_param=\"(8, &apos;\\t&apos; (9), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/3\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/4\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/5\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/6\" value_param=\"(8, &apos;\\x11&apos; (17), -10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/7\" value_param=\"(8, &apos;\\x11&apos; (17), 0.05)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/8\" value_param=\"(8, &apos;\\x11&apos; (17), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/9\" value_param=\"(16, &apos;\\t&apos; (9), -10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/10\" value_param=\"(16, &apos;\\t&apos; (9), 0.05)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/11\" value_param=\"(16, &apos;\\t&apos; (9), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/12\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), -10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/13\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/14\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/15\" value_param=\"(16, &apos;\\x11&apos; (17), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/16\" value_param=\"(16, &apos;\\x11&apos; (17), 0.05)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/17\" value_param=\"(16, &apos;\\x11&apos; (17), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/18\" value_param=\"(32, &apos;\\t&apos; (9), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/19\" value_param=\"(32, &apos;\\t&apos; (9), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/20\" value_param=\"(32, &apos;\\t&apos; (9), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/21\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/22\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/23\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/24\" value_param=\"(32, &apos;\\x11&apos; (17), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/25\" value_param=\"(32, &apos;\\x11&apos; (17), 0.05)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "    <testcase name=\"Test2_WillPass/26\" value_param=\"(32, &apos;\\x11&apos; (17), 10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams\" />"
            "  </testsuite>"
            "</testsuites>"
            "";

        // When the raw run text is parsed
        const AZStd::vector<TestImpact::TestRunSuite> suites = TestImpact::GTest::TestRunSuitesFactory(rawRun);

        // Expect the generated suite data to match that of the raw run text
        EXPECT_TRUE(suites == expectedSuites);
    }

    TEST(GTestRunSuiteFactory, ParseTestTargetC_ReturnsValidSuitesAndTests)
    {
        const AZStd::vector<TestImpact::TestRunSuite> expectedSuites = GetTestTargetCTestRunSuites();

        // Given the raw run output of TestTargetC
        const AZStd::string rawRun =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<testsuites tests=\"18\" failures=\"0\" disabled=\"0\" errors=\"0\" timestamp=\"2021-03-27T12:35:40\" time=\"1.022\" name=\"AllTests\">"
            "  <testsuite name=\"TestFixture\" tests=\"2\" failures=\"0\" disabled=\"0\" errors=\"0\" time=\"0.125\">"
            "    <testcase name=\"Test1_WillPass\" status=\"run\" time=\"0.004\" classname=\"TestFixture\" />"
            "    <testcase name=\"Test2_WillPass\" status=\"run\" time=\"0\" classname=\"TestFixture\" />"
            "  </testsuite>"
            "  <testsuite name=\"TestFixtureWithTypes/0\" tests=\"4\" failures=\"0\" disabled=\"0\" errors=\"0\" time=\"0.21\">"
            "    <testcase name=\"Test1_WillPass\" type_param=\"int\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes/0\" />"
            "    <testcase name=\"Test2_WillPass\" type_param=\"int\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithTypes/0\" />"
            "    <testcase name=\"Test3_WillPass\" type_param=\"int\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithTypes/0\" />"
            "    <testcase name=\"Test4_WillPass\" type_param=\"int\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithTypes/0\" />"
            "  </testsuite>"
            "  <testsuite name=\"TestFixtureWithTypes/1\" tests=\"4\" failures=\"0\" disabled=\"0\" errors=\"0\" time=\"0.208\">"
            "    <testcase name=\"Test1_WillPass\" type_param=\"float\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithTypes/1\" />"
            "    <testcase name=\"Test2_WillPass\" type_param=\"float\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithTypes/1\" />"
            "    <testcase name=\"Test3_WillPass\" type_param=\"float\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes/1\" />"
            "    <testcase name=\"Test4_WillPass\" type_param=\"float\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes/1\" />"
            "  </testsuite>"
            "  <testsuite name=\"TestFixtureWithTypes/2\" tests=\"4\" failures=\"0\" disabled=\"0\" errors=\"0\" time=\"0.199\">"
            "    <testcase name=\"Test1_WillPass\" type_param=\"double\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes/2\" />"
            "    <testcase name=\"Test2_WillPass\" type_param=\"double\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes/2\" />"
            "    <testcase name=\"Test3_WillPass\" type_param=\"double\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes/2\" />"
            "    <testcase name=\"Test4_WillPass\" type_param=\"double\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes/2\" />"
            "  </testsuite>"
            "  <testsuite name=\"TestFixtureWithTypes/3\" tests=\"4\" failures=\"0\" disabled=\"0\" errors=\"0\" time=\"0.049\">"
            "    <testcase name=\"Test1_WillPass\" type_param=\"char\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes/3\" />"
            "    <testcase name=\"Test2_WillPass\" type_param=\"char\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes/3\" />"
            "    <testcase name=\"Test3_WillPass\" type_param=\"char\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes/3\" />"
            "    <testcase name=\"Test4_WillPass\" type_param=\"char\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes/3\" />"
            "  </testsuite>"
            "</testsuites>"
            "";

        // When the raw run text is parsed
        const AZStd::vector<TestImpact::TestRunSuite> suites = TestImpact::GTest::TestRunSuitesFactory(rawRun);

        // Expect the generated suite data to match that of the raw run text
        EXPECT_TRUE(suites == expectedSuites);
    }

    TEST(GTestRunSuiteFactory, ParseTestTargetD_ReturnsValidSuitesAndTests)
    {
        const AZStd::vector<TestImpact::TestRunSuite> expectedSuites = GetTestTargetDTestRunSuites();

        // Given the raw run output of TestTargetD
        const AZStd::string rawRun =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<testsuites tests=\"249\" failures=\"0\" disabled=\"181\" errors=\"0\" timestamp=\"2021-03-25T15:18:40\" time=\"0.314\" name=\"AllTests\">"
            "  <testsuite name=\"TestCase\" tests=\"5\" failures=\"0\" disabled=\"1\" errors=\"0\" time=\"0.003\">"
            "    <testcase name=\"Test1_WillPass\" status=\"run\" time=\"0.001\" classname=\"TestCase\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass\" status=\"notrun\" time=\"0\" classname=\"TestCase\" />"
            "    <testcase name=\"Test3_WillPass\" status=\"run\" time=\"0\" classname=\"TestCase\" />"
            "    <testcase name=\"Test4_WillPass\" status=\"run\" time=\"0\" classname=\"TestCase\" />"
            "    <testcase name=\"Test5_WillPass\" status=\"run\" time=\"0\" classname=\"TestCase\" />"
            "  </testsuite>"
            "  <testsuite name=\"TestFixture1\" tests=\"2\" failures=\"0\" disabled=\"0\" errors=\"0\" time=\"0.004\">"
            "    <testcase name=\"Test1_WillPass\" status=\"run\" time=\"0.002\" classname=\"TestFixture1\" />"
            "    <testcase name=\"Test2_WillPass\" status=\"run\" time=\"0\" classname=\"TestFixture1\" />"
            "  </testsuite>"
            "  <testsuite name=\"DISABLED_TestFixture2\" tests=\"2\" failures=\"0\" disabled=\"2\" errors=\"0\" time=\"0\">"
            "    <testcase name=\"Test1_WillPass\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixture2\" />"
            "    <testcase name=\"Test2_WillPass\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixture2\" />"
            "  </testsuite>"
            "  <testsuite name=\"TestFixtureWithTypes1/0\" tests=\"3\" failures=\"0\" disabled=\"1\" errors=\"0\" time=\"0.001\">"
            "    <testcase name=\"Test1_WillPass\" type_param=\"int\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes1/0\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"int\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithTypes1/0\" />"
            "    <testcase name=\"Test3_WillPass\" type_param=\"int\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes1/0\" />"
            "  </testsuite>"
            "  <testsuite name=\"TestFixtureWithTypes1/1\" tests=\"3\" failures=\"0\" disabled=\"1\" errors=\"0\" time=\"0.003\">"
            "    <testcase name=\"Test1_WillPass\" type_param=\"float\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithTypes1/1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"float\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithTypes1/1\" />"
            "    <testcase name=\"Test3_WillPass\" type_param=\"float\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes1/1\" />"
            "  </testsuite>"
            "  <testsuite name=\"TestFixtureWithTypes1/2\" tests=\"3\" failures=\"0\" disabled=\"1\" errors=\"0\" time=\"0\">"
            "    <testcase name=\"Test1_WillPass\" type_param=\"double\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes1/2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"double\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithTypes1/2\" />"
            "    <testcase name=\"Test3_WillPass\" type_param=\"double\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes1/2\" />"
            "  </testsuite>"
            "  <testsuite name=\"TestFixtureWithTypes1/3\" tests=\"3\" failures=\"0\" disabled=\"1\" errors=\"0\" time=\"0.001\">"
            "    <testcase name=\"Test1_WillPass\" type_param=\"char\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes1/3\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"char\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithTypes1/3\" />"
            "    <testcase name=\"Test3_WillPass\" type_param=\"char\" status=\"run\" time=\"0\" classname=\"TestFixtureWithTypes1/3\" />"
            "  </testsuite>"
            "  <testsuite name=\"DISABLED_TestFixtureWithTypes2/0\" tests=\"3\" failures=\"0\" disabled=\"3\" errors=\"0\" time=\"0\">"
            "    <testcase name=\"Test1_WillPass\" type_param=\"int\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithTypes2/0\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"int\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithTypes2/0\" />"
            "    <testcase name=\"Test3_WillPass\" type_param=\"int\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithTypes2/0\" />"
            "  </testsuite>"
            "  <testsuite name=\"DISABLED_TestFixtureWithTypes2/1\" tests=\"3\" failures=\"0\" disabled=\"3\" errors=\"0\" time=\"0\">"
            "    <testcase name=\"Test1_WillPass\" type_param=\"float\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithTypes2/1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"float\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithTypes2/1\" />"
            "    <testcase name=\"Test3_WillPass\" type_param=\"float\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithTypes2/1\" />"
            "  </testsuite>"
            "  <testsuite name=\"DISABLED_TestFixtureWithTypes2/2\" tests=\"3\" failures=\"0\" disabled=\"3\" errors=\"0\" time=\"0\">"
            "    <testcase name=\"Test1_WillPass\" type_param=\"double\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithTypes2/2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"double\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithTypes2/2\" />"
            "    <testcase name=\"Test3_WillPass\" type_param=\"double\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithTypes2/2\" />"
            "  </testsuite>"
            "  <testsuite name=\"DISABLED_TestFixtureWithTypes2/3\" tests=\"3\" failures=\"0\" disabled=\"3\" errors=\"0\" time=\"0\">"
            "    <testcase name=\"Test1_WillPass\" type_param=\"char\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithTypes2/3\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"char\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithTypes2/3\" />"
            "    <testcase name=\"Test3_WillPass\" type_param=\"char\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithTypes2/3\" />"
            "  </testsuite>"
            "  <testsuite name=\"PermutationA/TestFixtureWithParams1\" tests=\"54\" failures=\"0\" disabled=\"27\" errors=\"0\" time=\"0.173\">"
            "    <testcase name=\"Test1_WillPass/0\" value_param=\"(1, &apos;\\x3&apos; (3), -0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/1\" value_param=\"(1, &apos;\\x3&apos; (3), 0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/2\" value_param=\"(1, &apos;\\x3&apos; (3), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/3\" value_param=\"(1, &apos;\\x5&apos; (5), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/4\" value_param=\"(1, &apos;\\x5&apos; (5), 0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/5\" value_param=\"(1, &apos;\\x5&apos; (5), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/6\" value_param=\"(1, &apos;\\a&apos; (7), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/7\" value_param=\"(1, &apos;\\a&apos; (7), 0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/8\" value_param=\"(1, &apos;\\a&apos; (7), 1)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/9\" value_param=\"(2, &apos;\\x3&apos; (3), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/10\" value_param=\"(2, &apos;\\x3&apos; (3), 0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/11\" value_param=\"(2, &apos;\\x3&apos; (3), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/12\" value_param=\"(2, &apos;\\x5&apos; (5), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/13\" value_param=\"(2, &apos;\\x5&apos; (5), 0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/14\" value_param=\"(2, &apos;\\x5&apos; (5), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/15\" value_param=\"(2, &apos;\\a&apos; (7), -0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/16\" value_param=\"(2, &apos;\\a&apos; (7), 0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/17\" value_param=\"(2, &apos;\\a&apos; (7), 1)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/18\" value_param=\"(4, &apos;\\x3&apos; (3), -0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/19\" value_param=\"(4, &apos;\\x3&apos; (3), 0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/20\" value_param=\"(4, &apos;\\x3&apos; (3), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/21\" value_param=\"(4, &apos;\\x5&apos; (5), -0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/22\" value_param=\"(4, &apos;\\x5&apos; (5), 0)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/23\" value_param=\"(4, &apos;\\x5&apos; (5), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/24\" value_param=\"(4, &apos;\\a&apos; (7), -0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/25\" value_param=\"(4, &apos;\\a&apos; (7), 0)\" status=\"run\" time=\"0.001\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/26\" value_param=\"(4, &apos;\\a&apos; (7), 1)\" status=\"run\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/0\" value_param=\"(1, &apos;\\x3&apos; (3), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/1\" value_param=\"(1, &apos;\\x3&apos; (3), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/2\" value_param=\"(1, &apos;\\x3&apos; (3), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/3\" value_param=\"(1, &apos;\\x5&apos; (5), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/4\" value_param=\"(1, &apos;\\x5&apos; (5), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/5\" value_param=\"(1, &apos;\\x5&apos; (5), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/6\" value_param=\"(1, &apos;\\a&apos; (7), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/7\" value_param=\"(1, &apos;\\a&apos; (7), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/8\" value_param=\"(1, &apos;\\a&apos; (7), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/9\" value_param=\"(2, &apos;\\x3&apos; (3), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/10\" value_param=\"(2, &apos;\\x3&apos; (3), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/11\" value_param=\"(2, &apos;\\x3&apos; (3), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/12\" value_param=\"(2, &apos;\\x5&apos; (5), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/13\" value_param=\"(2, &apos;\\x5&apos; (5), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/14\" value_param=\"(2, &apos;\\x5&apos; (5), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/15\" value_param=\"(2, &apos;\\a&apos; (7), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/16\" value_param=\"(2, &apos;\\a&apos; (7), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/17\" value_param=\"(2, &apos;\\a&apos; (7), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/18\" value_param=\"(4, &apos;\\x3&apos; (3), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/19\" value_param=\"(4, &apos;\\x3&apos; (3), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/20\" value_param=\"(4, &apos;\\x3&apos; (3), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/21\" value_param=\"(4, &apos;\\x5&apos; (5), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/22\" value_param=\"(4, &apos;\\x5&apos; (5), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/23\" value_param=\"(4, &apos;\\x5&apos; (5), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/24\" value_param=\"(4, &apos;\\a&apos; (7), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/25\" value_param=\"(4, &apos;\\a&apos; (7), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/26\" value_param=\"(4, &apos;\\a&apos; (7), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/TestFixtureWithParams1\" />"
            "  </testsuite>"
            "  <testsuite name=\"TestFixtureWithParams1\" tests=\"54\" failures=\"0\" disabled=\"27\" errors=\"0\" time=\"0.102\">"
            "    <testcase name=\"Test1_WillPass/0\" value_param=\"(8, &apos;\\t&apos; (9), -10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/1\" value_param=\"(8, &apos;\\t&apos; (9), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/2\" value_param=\"(8, &apos;\\t&apos; (9), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/3\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/4\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/5\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/6\" value_param=\"(8, &apos;\\x11&apos; (17), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/7\" value_param=\"(8, &apos;\\x11&apos; (17), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/8\" value_param=\"(8, &apos;\\x11&apos; (17), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/9\" value_param=\"(16, &apos;\\t&apos; (9), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/10\" value_param=\"(16, &apos;\\t&apos; (9), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/11\" value_param=\"(16, &apos;\\t&apos; (9), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/12\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), -10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/13\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/14\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/15\" value_param=\"(16, &apos;\\x11&apos; (17), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/16\" value_param=\"(16, &apos;\\x11&apos; (17), 0.05)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/17\" value_param=\"(16, &apos;\\x11&apos; (17), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/18\" value_param=\"(32, &apos;\\t&apos; (9), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/19\" value_param=\"(32, &apos;\\t&apos; (9), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/20\" value_param=\"(32, &apos;\\t&apos; (9), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/21\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/22\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/23\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/24\" value_param=\"(32, &apos;\\x11&apos; (17), -10)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/25\" value_param=\"(32, &apos;\\x11&apos; (17), 0.05)\" status=\"run\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"Test1_WillPass/26\" value_param=\"(32, &apos;\\x11&apos; (17), 10)\" status=\"run\" time=\"0.001\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/0\" value_param=\"(8, &apos;\\t&apos; (9), -10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/1\" value_param=\"(8, &apos;\\t&apos; (9), 0.05)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/2\" value_param=\"(8, &apos;\\t&apos; (9), 10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/3\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), -10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/4\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/5\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/6\" value_param=\"(8, &apos;\\x11&apos; (17), -10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/7\" value_param=\"(8, &apos;\\x11&apos; (17), 0.05)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/8\" value_param=\"(8, &apos;\\x11&apos; (17), 10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/9\" value_param=\"(16, &apos;\\t&apos; (9), -10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/10\" value_param=\"(16, &apos;\\t&apos; (9), 0.05)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/11\" value_param=\"(16, &apos;\\t&apos; (9), 10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/12\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), -10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/13\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/14\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/15\" value_param=\"(16, &apos;\\x11&apos; (17), -10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/16\" value_param=\"(16, &apos;\\x11&apos; (17), 0.05)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/17\" value_param=\"(16, &apos;\\x11&apos; (17), 10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/18\" value_param=\"(32, &apos;\\t&apos; (9), -10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/19\" value_param=\"(32, &apos;\\t&apos; (9), 0.05)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/20\" value_param=\"(32, &apos;\\t&apos; (9), 10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/21\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), -10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/22\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/23\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/24\" value_param=\"(32, &apos;\\x11&apos; (17), -10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/25\" value_param=\"(32, &apos;\\x11&apos; (17), 0.05)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/26\" value_param=\"(32, &apos;\\x11&apos; (17), 10)\" status=\"notrun\" time=\"0\" classname=\"TestFixtureWithParams1\" />"
            "  </testsuite>"
            "  <testsuite name=\"PermutationA/DISABLED_TestFixtureWithParams2\" tests=\"54\" failures=\"0\" disabled=\"54\" errors=\"0\" time=\"0\">"
            "    <testcase name=\"Test1_WillPass/0\" value_param=\"(1, &apos;\\x3&apos; (3), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/1\" value_param=\"(1, &apos;\\x3&apos; (3), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/2\" value_param=\"(1, &apos;\\x3&apos; (3), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/3\" value_param=\"(1, &apos;\\x5&apos; (5), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/4\" value_param=\"(1, &apos;\\x5&apos; (5), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/5\" value_param=\"(1, &apos;\\x5&apos; (5), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/6\" value_param=\"(1, &apos;\\a&apos; (7), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/7\" value_param=\"(1, &apos;\\a&apos; (7), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/8\" value_param=\"(1, &apos;\\a&apos; (7), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/9\" value_param=\"(2, &apos;\\x3&apos; (3), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/10\" value_param=\"(2, &apos;\\x3&apos; (3), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/11\" value_param=\"(2, &apos;\\x3&apos; (3), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/12\" value_param=\"(2, &apos;\\x5&apos; (5), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/13\" value_param=\"(2, &apos;\\x5&apos; (5), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/14\" value_param=\"(2, &apos;\\x5&apos; (5), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/15\" value_param=\"(2, &apos;\\a&apos; (7), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/16\" value_param=\"(2, &apos;\\a&apos; (7), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/17\" value_param=\"(2, &apos;\\a&apos; (7), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/18\" value_param=\"(4, &apos;\\x3&apos; (3), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/19\" value_param=\"(4, &apos;\\x3&apos; (3), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/20\" value_param=\"(4, &apos;\\x3&apos; (3), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/21\" value_param=\"(4, &apos;\\x5&apos; (5), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/22\" value_param=\"(4, &apos;\\x5&apos; (5), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/23\" value_param=\"(4, &apos;\\x5&apos; (5), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/24\" value_param=\"(4, &apos;\\a&apos; (7), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/25\" value_param=\"(4, &apos;\\a&apos; (7), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/26\" value_param=\"(4, &apos;\\a&apos; (7), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/0\" value_param=\"(1, &apos;\\x3&apos; (3), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/1\" value_param=\"(1, &apos;\\x3&apos; (3), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/2\" value_param=\"(1, &apos;\\x3&apos; (3), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/3\" value_param=\"(1, &apos;\\x5&apos; (5), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/4\" value_param=\"(1, &apos;\\x5&apos; (5), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/5\" value_param=\"(1, &apos;\\x5&apos; (5), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/6\" value_param=\"(1, &apos;\\a&apos; (7), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/7\" value_param=\"(1, &apos;\\a&apos; (7), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/8\" value_param=\"(1, &apos;\\a&apos; (7), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/9\" value_param=\"(2, &apos;\\x3&apos; (3), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/10\" value_param=\"(2, &apos;\\x3&apos; (3), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/11\" value_param=\"(2, &apos;\\x3&apos; (3), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/12\" value_param=\"(2, &apos;\\x5&apos; (5), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/13\" value_param=\"(2, &apos;\\x5&apos; (5), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/14\" value_param=\"(2, &apos;\\x5&apos; (5), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/15\" value_param=\"(2, &apos;\\a&apos; (7), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/16\" value_param=\"(2, &apos;\\a&apos; (7), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/17\" value_param=\"(2, &apos;\\a&apos; (7), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/18\" value_param=\"(4, &apos;\\x3&apos; (3), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/19\" value_param=\"(4, &apos;\\x3&apos; (3), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/20\" value_param=\"(4, &apos;\\x3&apos; (3), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/21\" value_param=\"(4, &apos;\\x5&apos; (5), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/22\" value_param=\"(4, &apos;\\x5&apos; (5), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/23\" value_param=\"(4, &apos;\\x5&apos; (5), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/24\" value_param=\"(4, &apos;\\a&apos; (7), -0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/25\" value_param=\"(4, &apos;\\a&apos; (7), 0)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/26\" value_param=\"(4, &apos;\\a&apos; (7), 1)\" status=\"notrun\" time=\"0\" classname=\"PermutationA/DISABLED_TestFixtureWithParams2\" />"
            "  </testsuite>"
            "  <testsuite name=\"DISABLED_TestFixtureWithParams2\" tests=\"54\" failures=\"0\" disabled=\"54\" errors=\"0\" time=\"0\">"
            "    <testcase name=\"Test1_WillPass/0\" value_param=\"(8, &apos;\\t&apos; (9), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/1\" value_param=\"(8, &apos;\\t&apos; (9), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/2\" value_param=\"(8, &apos;\\t&apos; (9), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/3\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/4\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/5\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/6\" value_param=\"(8, &apos;\\x11&apos; (17), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/7\" value_param=\"(8, &apos;\\x11&apos; (17), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/8\" value_param=\"(8, &apos;\\x11&apos; (17), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/9\" value_param=\"(16, &apos;\\t&apos; (9), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/10\" value_param=\"(16, &apos;\\t&apos; (9), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/11\" value_param=\"(16, &apos;\\t&apos; (9), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/12\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/13\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/14\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/15\" value_param=\"(16, &apos;\\x11&apos; (17), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/16\" value_param=\"(16, &apos;\\x11&apos; (17), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/17\" value_param=\"(16, &apos;\\x11&apos; (17), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/18\" value_param=\"(32, &apos;\\t&apos; (9), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/19\" value_param=\"(32, &apos;\\t&apos; (9), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/20\" value_param=\"(32, &apos;\\t&apos; (9), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/21\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/22\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/23\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/24\" value_param=\"(32, &apos;\\x11&apos; (17), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/25\" value_param=\"(32, &apos;\\x11&apos; (17), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"Test1_WillPass/26\" value_param=\"(32, &apos;\\x11&apos; (17), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/0\" value_param=\"(8, &apos;\\t&apos; (9), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/1\" value_param=\"(8, &apos;\\t&apos; (9), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/2\" value_param=\"(8, &apos;\\t&apos; (9), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/3\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/4\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/5\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/6\" value_param=\"(8, &apos;\\x11&apos; (17), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/7\" value_param=\"(8, &apos;\\x11&apos; (17), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/8\" value_param=\"(8, &apos;\\x11&apos; (17), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/9\" value_param=\"(16, &apos;\\t&apos; (9), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/10\" value_param=\"(16, &apos;\\t&apos; (9), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/11\" value_param=\"(16, &apos;\\t&apos; (9), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/12\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/13\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/14\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/15\" value_param=\"(16, &apos;\\x11&apos; (17), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/16\" value_param=\"(16, &apos;\\x11&apos; (17), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/17\" value_param=\"(16, &apos;\\x11&apos; (17), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/18\" value_param=\"(32, &apos;\\t&apos; (9), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/19\" value_param=\"(32, &apos;\\t&apos; (9), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/20\" value_param=\"(32, &apos;\\t&apos; (9), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/21\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/22\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/23\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/24\" value_param=\"(32, &apos;\\x11&apos; (17), -10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/25\" value_param=\"(32, &apos;\\x11&apos; (17), 0.05)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "    <testcase name=\"DISABLED_Test2_WillPass/26\" value_param=\"(32, &apos;\\x11&apos; (17), 10)\" status=\"notrun\" time=\"0\" classname=\"DISABLED_TestFixtureWithParams2\" />"
            "  </testsuite>"
            "</testsuites>"
            "";

        // When the raw run text is parsed
        const AZStd::vector<TestImpact::TestRunSuite> suites = TestImpact::GTest::TestRunSuitesFactory(rawRun);

        // Expect the generated suite data to match that of the raw run text
        EXPECT_TRUE(suites == expectedSuites);
    }

    TEST(PyTestRunSuiteFactory, FooBar)
    {
        const AZStd::string rawRun =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<testsuites>\n"
            "  <testsuite errors=\"1\" failures=\"1\" hostname=\"LHR14-3497F632\" name=\"pytest\" skipped=\"1\" tests=\"4\" "
            "time=\"378.792\" timestamp=\"2021-10-28T09:55:42.050709\">\n"
            "    <properties>\n"
            "      <property name=\"timestamp\" value=\"2021-10-28T09-55-42-078715\"/>\n"
            "      <property name=\"hostname\" value=\"LHR14-3497F632\"/>\n"
            "      <property name=\"username\" value=\"jonawals\"/>\n"
            "      <property name=\"build\" value=\"windows\"/>\n"
            "    </properties>\n"
            "    <testcase classname=\"AutomatedTesting.Gem.PythonTests.Atom.TestSuite_Main.TestAtomEditorComponentsMain\" "
            "file=\"AutomatedTesting\\Gem\\PythonTests\\Atom\\TestSuite_Main.py\" line=\"27\" "
            "name=\"test_AtomEditorComponents_AddedToEntity[windows-auto_test-windows_editor-AutomatedTesting]\" time=\"128.487\">\n"
            "      <properties>\n"
            "        <property name=\"timestamp\" value=\"2021-10-28T09-55-42-078715\"/>\n"
            "        <property name=\"log\" value=\"TestSuite_Main_TestAtomEditorComponentsMain_test_AtomEditorC-logs.zip\"/>\n"
            "      </properties>\n"
            "    </testcase>\n"
            "    <testcase classname=\"AutomatedTesting.Gem.PythonTests.Atom.TestSuite_Main.TestAtomEditorComponentsMain\" "
            "file=\"AutomatedTesting\\Gem\\PythonTests\\Atom\\TestSuite_Main.py\" line=\"199\" "
            "name=\"test_AtomEditorComponents_LightComponent[windows-auto_test-windows_editor-AutomatedTesting]\">\n"
            "      <skipped/>\n"
            "    </testcase>\n"
            "    <testcase classname=\"AutomatedTesting.Gem.PythonTests.Atom.TestSuite_Main.TestMaterialEditorBasicTests\" "
            "file=\"AutomatedTesting\\Gem\\PythonTests\\Atom\\TestSuite_Main.py\" line=\"284\" "
            "name=\"test_MaterialEditorBasicTests[windows-MaterialEditor-windows_generic-AutomatedTesting]\" time=\"175.259\">\n"
            "      <failure message=\"AssertionError: Did not get idle state from AP, message was instead: error_[WinError 10054] An existing connection was forcibly closed by the remote host\">Some failure message</failure>\n"
            "    </testcase>\n"
            "    <testcase classname=\"AutomatedTesting.Gem.PythonTests.Atom.TestSuite_Main.TestMaterialEditorBasicTests\" "
            "file=\"AutomatedTesting\\Gem\\PythonTests\\Atom\\TestSuite_Main.py\" line=\"284\" "
            "name=\"test_Dummy[windows-MaterialEditor-windows_generic-AutomatedTesting]\" time=\"175.259\">\n"
            "      <error message=\"Hello\">Some error message</error>\n"
            "    </testcase>\n"
            "  </testsuite>\n"
            "</testsuites>";

        AZ_Printf("Here\n", rawRun.c_str());

        const AZStd::vector<TestImpact::TestRunSuite> suites = TestImpact::JUnit::TestRunSuitesFactory(rawRun);
    }
} // namespace UnitTest
