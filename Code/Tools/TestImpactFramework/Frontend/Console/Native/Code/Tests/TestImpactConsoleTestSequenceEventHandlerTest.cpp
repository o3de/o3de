///*
// * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
// * its licensors.
// *
// * For complete copyright and license terms please see the LICENSE at the root of this
// * distribution (the "License"). All use of this software is governed by the License,
// * or, if provided, by the license below or the license accompanying this file. Do not
// * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
// * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// *
// */
//
//#include <TestImpactFramework/TestImpactClientTestSelection.h>
//#include <TestImpactFramework/TestImpactClientTestRun.h>
//
//#include <TestImpactConsoleTestSequenceEventHandler.h>
//
//#include <AzCore/std/containers/vector.h>
//#include <AzCore/std/smart_ptr/unique_ptr.h>
//#include <AzCore/UnitTest/TestTypes.h>
//#include <AzTest/AzTest.h>
//
//namespace UnitTest
//{
//    class ConsoleTestSequenceTestFixture
//        : public AllocatorsTestFixture
//    {
//    };
//
//    TEST_F(ConsoleTestSequenceTestFixture, CheckEmptyArgs_ExpectDefaultValues)
//    {
//        AZStd::unordered_set<AZStd::string> suites = { "PERIODIC","MAIN" };
//        TestImpact::Console::TestSequenceEventHandler seq(&suites);
//        AZStd::vector<AZStd::string> selectedTests = { "Test1", "Test2", "Test3", "Test4", "Test5" };
//        seq.operator()(TestImpact::Client::TestRunSelection(selectedTests, {"Test6"}));
//
//        for (auto i = 0; i < selectedTests.size(); i++)
//        {
//            seq.operator()(TestImpact::Client::TestRun(selectedTests[i], (TestImpact::Client::TestRunResult)i, AZStd::chrono::milliseconds(i)));
//        }
//    }
//}
