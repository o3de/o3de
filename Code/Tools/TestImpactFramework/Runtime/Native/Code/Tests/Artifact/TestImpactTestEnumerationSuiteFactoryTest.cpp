/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactTestUtils.h>

#include <Artifact/Factory/TestImpactTestEnumerationSuiteFactory.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/tuple.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    TEST(GTestEnumerationSuiteFactory, ParseEmptyString_ThrowsArtifactException)
    {
        // Given an empty string
        const AZStd::string rawEnum;

        try
        {
            // When attempting to parse the empty suite
            const AZStd::vector<TestImpact::TestEnumerationSuite> suites = TestImpact::GTest::TestEnumerationSuitesFactory(rawEnum);

            // Do not expect the parsing to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::ArtifactException& e)
        {
            // Expect an artifact exception
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(GTestEnumerationSuiteFactory, ParseInvalidString_ThrowsArtifactException)
    {
        // Given an invalid string
        const AZStd::string rawEnum = "!@?";

        try
        {
            // When attempting to parse the empty suite
            const AZStd::vector<TestImpact::TestEnumerationSuite> suites = TestImpact::GTest::TestEnumerationSuitesFactory(rawEnum);

            // Do not expect the parsing to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::ArtifactException& e)
        {
            // Expect an artifact exception
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(GTestEnumerationSuiteFactory, ParseTestTargetA_ReturnsValidSuitesAndTests)
    {
        const AZStd::vector<TestImpact::TestEnumerationSuite> expectedSuites = GetTestTargetATestEnumerationSuites();

        // Given the raw enum output of TestTargetA
        const AZStd::string rawEnum =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<testsuites tests=\"10\" name=\"AllTests\">\n"
            "  <testsuite name=\"TestCase\" tests=\"7\">\n"
            "    <testcase name=\"Test1_WillPass\" file=\"C:\\Tests\\TestImpactTestTargetA.cpp\" line=\"22\" />\n"
            "    <testcase name=\"Test2_WillPass\" file=\"C:\\Tests\\TestImpactTestTargetA.cpp\" line=\"27\" />\n"
            "    <testcase name=\"Test3_WillPass\" file=\"C:\\Tests\\TestImpactTestTargetA.cpp\" line=\"32\" />\n"
            "    <testcase name=\"Test4_WillPass\" file=\"C:\\Tests\\TestImpactTestTargetA.cpp\" line=\"37\" />\n"
            "    <testcase name=\"Test5_WillPass\" file=\"C:\\Tests\\TestImpactTestTargetA.cpp\" line=\"42\" />\n"
            "    <testcase name=\"Test6_WillPass\" file=\"C:\\Tests\\TestImpactTestTargetA.cpp\" line=\"47\" />\n"
            "    <testcase name=\"Test7_WillFail\" file=\"C:\\Tests\\TestImpactTestTargetA.cpp\" line=\"52\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"TestFixture\" tests=\"3\">\n"
            "    <testcase name=\"Test1_WillPass\" file=\"C:\\Tests\\TestTargetA\\Code\\Tests\\TestImpactTestTargetA.cpp\" line=\"57\" />\n"
            "    <testcase name=\"Test2_WillPass\" file=\"C:\\Tests\\TestTargetA\\Code\\Tests\\TestImpactTestTargetA.cpp\" line=\"62\" />\n"
            "    <testcase name=\"Test3_WillPass\" file=\"C:\\Tests\\TestTargetA\\Code\\Tests\\TestImpactTestTargetA.cpp\" line=\"67\" />\n"
            "  </testsuite>\n"
            "</testsuites>\n"
            "\n";

        // When the raw enumeration text is parsed
        const AZStd::vector<TestImpact::TestEnumerationSuite> suites = TestImpact::GTest::TestEnumerationSuitesFactory(rawEnum);

        // Expect the generated suite data to match that of the raw enum text
        EXPECT_TRUE(suites == expectedSuites);
    }

    TEST(GTestEnumerationSuiteFactory, ParseTestTargetB_ReturnsValidSuitesAndTests)
    {
        const AZStd::vector<TestImpact::TestEnumerationSuite> expectedSuites = GetTestTargetBTestEnumerationSuites();

        // Given the raw enum output of TestTargetB
        const AZStd::string rawEnum =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<testsuites tests=\"112\" name=\"AllTests\">\n"
            "  <testsuite name=\"TestCase\" tests=\"3\">\n"
            "    <testcase name=\"Test1_WillPass\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"29\" />\n"
            "    <testcase name=\"Test2_WillPass\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"34\" />\n"
            "    <testcase name=\"Test3_WillPass\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"39\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"TestFixture\" tests=\"1\">\n"
            "    <testcase name=\"Test1_WillPass\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"44\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"PermutationA/TestFixtureWithParams\" tests=\"54\">\n"
            "    <testcase name=\"Test1_WillPass/0\" value_param=\"(1, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/1\" value_param=\"(1, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/2\" value_param=\"(1, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/3\" value_param=\"(1, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/4\" value_param=\"(1, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/5\" value_param=\"(1, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/6\" value_param=\"(1, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/7\" value_param=\"(1, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/8\" value_param=\"(1, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/9\" value_param=\"(2, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/10\" value_param=\"(2, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/11\" value_param=\"(2, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/12\" value_param=\"(2, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/13\" value_param=\"(2, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/14\" value_param=\"(2, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/15\" value_param=\"(2, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/16\" value_param=\"(2, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/17\" value_param=\"(2, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/18\" value_param=\"(4, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/19\" value_param=\"(4, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/20\" value_param=\"(4, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/21\" value_param=\"(4, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/22\" value_param=\"(4, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/23\" value_param=\"(4, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/24\" value_param=\"(4, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/25\" value_param=\"(4, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/26\" value_param=\"(4, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/0\" value_param=\"(1, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/1\" value_param=\"(1, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/2\" value_param=\"(1, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/3\" value_param=\"(1, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/4\" value_param=\"(1, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/5\" value_param=\"(1, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/6\" value_param=\"(1, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/7\" value_param=\"(1, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/8\" value_param=\"(1, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/9\" value_param=\"(2, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/10\" value_param=\"(2, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/11\" value_param=\"(2, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/12\" value_param=\"(2, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/13\" value_param=\"(2, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/14\" value_param=\"(2, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/15\" value_param=\"(2, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/16\" value_param=\"(2, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/17\" value_param=\"(2, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/18\" value_param=\"(4, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/19\" value_param=\"(4, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/20\" value_param=\"(4, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/21\" value_param=\"(4, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/22\" value_param=\"(4, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/23\" value_param=\"(4, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/24\" value_param=\"(4, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/25\" value_param=\"(4, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/26\" value_param=\"(4, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"TestFixtureWithParams\" tests=\"54\">\n"
            "    <testcase name=\"Test1_WillPass/0\" value_param=\"(8, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/1\" value_param=\"(8, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/2\" value_param=\"(8, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/3\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/4\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/5\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/6\" value_param=\"(8, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/7\" value_param=\"(8, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/8\" value_param=\"(8, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/9\" value_param=\"(16, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/10\" value_param=\"(16, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/11\" value_param=\"(16, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/12\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/13\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/14\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/15\" value_param=\"(16, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/16\" value_param=\"(16, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/17\" value_param=\"(16, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/18\" value_param=\"(32, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/19\" value_param=\"(32, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/20\" value_param=\"(32, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/21\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/22\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/23\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/24\" value_param=\"(32, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/25\" value_param=\"(32, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test1_WillPass/26\" value_param=\"(32, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/0\" value_param=\"(8, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/1\" value_param=\"(8, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/2\" value_param=\"(8, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/3\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/4\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/5\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/6\" value_param=\"(8, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/7\" value_param=\"(8, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/8\" value_param=\"(8, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/9\" value_param=\"(16, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/10\" value_param=\"(16, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/11\" value_param=\"(16, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/12\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/13\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/14\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/15\" value_param=\"(16, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/16\" value_param=\"(16, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/17\" value_param=\"(16, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/18\" value_param=\"(32, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/19\" value_param=\"(32, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/20\" value_param=\"(32, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/21\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/22\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/23\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/24\" value_param=\"(32, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/25\" value_param=\"(32, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "    <testcase name=\"Test2_WillPass/26\" value_param=\"(32, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestImpactTestTargetB.cpp\" line=\"49\" />\n"
            "  </testsuite>\n"
            "</testsuites>\n"
            "\n";

        // When the raw enumeration text is parsed
        const AZStd::vector<TestImpact::TestEnumerationSuite> suites = TestImpact::GTest::TestEnumerationSuitesFactory(rawEnum);

        // Expect the generated suite data to match that of the raw enum text
        EXPECT_TRUE(suites == expectedSuites);
    }

    TEST(GTestEnumerationSuiteFactory, ParseTestTargetC_ReturnsValidSuitesAndTests)
    {
        const AZStd::vector<TestImpact::TestEnumerationSuite> expectedSuites = GetTestTargetCTestEnumerationSuites();

        // Given the raw enum output of TestTargetC
        const AZStd::string rawEnum =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<testsuites tests=\"18\" name=\"AllTests\">\n"
            "  <testsuite name=\"TestFixture\" tests=\"2\">\n"
            "    <testcase name=\"Test1_WillPass\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"32\" />\n"
            "    <testcase name=\"Test2_WillPass\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"37\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"TestFixtureWithTypes/0\" tests=\"4\">\n"
            "    <testcase name=\"Test1_WillPass\" type_param=\"int\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"42\" />\n"
            "    <testcase name=\"Test2_WillPass\" type_param=\"int\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"47\" />\n"
            "    <testcase name=\"Test3_WillPass\" type_param=\"int\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"52\" />\n"
            "    <testcase name=\"Test4_WillPass\" type_param=\"int\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"57\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"TestFixtureWithTypes/1\" tests=\"4\">\n"
            "    <testcase name=\"Test1_WillPass\" type_param=\"float\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"42\" />\n"
            "    <testcase name=\"Test2_WillPass\" type_param=\"float\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"47\" />\n"
            "    <testcase name=\"Test3_WillPass\" type_param=\"float\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"52\" />\n"
            "    <testcase name=\"Test4_WillPass\" type_param=\"float\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"57\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"TestFixtureWithTypes/2\" tests=\"4\">\n"
            "    <testcase name=\"Test1_WillPass\" type_param=\"double\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"42\" />\n"
            "    <testcase name=\"Test2_WillPass\" type_param=\"double\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"47\" />\n"
            "    <testcase name=\"Test3_WillPass\" type_param=\"double\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"52\" />\n"
            "    <testcase name=\"Test4_WillPass\" type_param=\"double\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"57\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"TestFixtureWithTypes/3\" tests=\"4\">\n"
            "    <testcase name=\"Test1_WillPass\" type_param=\"char\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"42\" />\n"
            "    <testcase name=\"Test2_WillPass\" type_param=\"char\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"47\" />\n"
            "    <testcase name=\"Test3_WillPass\" type_param=\"char\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"52\" />\n"
            "    <testcase name=\"Test4_WillPass\" type_param=\"char\" file=\"C:\\Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp\" line=\"57\" />\n"
            "  </testsuite>\n"
            "</testsuites>\n"
            "\n";

        // When the raw enumeration text is parsed
        const AZStd::vector<TestImpact::TestEnumerationSuite> suites = TestImpact::GTest::TestEnumerationSuitesFactory(rawEnum);

        // Expect the generated suite data to match that of the raw enum text
        EXPECT_TRUE(suites == expectedSuites);
    }

    TEST(GTestEnumerationSuiteFactory, ParseTestTargetD_ReturnsValidSuitesAndTests)
    {
        const AZStd::vector<TestImpact::TestEnumerationSuite> expectedSuites = GetTestTargetDTestEnumerationSuites();

        // Given the raw enum output of TestTargetD
        const AZStd::string rawEnum =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<testsuites tests=\"249\" name=\"AllTests\">\n"
            "  <testsuite name=\"TestCase\" tests=\"5\">\n"
            "    <testcase name=\"Test1_WillPass\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"56\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"61\" />\n"
            "    <testcase name=\"Test3_WillPass\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"66\" />\n"
            "    <testcase name=\"Test4_WillPass\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"71\" />\n"
            "    <testcase name=\"Test5_WillPass\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"76\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"TestFixture1\" tests=\"2\">\n"
            "    <testcase name=\"Test1_WillPass\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"81\" />\n"
            "    <testcase name=\"Test2_WillPass\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"86\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"DISABLED_TestFixture2\" tests=\"2\">\n"
            "    <testcase name=\"Test1_WillPass\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"91\" />\n"
            "    <testcase name=\"Test2_WillPass\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"96\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"TestFixtureWithTypes1/0\" tests=\"3\">\n"
            "    <testcase name=\"Test1_WillPass\" type_param=\"int\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"157\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"int\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"162\" />\n"
            "    <testcase name=\"Test3_WillPass\" type_param=\"int\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"167\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"TestFixtureWithTypes1/1\" tests=\"3\">\n"
            "    <testcase name=\"Test1_WillPass\" type_param=\"float\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"157\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"float\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"162\" />\n"
            "    <testcase name=\"Test3_WillPass\" type_param=\"float\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"167\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"TestFixtureWithTypes1/2\" tests=\"3\">\n"
            "    <testcase name=\"Test1_WillPass\" type_param=\"double\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"157\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"double\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"162\" />\n"
            "    <testcase name=\"Test3_WillPass\" type_param=\"double\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"167\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"TestFixtureWithTypes1/3\" tests=\"3\">\n"
            "    <testcase name=\"Test1_WillPass\" type_param=\"char\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"157\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"char\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"162\" />\n"
            "    <testcase name=\"Test3_WillPass\" type_param=\"char\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"167\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"DISABLED_TestFixtureWithTypes2/0\" tests=\"3\">\n"
            "    <testcase name=\"Test1_WillPass\" type_param=\"int\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"172\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"int\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"177\" />\n"
            "    <testcase name=\"Test3_WillPass\" type_param=\"int\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"182\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"DISABLED_TestFixtureWithTypes2/1\" tests=\"3\">\n"
            "    <testcase name=\"Test1_WillPass\" type_param=\"float\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"172\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"float\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"177\" />\n"
            "    <testcase name=\"Test3_WillPass\" type_param=\"float\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"182\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"DISABLED_TestFixtureWithTypes2/2\" tests=\"3\">\n"
            "    <testcase name=\"Test1_WillPass\" type_param=\"double\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"172\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"double\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"177\" />\n"
            "    <testcase name=\"Test3_WillPass\" type_param=\"double\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"182\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"DISABLED_TestFixtureWithTypes2/3\" tests=\"3\">\n"
            "    <testcase name=\"Test1_WillPass\" type_param=\"char\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"172\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass\" type_param=\"char\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"177\" />\n"
            "    <testcase name=\"Test3_WillPass\" type_param=\"char\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"182\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"PermutationA/TestFixtureWithParams1\" tests=\"54\">\n"
            "    <testcase name=\"Test1_WillPass/0\" value_param=\"(1, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/1\" value_param=\"(1, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/2\" value_param=\"(1, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/3\" value_param=\"(1, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/4\" value_param=\"(1, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/5\" value_param=\"(1, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/6\" value_param=\"(1, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/7\" value_param=\"(1, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/8\" value_param=\"(1, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/9\" value_param=\"(2, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/10\" value_param=\"(2, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/11\" value_param=\"(2, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/12\" value_param=\"(2, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/13\" value_param=\"(2, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/14\" value_param=\"(2, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/15\" value_param=\"(2, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/16\" value_param=\"(2, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/17\" value_param=\"(2, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/18\" value_param=\"(4, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/19\" value_param=\"(4, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/20\" value_param=\"(4, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/21\" value_param=\"(4, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/22\" value_param=\"(4, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/23\" value_param=\"(4, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/24\" value_param=\"(4, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/25\" value_param=\"(4, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/26\" value_param=\"(4, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/0\" value_param=\"(1, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/1\" value_param=\"(1, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/2\" value_param=\"(1, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/3\" value_param=\"(1, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/4\" value_param=\"(1, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/5\" value_param=\"(1, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/6\" value_param=\"(1, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/7\" value_param=\"(1, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/8\" value_param=\"(1, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/9\" value_param=\"(2, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/10\" value_param=\"(2, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/11\" value_param=\"(2, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/12\" value_param=\"(2, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/13\" value_param=\"(2, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/14\" value_param=\"(2, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/15\" value_param=\"(2, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/16\" value_param=\"(2, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/17\" value_param=\"(2, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/18\" value_param=\"(4, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/19\" value_param=\"(4, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/20\" value_param=\"(4, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/21\" value_param=\"(4, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/22\" value_param=\"(4, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/23\" value_param=\"(4, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/24\" value_param=\"(4, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/25\" value_param=\"(4, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/26\" value_param=\"(4, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"TestFixtureWithParams1\" tests=\"54\">\n"
            "    <testcase name=\"Test1_WillPass/0\" value_param=\"(8, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/1\" value_param=\"(8, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/2\" value_param=\"(8, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/3\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/4\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/5\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/6\" value_param=\"(8, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/7\" value_param=\"(8, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/8\" value_param=\"(8, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/9\" value_param=\"(16, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/10\" value_param=\"(16, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/11\" value_param=\"(16, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/12\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/13\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/14\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/15\" value_param=\"(16, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/16\" value_param=\"(16, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/17\" value_param=\"(16, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/18\" value_param=\"(32, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/19\" value_param=\"(32, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/20\" value_param=\"(32, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/21\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/22\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/23\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/24\" value_param=\"(32, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/25\" value_param=\"(32, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"Test1_WillPass/26\" value_param=\"(32, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/0\" value_param=\"(8, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/1\" value_param=\"(8, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/2\" value_param=\"(8, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/3\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/4\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/5\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/6\" value_param=\"(8, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/7\" value_param=\"(8, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/8\" value_param=\"(8, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/9\" value_param=\"(16, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/10\" value_param=\"(16, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/11\" value_param=\"(16, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/12\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/13\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/14\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/15\" value_param=\"(16, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/16\" value_param=\"(16, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/17\" value_param=\"(16, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/18\" value_param=\"(32, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/19\" value_param=\"(32, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/20\" value_param=\"(32, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/21\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/22\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/23\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/24\" value_param=\"(32, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/25\" value_param=\"(32, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/26\" value_param=\"(32, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"101\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"PermutationA/DISABLED_TestFixtureWithParams2\" tests=\"54\">\n"
            "    <testcase name=\"Test1_WillPass/0\" value_param=\"(1, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/1\" value_param=\"(1, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/2\" value_param=\"(1, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/3\" value_param=\"(1, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/4\" value_param=\"(1, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/5\" value_param=\"(1, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/6\" value_param=\"(1, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/7\" value_param=\"(1, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/8\" value_param=\"(1, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/9\" value_param=\"(2, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/10\" value_param=\"(2, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/11\" value_param=\"(2, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/12\" value_param=\"(2, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/13\" value_param=\"(2, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/14\" value_param=\"(2, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/15\" value_param=\"(2, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/16\" value_param=\"(2, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/17\" value_param=\"(2, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/18\" value_param=\"(4, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/19\" value_param=\"(4, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/20\" value_param=\"(4, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/21\" value_param=\"(4, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/22\" value_param=\"(4, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/23\" value_param=\"(4, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/24\" value_param=\"(4, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/25\" value_param=\"(4, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/26\" value_param=\"(4, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/0\" value_param=\"(1, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/1\" value_param=\"(1, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/2\" value_param=\"(1, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/3\" value_param=\"(1, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/4\" value_param=\"(1, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/5\" value_param=\"(1, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/6\" value_param=\"(1, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/7\" value_param=\"(1, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/8\" value_param=\"(1, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/9\" value_param=\"(2, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/10\" value_param=\"(2, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/11\" value_param=\"(2, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/12\" value_param=\"(2, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/13\" value_param=\"(2, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/14\" value_param=\"(2, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/15\" value_param=\"(2, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/16\" value_param=\"(2, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/17\" value_param=\"(2, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/18\" value_param=\"(4, &apos;\\x3&apos; (3), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/19\" value_param=\"(4, &apos;\\x3&apos; (3), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/20\" value_param=\"(4, &apos;\\x3&apos; (3), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/21\" value_param=\"(4, &apos;\\x5&apos; (5), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/22\" value_param=\"(4, &apos;\\x5&apos; (5), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/23\" value_param=\"(4, &apos;\\x5&apos; (5), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/24\" value_param=\"(4, &apos;\\a&apos; (7), -0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/25\" value_param=\"(4, &apos;\\a&apos; (7), 0)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/26\" value_param=\"(4, &apos;\\a&apos; (7), 1)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "  </testsuite>\n"
            "  <testsuite name=\"DISABLED_TestFixtureWithParams2\" tests=\"54\">\n"
            "    <testcase name=\"Test1_WillPass/0\" value_param=\"(8, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/1\" value_param=\"(8, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/2\" value_param=\"(8, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/3\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/4\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/5\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/6\" value_param=\"(8, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/7\" value_param=\"(8, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/8\" value_param=\"(8, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/9\" value_param=\"(16, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/10\" value_param=\"(16, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/11\" value_param=\"(16, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/12\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/13\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/14\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/15\" value_param=\"(16, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/16\" value_param=\"(16, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/17\" value_param=\"(16, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/18\" value_param=\"(32, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/19\" value_param=\"(32, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/20\" value_param=\"(32, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/21\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/22\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/23\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/24\" value_param=\"(32, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/25\" value_param=\"(32, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"Test1_WillPass/26\" value_param=\"(32, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/0\" value_param=\"(8, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/1\" value_param=\"(8, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/2\" value_param=\"(8, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/3\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/4\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/5\" value_param=\"(8, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/6\" value_param=\"(8, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/7\" value_param=\"(8, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/8\" value_param=\"(8, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/9\" value_param=\"(16, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/10\" value_param=\"(16, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/11\" value_param=\"(16, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/12\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/13\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/14\" value_param=\"(16, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/15\" value_param=\"(16, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/16\" value_param=\"(16, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/17\" value_param=\"(16, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/18\" value_param=\"(32, &apos;\\t&apos; (9), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/19\" value_param=\"(32, &apos;\\t&apos; (9), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/20\" value_param=\"(32, &apos;\\t&apos; (9), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/21\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/22\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/23\" value_param=\"(32, &apos;\\r&apos; (13, 0xD), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/24\" value_param=\"(32, &apos;\\x11&apos; (17), -10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/25\" value_param=\"(32, &apos;\\x11&apos; (17), 0.05)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "    <testcase name=\"DISABLED_Test2_WillPass/26\" value_param=\"(32, &apos;\\x11&apos; (17), 10)\" file=\"C:\\Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp\" line=\"111\" />\n"
            "  </testsuite>\n"
            "</testsuites>\n"
            "\n";

        // When the raw enumeration text is parsed
        const AZStd::vector<TestImpact::TestEnumerationSuite> suites = TestImpact::GTest::TestEnumerationSuitesFactory(rawEnum);

        // Expect the generated suite data to match that of the raw enum text
        EXPECT_TRUE(suites == expectedSuites);
    }
} // namespace UnitTest
