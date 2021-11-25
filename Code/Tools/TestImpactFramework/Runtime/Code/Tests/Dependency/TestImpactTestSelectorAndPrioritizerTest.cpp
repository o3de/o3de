///*
// * Copyright (c) Contributors to the Open 3D Engine Project.
// * For complete copyright and license terms please see the LICENSE at the root of this distribution.
// *
// * SPDX-License-Identifier: Apache-2.0 OR MIT
// *
// */
//
//#include <TestImpactTestUtils.h>
//#include <TestImpactMicroRepo.h>
//
//#include <Dependency/TestImpactDynamicDependencyMap.h>
//#include <Dependency/TestImpactDependencyException.h>
//#include <Dependency/TestImpactTestSelectorAndPrioritizer.h>
//#include <BuildSystem/Native/TestImpactNativeBuildTargetTraitsTraits.h>
//
//#include <AzCore/std/algorithm.h>
//#include <AzCore/std/smart_ptr/unique_ptr.h>
//#include <AzCore/UnitTest/TestTypes.h>
//#include <AzTest/AzTest.h>
//
//namespace UnitTest
//{
//    class TestSelectorAndPrioritizerFixture
//        : public AllocatorsTestFixture
//    {
//    public:
//    protected:
//        using NativeBuildTargetList = TestImpact::BuildTargetList<TestImpact::NativeBuildTargetTraits>; 
//        using NativeDynamicDependencyMap = TestImpact::DynamicDependencyMap<TestImpact::NativeBuildTargetTraits>;
//        using NativeTestSelectorAndPrioritizer = TestImpact::TestSelectorAndPrioritizer<TestImpact::NativeBuildTargetTraits>;
//
//        AZStd::unique_ptr<NativeBuildTargetList> m_buildTargets;
//        AZStd::unique_ptr<NativeDynamicDependencyMap> m_dynamicDependencyMap;
//        AZStd::unique_ptr<NativeTestSelectorAndPrioritizer> m_testSelectorAndPrioritizer;
//    };
//
//    class TestSelectorAndPrioritizerFixtureWithParams
//        : public TestSelectorAndPrioritizerFixture
//        , public ::testing::WithParamInterface<AZStd::tuple<MicroRepo::SourceMap::value_type, TestImpact::Policy::TestPrioritization>>
//    {
//    public:
//        TestSelectorAndPrioritizerFixtureWithParams()
//        {
//            auto [source, selectionStrategy] = GetParam();
//            m_source.first = source.first;
//            m_source.second = source.second;
//            m_testSelectionStrategy = selectionStrategy;
//        }
//
//    protected:
//        MicroRepo::SourceMapEntry m_source;
//        TestImpact::Policy::TestPrioritization m_testSelectionStrategy;
//    };
//
//    class TestSelectorAndPrioritizerFixtureWithAllSources
//        : public TestSelectorAndPrioritizerFixtureWithParams
//    {};
//
//    class TestSelectorAndPrioritizerFixtureWithAllSourcesExceptAutogenSources
//        : public TestSelectorAndPrioritizerFixtureWithParams
//    {};
//
//    // UPDATE THE EXPECTED!!!!!!!!!!!!!!!! THESE HAVE CHANGED
//
//    // Action  : Create
//    // Parent  : Yes
//    // Coverage: No
//    TEST_P(TestSelectorAndPrioritizerFixtureWithAllSources, CreateProductionFile_ParentYesCoverageNo_ExpectSelectAllTestTargetsCoveringParentTargets)
//    {
//        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
//            MicroRepo::CreateTestTargetDescriptorsWithSharedSources(), MicroRepo::CreateProductionTargetDescriptorsWithSharedSources());
//        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());
//    
//        m_testSelectorAndPrioritizer = AZStd::make_unique< NativeTestSelectorAndPrioritizer>(
//            m_dynamicDependencyMap.get(), TestImpact::DependencyGraphDataMap{});
//    
//        m_dynamicDependencyMap->ReplaceSourceCoverage(
//            TestImpact::SourceCoveringTestsList(MicroRepo::CreateSourceCoverageTestsWithoutSpecifiedSource(MicroRepo::CreateSourceCoveringTestListWithSharedSources(), m_source.first.c_str())));
//    
//        TestImpact::ChangeList changeList;
//        changeList.m_createdFiles.push_back(m_source.first.c_str());
//
//        const auto changeDependecyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList, TestImpact::Policy::IntegrityFailure::Continue);
//
//        const auto selectedTestTargets = m_testSelectorAndPrioritizer->SelectTestTargets(changeDependecyList, m_testSelectionStrategy);
//    
//        if (m_testSelectionStrategy == TestImpact::Policy::TestPrioritization::None)
//        {
//            EXPECT_EQ(selectedTestTargets.size(), m_source.second.m_createParentYesCoverageNo.size());
//            for (const auto testTarget : selectedTestTargets)
//            {
//                const auto& expectedSelectedTargets = m_source.second.m_createParentYesCoverageNo;
//                EXPECT_TRUE(AZStd::find(expectedSelectedTargets.begin(), expectedSelectedTargets.end(), testTarget->GetName().c_str()) != expectedSelectedTargets.end());
//            }
//        }
//    }
//
//    // Action  : Create
//    // Parent  : No
//    // Coverage: Yes
//    // Source  : Indeterminate
//    TEST_P(TestSelectorAndPrioritizerFixtureWithAllSourcesExceptAutogenSources, CreateFile_ParentNoCoverageYes_ExpectDependencyException)
//    {
//        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
//            MicroRepo::CreateTargetDescriptorWithoutSpecifiedSource(MicroRepo::CreateTestTargetDescriptorsWithSharedSources(), m_source.first.c_str()),
//            MicroRepo::CreateTargetDescriptorWithoutSpecifiedSource(MicroRepo::CreateProductionTargetDescriptorsWithSharedSources(), m_source.first.c_str()));
//        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());
//
//        m_testSelectorAndPrioritizer = AZStd::make_unique< NativeTestSelectorAndPrioritizer>(
//            m_dynamicDependencyMap.get(), TestImpact::DependencyGraphDataMap{});
//
//        m_dynamicDependencyMap->ReplaceSourceCoverage(TestImpact::SourceCoveringTestsList(MicroRepo::CreateSourceCoveringTestListWithSharedSources()));
//
//        TestImpact::ChangeList changeList;
//        changeList.m_createdFiles.push_back(m_source.first.c_str());
//
//        try
//        {
//            const auto changeDependecyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList, TestImpact::Policy::IntegrityFailure::Continue);
//
//            // Do not expect this statement to be reachable
//            FAIL();
//        }
//        catch ([[maybe_unused]] const TestImpact::DependencyException& e)
//        {
//            // Expect an dependency exception
//            SUCCEED();
//        }
//        catch (...)
//        {
//            // Do not expect any other exceptions
//            FAIL();
//        }
//    }
//
//    // Action  : Create
//    // Parent  : No
//    // Coverage: No
//    TEST_P(TestSelectorAndPrioritizerFixtureWithAllSources, CreateFile_ParentNoCoverageNo_ExpectFileSkipped)
//    {
//        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
//            MicroRepo::CreateTargetDescriptorWithoutSpecifiedSource(MicroRepo::CreateTestTargetDescriptorsWithSharedSources(), m_source.first.c_str()),
//            MicroRepo::CreateTargetDescriptorWithoutSpecifiedSource(MicroRepo::CreateProductionTargetDescriptorsWithSharedSources(), m_source.first.c_str()));
//        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());
//    
//        m_testSelectorAndPrioritizer = AZStd::make_unique< NativeTestSelectorAndPrioritizer>(
//            m_dynamicDependencyMap.get(), TestImpact::DependencyGraphDataMap{});
//    
//        m_dynamicDependencyMap->ReplaceSourceCoverage(
//            TestImpact::SourceCoveringTestsList(MicroRepo::CreateSourceCoverageTestsWithoutSpecifiedSource(MicroRepo::CreateSourceCoveringTestListWithSharedSources(), m_source.first.c_str())));
//    
//        TestImpact::ChangeList changeList;
//        changeList.m_createdFiles.push_back(m_source.first.c_str());
//        const auto changeDependecyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList, TestImpact::Policy::IntegrityFailure::Continue);
//
//        const auto selectedTestTargets = m_testSelectorAndPrioritizer->SelectTestTargets(changeDependecyList, m_testSelectionStrategy);
//    
//        EXPECT_TRUE(selectedTestTargets.empty());
//    }
//
//    // Action  : Create
//    // Parent  : Yes
//    // Coverage: Yes
//    TEST_P(TestSelectorAndPrioritizerFixtureWithAllSources, CreateFile_ParentYesCoverageYes_ExpectDependencyException)
//    {
//        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
//            MicroRepo::CreateTestTargetDescriptorsWithSharedSources(), MicroRepo::CreateProductionTargetDescriptorsWithSharedSources());
//        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());
//
//        m_testSelectorAndPrioritizer = AZStd::make_unique< NativeTestSelectorAndPrioritizer>(
//            m_dynamicDependencyMap.get(), TestImpact::DependencyGraphDataMap{});
//
//        m_dynamicDependencyMap->ReplaceSourceCoverage(TestImpact::SourceCoveringTestsList(MicroRepo::CreateSourceCoveringTestListWithSharedSources()));
//
//        TestImpact::ChangeList changeList;
//        changeList.m_createdFiles.push_back(m_source.first.c_str());
//
//        try
//        {
//            const auto changeDependecyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList, TestImpact::Policy::IntegrityFailure::Continue);
//
//            // Do not expect this statement to be reachable
//            FAIL();
//        }
//        catch ([[maybe_unused]] const TestImpact::DependencyException& e)
//        {
//            // Expect an dependency exception
//            SUCCEED();
//        }
//        catch (...)
//        {
//            // Do not expect any other exceptions
//            FAIL();
//        }
//    }
//
//    // Action  : Update
//    // Parent  : Yes
//    // Coverage: No
//    TEST_P(TestSelectorAndPrioritizerFixtureWithAllSources, UpdateTestFile_ParentYesCoverageNo_ExpectSelectAllParentTestTargets)
//    {
//        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
//            MicroRepo::CreateTestTargetDescriptorsWithSharedSources(), MicroRepo::CreateProductionTargetDescriptorsWithSharedSources());
//        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());
//
//        m_testSelectorAndPrioritizer = AZStd::make_unique< NativeTestSelectorAndPrioritizer>(
//            m_dynamicDependencyMap.get(), TestImpact::DependencyGraphDataMap{});
//
//        m_dynamicDependencyMap->ReplaceSourceCoverage(
//            TestImpact::SourceCoveringTestsList(MicroRepo::CreateSourceCoverageTestsWithoutSpecifiedSource(MicroRepo::CreateSourceCoveringTestListWithSharedSources(), m_source.first.c_str())));
//
//        TestImpact::ChangeList changeList;
//        changeList.m_updatedFiles.push_back(m_source.first.c_str());
//        const auto changeDependecyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList, TestImpact::Policy::IntegrityFailure::Continue);
//
//        const auto selectedTestTargets = m_testSelectorAndPrioritizer->SelectTestTargets(changeDependecyList, m_testSelectionStrategy);
//
//        if (m_testSelectionStrategy == TestImpact::Policy::TestPrioritization::None)
//        {
//            EXPECT_EQ(selectedTestTargets.size(), m_source.second.m_updateParentYesCoverageNo.size());
//            for (const auto testTarget : selectedTestTargets)
//            {
//                const auto& expectedSelectedTargets = m_source.second.m_updateParentYesCoverageNo;
//                EXPECT_TRUE(AZStd::find(expectedSelectedTargets.begin(), expectedSelectedTargets.end(), testTarget->GetName().c_str()) != expectedSelectedTargets.end());
//            }
//        }
//    }
//
//    // Action  : Update
//    // Parent  : No
//    // Coverage: Yes
//    TEST_P(TestSelectorAndPrioritizerFixtureWithAllSourcesExceptAutogenSources, UpdateFile_ParentNoCoverageYes_ExpectSelectAllTestsCoveringThisFileAndDeleteExistingCoverage)
//    {
//        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
//            MicroRepo::CreateTargetDescriptorWithoutSpecifiedSource(MicroRepo::CreateTestTargetDescriptorsWithSharedSources(), m_source.first.c_str()),
//            MicroRepo::CreateTargetDescriptorWithoutSpecifiedSource(MicroRepo::CreateProductionTargetDescriptorsWithSharedSources(), m_source.first.c_str()));
//        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());
//
//        m_testSelectorAndPrioritizer = AZStd::make_unique< NativeTestSelectorAndPrioritizer>(
//            m_dynamicDependencyMap.get(), TestImpact::DependencyGraphDataMap{});
//
//        m_dynamicDependencyMap->ReplaceSourceCoverage(TestImpact::SourceCoveringTestsList(MicroRepo::CreateSourceCoveringTestListWithSharedSources()));
//
//        TestImpact::ChangeList changeList;
//        changeList.m_updatedFiles.push_back(m_source.first.c_str());
//
//        const auto changeDependecyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList, TestImpact::Policy::IntegrityFailure::Continue);
//        const auto sourceDependency = m_dynamicDependencyMap->GetSourceDependency(m_source.first.c_str());
//        EXPECT_FALSE(sourceDependency.has_value());
//
//        const auto selectedTestTargets = m_testSelectorAndPrioritizer->SelectTestTargets(changeDependecyList, m_testSelectionStrategy);
//
//        if (m_testSelectionStrategy == TestImpact::Policy::TestPrioritization::None)
//        {
//            const auto& expectedSelectedTargets = m_source.second.m_updateParentNoCoverageYes;
//            EXPECT_EQ(selectedTestTargets.size(), expectedSelectedTargets.size());
//            for (const auto testTarget : selectedTestTargets)
//            {
//                EXPECT_TRUE(AZStd::find(expectedSelectedTargets.begin(), expectedSelectedTargets.end(), testTarget->GetName().c_str()) != expectedSelectedTargets.end());
//            }
//        }
//    }
//
//    // Action  : Update
//    // Parent  : No
//    // Coverage: No
//    TEST_P(TestSelectorAndPrioritizerFixtureWithAllSources, UpdateFile_ParentNoCoverageNo_ExpectFileSkipped)
//    {
//        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
//            MicroRepo::CreateTargetDescriptorWithoutSpecifiedSource(MicroRepo::CreateTestTargetDescriptorsWithSharedSources(), m_source.first.c_str()),
//            MicroRepo::CreateTargetDescriptorWithoutSpecifiedSource(MicroRepo::CreateProductionTargetDescriptorsWithSharedSources(), m_source.first.c_str()));
//        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());
//
//        m_testSelectorAndPrioritizer = AZStd::make_unique< NativeTestSelectorAndPrioritizer>(
//            m_dynamicDependencyMap.get(), TestImpact::DependencyGraphDataMap{});
//
//        m_dynamicDependencyMap->ReplaceSourceCoverage(
//            TestImpact::SourceCoveringTestsList(MicroRepo::CreateSourceCoverageTestsWithoutSpecifiedSource(MicroRepo::CreateSourceCoveringTestListWithSharedSources(), m_source.first.c_str())));
//
//        TestImpact::ChangeList changeList;
//        changeList.m_updatedFiles.push_back(m_source.first.c_str());
//        const auto changeDependecyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList, TestImpact::Policy::IntegrityFailure::Continue);
//
//        const auto selectedTestTargets = m_testSelectorAndPrioritizer->SelectTestTargets(changeDependecyList, m_testSelectionStrategy);
//
//        EXPECT_TRUE(selectedTestTargets.empty());
//    }
//
//    // Action  : Update
//    // Parent  : Yes
//    // Coverage: Yes
//    TEST_P(TestSelectorAndPrioritizerFixtureWithAllSources, UpdateProductionFile_ParentYesCoverageYes_ExpectSelectAllTestsCoveringThisFile)
//    {
//        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
//            MicroRepo::CreateTestTargetDescriptorsWithSharedSources(), MicroRepo::CreateProductionTargetDescriptorsWithSharedSources());
//        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());
//    
//        m_testSelectorAndPrioritizer = AZStd::make_unique< NativeTestSelectorAndPrioritizer>(
//            m_dynamicDependencyMap.get(), TestImpact::DependencyGraphDataMap{});
//    
//        m_dynamicDependencyMap->ReplaceSourceCoverage(TestImpact::SourceCoveringTestsList(MicroRepo::CreateSourceCoveringTestListWithSharedSources()));
//    
//        TestImpact::ChangeList changeList;
//        changeList.m_updatedFiles.push_back(m_source.first.c_str());
//        const auto changeDependecyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList, TestImpact::Policy::IntegrityFailure::Continue);
//    
//        const auto selectedTestTargets = m_testSelectorAndPrioritizer->SelectTestTargets(changeDependecyList, m_testSelectionStrategy);
//    
//        if (m_testSelectionStrategy == TestImpact::Policy::TestPrioritization::None)
//        {
//            const auto& expectedSelectedTargets = m_source.second.m_updateParentYesCoverageYes;
//            EXPECT_EQ(selectedTestTargets.size(), expectedSelectedTargets.size());
//            for (const auto testTarget : selectedTestTargets)
//            {
//                EXPECT_TRUE(AZStd::find(expectedSelectedTargets.begin(), expectedSelectedTargets.end(), testTarget->GetName().c_str()) != expectedSelectedTargets.end());
//            }
//        }
//    }
//
//    // Action  : Delete
//    // Parent  : Yes
//    // Coverage: No
//    TEST_P(TestSelectorAndPrioritizerFixtureWithAllSources, DeleteFile_ParentYesCoverageNo_ExpectDependencyException)
//    {
//        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
//            MicroRepo::CreateTestTargetDescriptorsWithSharedSources(), MicroRepo::CreateProductionTargetDescriptorsWithSharedSources());
//        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());
//
//        m_testSelectorAndPrioritizer = AZStd::make_unique< NativeTestSelectorAndPrioritizer>(
//            m_dynamicDependencyMap.get(), TestImpact::DependencyGraphDataMap{});
//
//        m_dynamicDependencyMap->ReplaceSourceCoverage(
//            TestImpact::SourceCoveringTestsList(MicroRepo::CreateSourceCoverageTestsWithoutSpecifiedSource(MicroRepo::CreateSourceCoveringTestListWithSharedSources(), m_source.first.c_str())));
//
//        TestImpact::ChangeList changeList;
//        changeList.m_deletedFiles.push_back(m_source.first.c_str());
//
//        try
//        {
//            const auto changeDependecyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList, TestImpact::Policy::IntegrityFailure::Continue);
//
//            // Do not expect this statement to be reachable
//            FAIL();
//        }
//        catch ([[maybe_unused]] const TestImpact::DependencyException& e)
//        {
//            // Expect an dependency exception
//            SUCCEED();
//        }
//        catch (...)
//        {
//            // Do not expect any other exceptions
//            FAIL();
//        }
//    }
//
//    // Action  : Delete
//    // Parent  : No
//    // Coverage: Yes
//    TEST_P(TestSelectorAndPrioritizerFixtureWithAllSources, DeleteFile_ParentNoCoverageYes_ExpectSelectAllTestsCoveringThisFileAndDeleteExistingCoverage)
//    {
//        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
//            MicroRepo::CreateTargetDescriptorWithoutSpecifiedSource(MicroRepo::CreateTestTargetDescriptorsWithSharedSources(), m_source.first.c_str()),
//            MicroRepo::CreateTargetDescriptorWithoutSpecifiedSource(MicroRepo::CreateProductionTargetDescriptorsWithSharedSources(), m_source.first.c_str()));
//        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());
//
//        m_testSelectorAndPrioritizer = AZStd::make_unique< NativeTestSelectorAndPrioritizer>(
//            m_dynamicDependencyMap.get(), TestImpact::DependencyGraphDataMap{});
//
//        m_dynamicDependencyMap->ReplaceSourceCoverage(TestImpact::SourceCoveringTestsList(MicroRepo::CreateSourceCoveringTestListWithSharedSources()));
//
//        TestImpact::ChangeList changeList;
//        changeList.m_deletedFiles.push_back(m_source.first.c_str());
//
//        const auto changeDependecyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList, TestImpact::Policy::IntegrityFailure::Continue);
//        const auto sourceDependency = m_dynamicDependencyMap->GetSourceDependency(m_source.first.c_str());
//        EXPECT_FALSE(sourceDependency.has_value());
//
//        const auto selectedTestTargets = m_testSelectorAndPrioritizer->SelectTestTargets(changeDependecyList, m_testSelectionStrategy);
//
//        if (m_testSelectionStrategy == TestImpact::Policy::TestPrioritization::None)
//        {
//            const auto& expectedSelectedTargets = m_source.second.m_deleteParentNoCoverageYes;
//            EXPECT_EQ(selectedTestTargets.size(), expectedSelectedTargets.size());
//            for (const auto testTarget : selectedTestTargets)
//            {
//                EXPECT_TRUE(AZStd::find(expectedSelectedTargets.begin(), expectedSelectedTargets.end(), testTarget->GetName().c_str()) != expectedSelectedTargets.end());
//            }
//        }
//    }
//
//    // Action  : Delete
//    // Parent  : No
//    // Coverage: No
//    TEST_P(TestSelectorAndPrioritizerFixtureWithAllSources, DeleteFile_ParentNoCoverageNo_ExpectFileSkipped)
//    {
//        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
//            MicroRepo::CreateTargetDescriptorWithoutSpecifiedSource(MicroRepo::CreateTestTargetDescriptorsWithSharedSources(), m_source.first.c_str()),
//            MicroRepo::CreateTargetDescriptorWithoutSpecifiedSource(MicroRepo::CreateProductionTargetDescriptorsWithSharedSources(), m_source.first.c_str()));
//        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());
//
//        m_testSelectorAndPrioritizer = AZStd::make_unique< NativeTestSelectorAndPrioritizer>(
//            m_dynamicDependencyMap.get(), TestImpact::DependencyGraphDataMap{});
//
//        m_dynamicDependencyMap->ReplaceSourceCoverage(
//            TestImpact::SourceCoveringTestsList(MicroRepo::CreateSourceCoverageTestsWithoutSpecifiedSource(MicroRepo::CreateSourceCoveringTestListWithSharedSources(), m_source.first.c_str())));
//
//        TestImpact::ChangeList changeList;
//        changeList.m_deletedFiles.push_back(m_source.first.c_str());
//        const auto changeDependecyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList, TestImpact::Policy::IntegrityFailure::Continue);
//
//        const auto selectedTestTargets = m_testSelectorAndPrioritizer->SelectTestTargets(changeDependecyList, m_testSelectionStrategy);
//
//        EXPECT_TRUE(selectedTestTargets.empty());
//    }
//
//    // Action  : Delete
//    // Parent  : Yes
//    // Coverage: Yes
//    TEST_P(TestSelectorAndPrioritizerFixtureWithAllSources, DeleteFile_ParentYesCoverageYes_ExpectDependencyException)
//    {
//        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
//            MicroRepo::CreateTestTargetDescriptorsWithSharedSources(), MicroRepo::CreateProductionTargetDescriptorsWithSharedSources());
//        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());
//
//        m_testSelectorAndPrioritizer = AZStd::make_unique< NativeTestSelectorAndPrioritizer>(
//            m_dynamicDependencyMap.get(), TestImpact::DependencyGraphDataMap{});
//
//        m_dynamicDependencyMap->ReplaceSourceCoverage(TestImpact::SourceCoveringTestsList(MicroRepo::CreateSourceCoveringTestListWithSharedSources()));
//
//        TestImpact::ChangeList changeList;
//        changeList.m_deletedFiles.push_back(m_source.first.c_str());
//
//        try
//        {
//            const auto changeDependecyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList, TestImpact::Policy::IntegrityFailure::Continue);
//
//            // Do not expect this statement to be reachable
//            FAIL();
//        }
//        catch ([[maybe_unused]] const TestImpact::DependencyException& e)
//        {
//            // Expect an dependency exception
//            SUCCEED();
//        }
//        catch (...)
//        {
//            // Do not expect any other exceptions
//            FAIL();
//        }
//    }
//
//    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    //TEST_F(TestSelectorAndPrioritizerFixture, AllValidChangelistOperationPermutations_ExpectValidTestSelections)
//    //{
//    //  // do permutations, 
//    //}
//
//    INSTANTIATE_TEST_CASE_P(
//        ,
//        TestSelectorAndPrioritizerFixtureWithAllSources,
//        ::testing::Combine(
//            ::testing::ValuesIn(MicroRepo::GenerateSourceMap(MicroRepo::Sources::AutogenInput | MicroRepo::Sources::Production | MicroRepo::Sources::Mixed | MicroRepo::Sources::Test)),
//            ::testing::Values(TestImpact::Policy::TestPrioritization::None/*, TestImpact::TestSelection::SelectAndPriotitize*/))
//    );
//
//    INSTANTIATE_TEST_CASE_P(
//        ,
//        TestSelectorAndPrioritizerFixtureWithAllSourcesExceptAutogenSources,
//        ::testing::Combine(
//            ::testing::ValuesIn(MicroRepo::GenerateSourceMap(MicroRepo::Sources::Production | MicroRepo::Sources::Mixed | MicroRepo::Sources::Test)),
//            ::testing::Values(TestImpact::Policy::TestPrioritization::None/*, TestImpact::TestSelection::SelectAndPriotitize*/))
//    );
//} // namespace TestImpact
