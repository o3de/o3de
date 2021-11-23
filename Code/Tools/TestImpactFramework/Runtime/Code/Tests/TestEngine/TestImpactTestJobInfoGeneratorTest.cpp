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
//#include <Target/TestImpactTestTarget.h>
//#include <TestEngine/JobRunner/TestImpactTestJobInfoGenerator.h>
//
//#include <AzCore/UnitTest/TestTypes.h>
//#include <AzTest/AzTest.h>
//
//namespace UnitTest
//{
//    class TestJobInfoGeneratorFixture
//        : public AllocatorsTestFixture
//    {
//    public:
//        TestJobInfoGeneratorFixture()
//            : m_testJobInfoGenerator(
//                "C:\\o3de_TIF_Feature",
//                "C:\\o3de_TIF_Feature\\windows_vs2019\\bin\\debug",
//                "C:\\o3de_TIF_Feature\\windows_vs2019\\bin\\TestImpactFramework",
//                "C:\\o3de_TIF_Feature\\windows_vs2019\\bin\\TestImpactFramework\\Temp",
//                LY_TEST_IMPACT_AZ_TESTRUNNER_BIN,
//                LY_TEST_IMPACT_INSTRUMENTATION_BIN)
//        {
//        }
//
//    protected:
//        TestImpact::TestJobInfoGenerator m_testJobInfoGenerator;
//    };
//
//    TEST_F(TestJobInfoGeneratorFixture, FOO)
//    {
//        TestImpact::NativeTestTarget testTarget(TestImpact::TestTargetDescriptor(
//            {
//                TestImpact::BuildMetaData
//                {
//                    "WhiteBox.Editor.Tests", "WhiteBox.Editor.Tests", ""
//                },
//                {}
//            },
//            TestImpact::TestTargetMeta
//            {
//                "main", "--args", AZStd::chrono::milliseconds{10}, TestImpact::LaunchMethod::TestRunner
//            }));
//
//        const auto enumJobInfo = m_testJobInfoGenerator.GenerateTestEnumerationJobInfo(&testTarget, { 1 }, TestImpact::TestEnumerator::JobData::CachePolicy::Read);
//        const auto regularRunJobInfo = m_testJobInfoGenerator.GenerateRegularTestRunJobInfo(&testTarget, { 2 });
//        const auto lineCoverageJobInfo = m_testJobInfoGenerator.GenerateInstrumentedTestRunJobInfo(&testTarget, { 3 }, TestImpact::CoverageLevel::Line);
//        const auto sourceCoverageJobInfo = m_testJobInfoGenerator.GenerateInstrumentedTestRunJobInfo(&testTarget, { 4 }, TestImpact::CoverageLevel::Source);
//
//        std::cout << enumJobInfo.GetCommand().m_args.c_str() << "\n\n";
//        std::cout << regularRunJobInfo.GetCommand().m_args.c_str() << "\n\n";
//        std::cout << lineCoverageJobInfo.GetCommand().m_args.c_str() << "\n\n";
//        std::cout << sourceCoverageJobInfo.GetCommand().m_args.c_str() << "\n\n";
//    }
//} // namespace TestImpact
