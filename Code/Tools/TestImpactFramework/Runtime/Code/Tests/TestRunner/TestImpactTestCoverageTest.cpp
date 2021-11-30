///*
// * Copyright (c) Contributors to the Open 3D Engine Project.
// * For complete copyright and license terms please see the LICENSE at the root of this distribution.
// *
// * SPDX-License-Identifier: Apache-2.0 OR MIT
// *
// */
//
//#include <TestRunner/Run/TestImpactTestCoverage.h>
//
//#include <AzCore/UnitTest/TestTypes.h>
//#include <AzTest/AzTest.h>
//
//namespace UnitTest
//{
//    namespace
//    {
//        TestImpact::RepoPath GenerateSourcePath(AZ::u32 index)
//        {
//            return AZStd::string::format("SourceFile%u", index);
//        }
//
//        TestImpact::RepoPath GenerateModulePath(AZ::u32 index)
//        {
//            return AZStd::string::format("Module%u", index);
//        }
//
//        AZStd::vector<TestImpact::LineCoverage> GenerateLineCoverages(AZ::u32 numLines)
//        {
//            AZStd::vector<TestImpact::LineCoverage> lineCoverages;
//            for (size_t i = 0; i < numLines; i++)
//            {
//                // Fudge some superficially different but trivially checkable line coverage data
//                lineCoverages.emplace_back(TestImpact::LineCoverage{i, i * 2});
//            }
//
//            return lineCoverages;
//        }
//
//        TestImpact::SourceCoverage GenerateSourceCoverage(AZ::u32 index, TestImpact::CoverageLevel coverageLevel)
//        {
//            TestImpact::SourceCoverage sourceCoverage;
//            sourceCoverage.m_path = GenerateSourcePath(index);
//            if (coverageLevel == TestImpact::CoverageLevel::Line)
//            {
//                sourceCoverage.m_coverage = GenerateLineCoverages(index + 1);
//            }
//
//            return sourceCoverage;
//        }
//
//        AZStd::vector<TestImpact::SourceCoverage> GenerateSourceCoverages(AZ::u32 numSources, TestImpact::CoverageLevel coverageLevel)
//        {
//            AZStd::vector<TestImpact::SourceCoverage> sourceCoverages;
//            for (AZ::u32 i = 0; i < numSources; i++)
//            {
//                sourceCoverages.emplace_back(GenerateSourceCoverage(i, coverageLevel));
//            }
//
//            return sourceCoverages;
//        }
//
//        TestImpact::ModuleCoverage GenerateModuleCoverage(AZ::u32 index, AZ::u32 numSources, TestImpact::CoverageLevel coverageLevel)
//        {
//            TestImpact::ModuleCoverage moduleCoverage;
//            moduleCoverage.m_path = GenerateModulePath(index);
//            moduleCoverage.m_sources = GenerateSourceCoverages(numSources, coverageLevel);
//            return moduleCoverage;
//        }
//
//        AZStd::vector<TestImpact::ModuleCoverage> GenerateModuleCoverages(AZ::u32 numModules, TestImpact::CoverageLevel coverageLevel)
//        {
//            AZStd::vector<TestImpact::ModuleCoverage> moduleCoverages;
//            for (AZ::u32 i = 0; i < numModules; i++)
//            {
//                // Fudge some superficially different but trivially deducible module coverage data
//                moduleCoverages.emplace_back(GenerateModuleCoverage(i, i + 1, coverageLevel));
//            }
//
//            return moduleCoverages;
//        }
//    } // namespace
//
//    using CoveragePermutation = AZStd::tuple
//        <
//            unsigned,                   // Number of modules covered
//            TestImpact::CoverageLevel   // Test coverage level
//        >;
//
//    // Fixture parameterized for different max number of concurrent jobs
//    class TestCoverageFixtureWithCoverageParams
//        : public AllocatorsTestFixture
//        , public ::testing::WithParamInterface<CoveragePermutation>
//    {
//    public:
//        void SetUp() override;
//
//    protected:
//        void ValidateTestCoverage(const TestImpact::TestCoverage& testCoverage);
//
//        size_t m_numModulesCovered;
//        TestImpact::CoverageLevel m_coverageLevel;
//    };
//
//    void TestCoverageFixtureWithCoverageParams::SetUp()
//    {
//        AllocatorsTestFixture::SetUp();
//        const auto& [numModulesCovered, coverageLevel] = GetParam();
//        m_numModulesCovered = numModulesCovered;
//        m_coverageLevel = coverageLevel;
//    }
//
//    void TestCoverageFixtureWithCoverageParams::ValidateTestCoverage(const TestImpact::TestCoverage& testCoverage)
//    {
//        // Expect the coverage level to match that which was used to generate the module coverages generated
//        EXPECT_EQ(testCoverage.GetCoverageLevel(), m_coverageLevel);
//
//        // Expect the number of modules covered to match the number of module coverages generated
//        EXPECT_EQ(testCoverage.GetNumModulesCovered(), m_numModulesCovered);
//
//        // Expect the number of unique sources covered to match the number of modules coverages generated
//        EXPECT_EQ(testCoverage.GetNumSourcesCovered(), m_numModulesCovered);
//
//        // Expect the unique sources covered to match the procedurally generated source paths
//        for (size_t sourceIndex = 0; sourceIndex < testCoverage.GetNumSourcesCovered(); sourceIndex++)
//        {
//            EXPECT_EQ(testCoverage.GetSourcesCovered()[sourceIndex], GenerateSourcePath(sourceIndex));
//        }
//
//        // Expect each module covered to match that of the corresponding procedurally generated modules
//        for (size_t moduleIndex = 0; moduleIndex < testCoverage.GetNumModulesCovered(); moduleIndex++)
//        {
//            const TestImpact::ModuleCoverage& moduleCoverage = testCoverage.GetModuleCoverages()[moduleIndex];
//
//            // Expect the module path to match that of the corresponding procedurally generated module
//            EXPECT_EQ(moduleCoverage.m_path, GenerateModulePath(moduleIndex));
//
//            // Expect the module's number of sources to match that of the corresponding procedurally generated module
//            EXPECT_EQ(moduleCoverage.m_sources.size(), moduleIndex + 1);
//
//            for (size_t sourceIndex = 0; sourceIndex < moduleCoverage.m_sources.size(); sourceIndex++)
//            {
//                const TestImpact::SourceCoverage& sourceCoverage = moduleCoverage.m_sources[sourceIndex];
//
//                // Expect the source path to match the procedurally generated source path
//                EXPECT_EQ(sourceCoverage.m_path, GenerateSourcePath(sourceIndex));
//
//                if (m_coverageLevel == TestImpact::CoverageLevel::Line)
//                {
//                    // Expect there to actually be line coverage data if this coverage was procedurally generated with line data
//                    EXPECT_FALSE(sourceCoverage.m_coverage.empty());
//
//                    const AZStd::vector<TestImpact::LineCoverage>& lineCoverages = sourceCoverage.m_coverage;
//
//                    // Expect the source's number of lines to match that of the corresponding procedurally generated source
//                    EXPECT_EQ(lineCoverages.size(), sourceIndex + 1);
//
//                    for (size_t lineIndex = 0; lineIndex < lineCoverages.size(); lineIndex++)
//                    {
//                        // The expected line number and hit count are deduced as follows:
//                        // Line number: line index
//                        // Hit count: 2x line index
//                        EXPECT_EQ(lineCoverages[lineIndex].m_lineNumber, lineIndex);
//                        EXPECT_EQ(lineCoverages[lineIndex].m_hitCount, lineIndex * 2);
//                    }
//                }
//                else
//                {
//                    // Do not expect there to actually be line coverage data if this coverage was not procedurally generated with line data
//                    EXPECT_TRUE(sourceCoverage.m_coverage.empty());
//                }
//            }
//        }
//    }
//
//    TEST(TestCoverage, EmptyCoverage_ExpectTestRunException)
//    {
//        // When constructing a test coverage from the empty module coverages
//        TestImpact::TestCoverage testCoverage(AZStd::vector<TestImpact::ModuleCoverage>{});
//
//        // Expect the test coverage fields to be empty
//        EXPECT_EQ(testCoverage.GetNumModulesCovered(), 0);
//        EXPECT_EQ(testCoverage.GetNumSourcesCovered(), 0);
//        EXPECT_TRUE(testCoverage.GetModuleCoverages().empty());
//        EXPECT_TRUE(testCoverage.GetSourcesCovered().empty());
//    }
//
//    TEST_P(TestCoverageFixtureWithCoverageParams, AllCoveragePermutations_ExpectTestCoverageMetaDatasToMatchPermutations)
//    {
//        // Given a procedurally generated test coverage
//        const TestImpact::TestCoverage testCoverage(GenerateModuleCoverages(m_numModulesCovered, m_coverageLevel));
//
//        // Expect the test coverage data and meta-data to match that of the rules used to procedurally generate the coverage data
//        ValidateTestCoverage(testCoverage);
//    }
//
//    INSTANTIATE_TEST_CASE_P(
//        ,
//        TestCoverageFixtureWithCoverageParams,
//        ::testing::Combine(
//            ::testing::Range(1u, 11u),
//            ::testing::Values(TestImpact::CoverageLevel::Line, TestImpact::CoverageLevel::Source))
//    );
//} // namespace UnitTest
