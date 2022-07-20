/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactMicroRepo.h>
#include <TestImpactTestUtils.h>

#include <Artifact/Factory/TestImpactNativeTargetDescriptorFactory.h>
#include <Artifact/Factory/TestImpactNativeTestTargetMetaMapFactory.h>
#include <Artifact/Static/TestImpactNativeTargetDescriptorCompiler.h>
#include <Artifact/TestImpactArtifactException.h>
#include <BuildTarget/Common/TestImpactBuildTargetList.h>
#include <Dependency/TestImpactDependencyException.h>
#include <Dependency/TestImpactDynamicDependencyMap.h>
#include <Target/Native/TestImpactNativeTestTarget.h>
#include <Target/Native/TestImpactNativeProductionTarget.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    namespace
    {
        using NativeBuildTarget = TestImpact::BuildTarget<TestImpact::NativeTestTarget, TestImpact::NativeProductionTarget>;
        using NativeTestTargetList = TestImpact::TargetList<TestImpact::NativeTestTarget>;
        using NativeProductionTargetList = TestImpact::TargetList<TestImpact::NativeProductionTarget>;
        using NativeBuildTargetList = TestImpact::BuildTargetList<TestImpact::NativeTestTarget, TestImpact::NativeProductionTarget>;
        using NativeSourceDependency = TestImpact::SourceDependency<TestImpact::NativeTestTarget, TestImpact::NativeProductionTarget>;
        using NativeDynamicDependencyMap = TestImpact::DynamicDependencyMap<TestImpact::NativeTestTarget, TestImpact::NativeProductionTarget>;

        void ValidateTarget(const TestImpact::NativeTarget& target, const TestImpact::NativeTarget& expectedTarget)
        {
            EXPECT_EQ(target.GetName(), expectedTarget.GetName());
            EXPECT_EQ(target.GetOutputName(), expectedTarget.GetOutputName());
            EXPECT_EQ(target.GetPath(), expectedTarget.GetPath());
            EXPECT_TRUE(target.GetSources() == expectedTarget.GetSources());
        }

        void ValidateBuildTarget(const NativeBuildTarget& buildTarget, const TestImpact::NativeTestTarget& expectedTestTarget)
        {
            EXPECT_EQ(buildTarget.GetTargetType(), TestImpact::BuildTargetType::TestTarget);

            buildTarget.Visit(
            [&expectedTestTarget](auto&& target)
            {
                ValidateTarget(*target, expectedTestTarget);
            });
        }

        void ValidateBuildTarget(
            const NativeBuildTarget& buildTarget, const TestImpact::NativeProductionTarget& expectedProductionTarget)
        {
            EXPECT_EQ(buildTarget.GetTargetType(), TestImpact::BuildTargetType::ProductionTarget);

            buildTarget.Visit(
            [&expectedProductionTarget](auto&& target)
            {                
                ValidateTarget(*target, expectedProductionTarget);
            });
        }

        void ValidateProductionTarget(
            const TestImpact::NativeProductionTarget& productionTarget, const TestImpact::NativeProductionTarget& expectedTarget)
        {
            ValidateTarget(productionTarget, expectedTarget);
        }

        void ValidateTestTarget(const TestImpact::NativeTestTarget& testTarget, const TestImpact::NativeTestTarget& expectedTarget)
        {
            ValidateTarget(testTarget, expectedTarget);
            EXPECT_EQ(testTarget.GetSuite(), expectedTarget.GetSuite());
            EXPECT_EQ(testTarget.GetLaunchMethod(), expectedTarget.GetLaunchMethod());
        }

        void ValidateSourceDependency(
            const NativeSourceDependency& sourceDependency,
            const AZStd::vector<TestImpact::SourceCoveringTests>& sourceCoveringTestsList)
        {
            const auto sourceCoveringTests = AZStd::find_if(
                sourceCoveringTestsList.begin(), sourceCoveringTestsList.end(),
                [&sourceDependency](const TestImpact::SourceCoveringTests& coverage)
                {
                    return coverage.GetPath() == sourceDependency.GetPath();
                });

            // Expect the source file to exist in the source covering test list
            EXPECT_TRUE(sourceCoveringTests != sourceCoveringTestsList.end());

            // Expect the number of covering tests in the dependency and source's covering tests to match
            EXPECT_EQ(sourceDependency.GetCoveringTestTargets().size(), sourceCoveringTests->GetNumCoveringTestTargets());

            for (const auto* coveringTestTarget : sourceDependency.GetCoveringTestTargets())
            {
                // Expect the covering test in the dependency to exist in the source's covering tests
                const auto& coveringTestTargets = sourceCoveringTests->GetCoveringTestTargets();
                EXPECT_TRUE(
                    AZStd::find(coveringTestTargets.begin(), coveringTestTargets.end(), coveringTestTarget->GetName()) !=
                    coveringTestTargets.end());
            }
        }

        void ValidateSourceCoverage(const TestImpact::SourceCoveringTestsList& lhs, const TestImpact::SourceCoveringTestsList& rhs)
        {
            EXPECT_EQ(lhs.GetNumSources(), rhs.GetNumSources());
            EXPECT_TRUE(lhs.GetCoverage() == rhs.GetCoverage());
        }

        template<typename TargetList>
        size_t CountSources(const TargetList& targetList)
        {
            size_t numSources = 0;
            for (const auto& target : targetList.GetTargets())
            {
                numSources += target.GetSources().m_staticSources.size();
            }

            return numSources;
        }
    } // namespace

    class DynamicDependencyMapFixture : public AllocatorsTestFixture
    {
    protected:
        AZStd::unique_ptr<NativeBuildTargetList> m_buildTargets;
        AZStd::unique_ptr<NativeDynamicDependencyMap> m_dynamicDependencyMap;
        AZStd::unique_ptr<NativeProductionTargetList> m_productionTargets;
        AZStd::unique_ptr<NativeTestTargetList> m_testTargets;
    };

    TEST_F(DynamicDependencyMapFixture, NoProductionTargetDescriptors_ExpectTargetException)
    {
        try
        {
            // When constructing a dynamic dependency map with no production targets
            m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
                MicroRepo::CreateTestTargetDescriptors(), AZStd::vector<AZStd::unique_ptr<TestImpact::NativeProductionTargetDescriptor>>{});
            m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

            // Do not expect this statement to be reachable
            FAIL();
        } catch ([[maybe_unused]] const TestImpact::TargetException& e)
        {
            // Expect a target exception
            SUCCEED();
        } catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(DynamicDependencyMapFixture, NoTestTargetDescriptors_ExpectTargetException)
    {
        try
        {
            // When constructing a dynamic dependency map with no test targets
            m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
                AZStd::vector<AZStd::unique_ptr<TestImpact::NativeTestTargetDescriptor>>{}, MicroRepo::CreateProductionTargetDescriptors());
            m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

            // Do not expect this statement to be reachable
            FAIL();
        } catch ([[maybe_unused]] const TestImpact::TargetException& e)
        {
            // Expect a target exception
            SUCCEED();
        } catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(DynamicDependencyMapFixture, ProductionTargetDescriptorsAndTestTargetDescriptors_ExpectValidTargets)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // When constructing a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        // Expect the number of production targets in the dynamic dependency map to match that of those constructed from the descriptors
        EXPECT_EQ(
            m_dynamicDependencyMap->GetBuildTargets()->GetProductionTargetList().GetNumTargets(), m_productionTargets->GetNumTargets());

        // Expect the number of test targets in the dynamic dependency map to match that of those constructed from the descriptors
        EXPECT_EQ(m_dynamicDependencyMap->GetBuildTargets()->GetTestTargetList().GetNumTargets(), m_testTargets->GetNumTargets());

        // Expect the total number of build targets in the repository to match the total number of descriptors used to construct those
        // targets
        EXPECT_EQ(
            m_dynamicDependencyMap->GetBuildTargets()->GetNumTargets(),
            m_productionTargets->GetNumTargets() + m_testTargets->GetNumTargets());

        // Expect no orphaned source files as each file belongs to at least one parent build target
        const auto orphans = m_dynamicDependencyMap->GetOrphanSourceFiles();
        EXPECT_TRUE(orphans.empty());

        // Expect each production target in the dynamic dependency map to match that of the descriptors used to construct those targets
        for (const auto& productionTarget : m_dynamicDependencyMap->GetBuildTargets()->GetProductionTargetList().GetTargets())
        {
            const auto* expectedProductionTarget = m_productionTargets->GetTargetOrThrow(productionTarget.GetName());
            ValidateProductionTarget(productionTarget, *expectedProductionTarget);
        }

        // Expect each test target in the dynamic dependency map to match that of the descriptors used to construct those targets
        for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargets()->GetTestTargetList().GetTargets())
        {
            const auto* expectedTestTarget = m_testTargets->GetTargetOrThrow(testTarget.GetName());
            ValidateTestTarget(testTarget, *expectedTestTarget);
        }
    }

    TEST_F(DynamicDependencyMapFixture, GetSourceDependency_ValidPath_ExpectValidSources)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // When constructing a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        // Expect the number of sources in the dynamic dependency map to match the total number of sources in the descriptors
        EXPECT_EQ(
            m_dynamicDependencyMap->GetNumSources(),
            CountSources(*m_productionTargets) + CountSources(*m_testTargets) - 1 /*THAT SHARED ONE I GUESS!? INVESTIGATE*/);

        // Expect each production source's parent to match that of the corresponding source dependency
        for (const auto& productionTarget : m_dynamicDependencyMap->GetBuildTargets()->GetProductionTargetList().GetTargets())
        {
            for (const auto& staticSource : productionTarget.GetSources().m_staticSources)
            {
                if (staticSource == "ProdAndTest.cpp")
                    continue;
                const auto sourceDependency = m_dynamicDependencyMap->GetSourceDependency(staticSource);
                EXPECT_TRUE(sourceDependency.has_value());
                EXPECT_TRUE(sourceDependency->GetCoveringTestTargets().empty());
                EXPECT_EQ(sourceDependency->GetNumParentTargets(), 1);
                ValidateBuildTarget(*sourceDependency->GetParentTargets().begin(), productionTarget);
            }
        }

        // Expect each test source's parent to match that of the corresponding source dependency
        for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargets()->GetTestTargetList().GetTargets())
        {
            for (const auto& staticSource : testTarget.GetSources().m_staticSources)
            {
                if (staticSource == "ProdAndTest.cpp")
                    continue;
                const auto sourceDependency = m_dynamicDependencyMap->GetSourceDependency(staticSource);
                EXPECT_TRUE(sourceDependency.has_value());
                EXPECT_TRUE(sourceDependency->GetCoveringTestTargets().empty());
                EXPECT_EQ(sourceDependency->GetNumParentTargets(), 1);
                ValidateBuildTarget(*sourceDependency->GetParentTargets().begin(), testTarget);
            }
        }
    }

    TEST_F(DynamicDependencyMapFixture, GetSourceDependencyOrThrow_ValidPath_ExpectValidSources)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // When constructing a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        // Expect the number of sources in the dynamic dependency map to match the total number of sources in the descriptors
        EXPECT_EQ(m_dynamicDependencyMap->GetNumSources(), CountSources(*m_productionTargets) + CountSources(*m_testTargets) - 1);

        // Expect each production source's parent to match that of the corresponding source dependency
        for (const auto& productionTarget : m_dynamicDependencyMap->GetBuildTargets()->GetProductionTargetList().GetTargets())
        {
            for (const auto& staticSource : productionTarget.GetSources().m_staticSources)
            {
                if (staticSource == "ProdAndTest.cpp")
                    continue;
                const auto sourceDependency = m_dynamicDependencyMap->GetSourceDependencyOrThrow(staticSource);
                EXPECT_TRUE(sourceDependency.GetCoveringTestTargets().empty());
                EXPECT_EQ(sourceDependency.GetNumParentTargets(), 1);
                ValidateBuildTarget(*sourceDependency.GetParentTargets().begin(), productionTarget);
            }
        }

        // Expect each test source's parent to match that of the corresponding source dependency
        for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargets()->GetTestTargetList().GetTargets())
        {
            for (const auto& staticSource : testTarget.GetSources().m_staticSources)
            {
                if (staticSource == "ProdAndTest.cpp")
                    continue;
                const auto sourceDependency = m_dynamicDependencyMap->GetSourceDependencyOrThrow(staticSource);
                EXPECT_TRUE(sourceDependency.GetCoveringTestTargets().empty());
                EXPECT_EQ(sourceDependency.GetNumParentTargets(), 1);
                ValidateBuildTarget(*sourceDependency.GetParentTargets().begin(), testTarget);
            }
        }
    }

    TEST_F(DynamicDependencyMapFixture, GetSourceDependencyAutogen_ExpectValidSources)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // Given a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        const auto validateAutogenSource = [this](const AZStd::string& path)
        {
            // When retrieving the source dependencies for the specified autogen source
            const auto sourceDependency = m_dynamicDependencyMap->GetSourceDependencyOrThrow(path);

            // Expect the parent build data to match the expected parent Lib B production target
            EXPECT_EQ(sourceDependency.GetNumParentTargets(), 1);

            m_dynamicDependencyMap->GetBuildTargets()->GetBuildTargetOrThrow("Lib B").Visit(
            [&sourceDependency](auto&& autogenTarget)
            {
                for (const auto& parentTarget : sourceDependency.GetParentTargets())
                {
                    ValidateBuildTarget(parentTarget, *autogenTarget);
                }
            });
        };

        // Expect the input source and two output sources for this autogen coupling to refer to the same build data
        validateAutogenSource("LibB_AutogenInput.xml");
        validateAutogenSource("LibB_2.cpp");
        validateAutogenSource("LibB_3.cpp");
    }

    TEST_F(DynamicDependencyMapFixture, ReplaceSourceCoverage_ExpectValidCoverage)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // Given a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        // Given a list of source covering test targets representing the test coverage of the repository
        const auto sourceCoveringTests = MicroRepo::CreateSourceCoveringTestList();

        // When applying the source coverage test list to the dynamic dependency map
        m_dynamicDependencyMap->ReplaceSourceCoverage(
            TestImpact::SourceCoveringTestsList(AZStd::move(MicroRepo::CreateSourceCoveringTestList())));

        // Expect the input source and two output sources for this autogen coupling to refer to the same build data
        const auto autogenInputDependency = m_dynamicDependencyMap->GetSourceDependencyOrThrow("LibB_AutogenInput.xml");
        const auto autogenOutput2Dependency = m_dynamicDependencyMap->GetSourceDependencyOrThrow("LibB_2.cpp");
        const auto autogenOutput3Dependency = m_dynamicDependencyMap->GetSourceDependencyOrThrow("LibB_3.cpp");
        for (const auto& parentTarget : autogenInputDependency.GetParentTargets())
        {
            if (AZStd::find_if(
                    autogenOutput2Dependency.GetParentTargets().begin(), autogenOutput2Dependency.GetParentTargets().end(),
                    [&parentTarget](const NativeBuildTarget& p)
                    {
                        return parentTarget.GetTarget() == p.GetTarget();
                    }) == autogenOutput2Dependency.GetParentTargets().end() &&
                AZStd::find_if(
                    autogenOutput3Dependency.GetParentTargets().begin(), autogenOutput3Dependency.GetParentTargets().end(),
                    [&parentTarget](const NativeBuildTarget& p)
                    {
                        return parentTarget.GetTarget() == p.GetTarget();
                    }) == autogenOutput3Dependency.GetParentTargets().end())
            {
                FAIL();
            }
        }

        for (const auto* coveringTestTarget : autogenInputDependency.GetCoveringTestTargets())
        {
            if (AZStd::find(
                    autogenOutput2Dependency.GetCoveringTestTargets().begin(), autogenOutput2Dependency.GetCoveringTestTargets().end(),
                    coveringTestTarget) == autogenOutput2Dependency.GetCoveringTestTargets().end() &&
                AZStd::find(
                    autogenOutput3Dependency.GetCoveringTestTargets().begin(), autogenOutput3Dependency.GetCoveringTestTargets().end(),
                    coveringTestTarget) == autogenOutput3Dependency.GetCoveringTestTargets().end())
            {
                FAIL();
            }
        }

        // Expect each production source's parent and covering tests to match that of the corresponding source dependency
        for (const auto& productionTarget : m_dynamicDependencyMap->GetBuildTargets()->GetProductionTargetList().GetTargets())
        {
            for (const auto& staticSource : productionTarget.GetSources().m_staticSources)
            {
                if (staticSource == "ProdAndTest.cpp")
                    continue;
                const auto sourceDependency = m_dynamicDependencyMap->GetSourceDependencyOrThrow(staticSource);
                EXPECT_FALSE(sourceDependency.GetCoveringTestTargets().empty());
                EXPECT_EQ(sourceDependency.GetNumParentTargets(), 1);
                ValidateBuildTarget(*sourceDependency.GetParentTargets().begin(), productionTarget);
                ValidateSourceDependency(sourceDependency, sourceCoveringTests);
            }
        }

        // Expect each test source's parent and covering tests to match that of the corresponding source dependency
        for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargets()->GetTestTargetList().GetTargets())
        {
            for (const auto& staticSource : testTarget.GetSources().m_staticSources)
            {
                if (staticSource == "ProdAndTest.cpp")
                    continue;
                const auto sourceDependency = m_dynamicDependencyMap->GetSourceDependencyOrThrow(staticSource);
                EXPECT_FALSE(sourceDependency.GetCoveringTestTargets().empty());
                EXPECT_EQ(sourceDependency.GetNumParentTargets(), 1);
                ValidateBuildTarget(*sourceDependency.GetParentTargets().begin(), testTarget);
                ValidateSourceDependency(sourceDependency, sourceCoveringTests);
            }
        }
    }

    TEST_F(DynamicDependencyMapFixture, ReplaceSourceCoverageWithOrphans_ExpectValidCoverageAndOrphanedFiles)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // Given a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        // Given a list of source covering test targets with two covered sources that will have no parents in the dependency map
        auto sourceCoveringTestList = MicroRepo::CreateSourceCoveringTestList();
        sourceCoveringTestList.push_back(
            TestImpact::SourceCoveringTests(TestImpact::RepoPath("Orphan.cpp"), AZStd::vector<AZStd::string>{ "Test A", "Test B" }));
        sourceCoveringTestList.push_back(
            TestImpact::SourceCoveringTests(TestImpact::RepoPath("Orphan.h"), AZStd::vector<AZStd::string>{ "Test Aux", "Test Core" }));

        // When applying the source coverage test list to the dynamic dependency map
        m_dynamicDependencyMap->ReplaceSourceCoverage(TestImpact::SourceCoveringTestsList(AZStd::move(sourceCoveringTestList)));

        // Expect two orphaned files to be in the source dependency map
        const auto orphans = m_dynamicDependencyMap->GetOrphanSourceFiles();
        EXPECT_EQ(orphans.size(), 2);

        const auto validateOrphan = [&orphans, this](const AZStd::string& orphan, const AZStd::vector<AZStd::string>& coveringTests)
        {
            // Expect the specified orphaned file to exist in the orphaned file list
            EXPECT_TRUE(AZStd::find(orphans.begin(), orphans.end(), orphan) != orphans.end());

            // Expect the specified orphaned file to exist in the dynamic dependency map
            const auto sourceDependency = m_dynamicDependencyMap->GetSourceDependencyOrThrow(orphan);

            // Expect no parent build targets as this is an orphaned file
            EXPECT_EQ(sourceDependency.GetNumParentTargets(), 0);

            // Expect the number of covering test targets to match that of the orphaned file
            EXPECT_EQ(sourceDependency.GetCoveringTestTargets().size(), coveringTests.size());

            // Expect each covering test target to exist in the dependency's covering targets list
            for (const auto& testTarget : sourceDependency.GetCoveringTestTargets())
            {
                EXPECT_TRUE(AZStd::find(coveringTests.begin(), coveringTests.end(), testTarget->GetName()) != coveringTests.end());
            }
        };

        validateOrphan("Orphan.cpp", { "Test A", "Test B" });
        validateOrphan("Orphan.h", { "Test Aux", "Test Core" });

        sourceCoveringTestList = MicroRepo::CreateSourceCoveringTestList();
        sourceCoveringTestList.push_back(
            TestImpact::SourceCoveringTests(TestImpact::RepoPath("Orphan.cpp"), AZStd::vector<AZStd::string>{ "Test A", "Test B" }));
        sourceCoveringTestList.push_back(
            TestImpact::SourceCoveringTests(TestImpact::RepoPath("Orphan.h"), AZStd::vector<AZStd::string>{ "Test Aux", "Test Core" }));

        // Expect each production source's parent and covering tests to match that of the corresponding source dependency
        for (const auto& productionTarget : m_dynamicDependencyMap->GetBuildTargets()->GetProductionTargetList().GetTargets())
        {
            for (const auto& staticSource : productionTarget.GetSources().m_staticSources)
            {
                if (staticSource == "ProdAndTest.cpp")
                    continue;
                const auto sourceDependency = m_dynamicDependencyMap->GetSourceDependencyOrThrow(staticSource);
                EXPECT_FALSE(sourceDependency.GetCoveringTestTargets().empty());
                EXPECT_EQ(sourceDependency.GetNumParentTargets(), 1);
                ValidateBuildTarget(*sourceDependency.GetParentTargets().begin(), productionTarget);
                ValidateSourceDependency(sourceDependency, sourceCoveringTestList);
            }
        }

        // Expect each test source's parent and covering tests to match that of the corresponding source dependency
        for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargets()->GetTestTargetList().GetTargets())
        {
            for (const auto& staticSource : testTarget.GetSources().m_staticSources)
            {
                if (staticSource == "ProdAndTest.cpp")
                    continue;
                const auto sourceDependency = m_dynamicDependencyMap->GetSourceDependencyOrThrow(staticSource);
                EXPECT_FALSE(sourceDependency.GetCoveringTestTargets().empty());
                EXPECT_EQ(sourceDependency.GetNumParentTargets(), 1);
                ValidateBuildTarget(*sourceDependency.GetParentTargets().begin(), testTarget);
                ValidateSourceDependency(sourceDependency, sourceCoveringTestList);
            }
        }
    }

    TEST_F(DynamicDependencyMapFixture, ExportSourceCoverage_ExpecExportedSourceCoverageToMatchReference)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // Given a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        // Given a list of source covering test targets representing the test coverage of the repository
        const auto sourceCoveringTestList = MicroRepo::CreateSourceCoveringTestList();

        // When applying the source coverage test list to the dynamic dependency map
        m_dynamicDependencyMap->ReplaceSourceCoverage(
            TestImpact::SourceCoveringTestsList(AZStd::move(MicroRepo::CreateSourceCoveringTestList())));

        // Expect the retrieved coverage to match the applied coverage
        ValidateSourceCoverage(
            m_dynamicDependencyMap->ExportSourceCoverage(), TestImpact::SourceCoveringTestsList(MicroRepo::CreateSourceCoveringTestList()));
    }

    TEST_F(DynamicDependencyMapFixture, GetSourceDependency_InvalidPath_ExpectEmpty)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // When constructing a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        // When retrieving a source not in the dynamic dependency map
        auto invalidSourceDependency = m_dynamicDependencyMap->GetSourceDependency("invalid");

        // Expect the retrieved source dependencies to be empty
        EXPECT_FALSE(invalidSourceDependency.has_value());
    }

    TEST_F(DynamicDependencyMapFixture, GetSourceDependencyOrThrow_InvalidPath_ExpectDependencyException)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // When constructing a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        try
        {
            // When retrieving a source not in the dynamic dependency map
            m_dynamicDependencyMap->GetSourceDependencyOrThrow("invalid");

            // Do not expect this statement to be reachable
            FAIL();
        } catch ([[maybe_unused]] const TestImpact::DependencyException& e)
        {
            // Expect a target exception
            SUCCEED();
        } catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(DynamicDependencyMapFixture, GetBuildTarget_ValidBuildTargets_ExpectValidBuildTarget)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // Given a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        for (const auto& expectedProductionTarget : m_productionTargets->GetTargets())
        {
            m_dynamicDependencyMap->GetBuildTargets()->GetBuildTargetOrThrow(expectedProductionTarget.GetName()).Visit(
                [&expectedProductionTarget](auto&& buildTarget)
                {
                    // Expect the build target to be valid
                    EXPECT_TRUE(buildTarget);


                    // Expect the retrieved build target to match the production target we queried
                    ValidateBuildTarget(buildTarget, expectedProductionTarget);
                });
        }

        for (const auto& expectedTestTarget : m_testTargets->GetTargets())
        {
            m_dynamicDependencyMap->GetBuildTargets()->GetBuildTargetOrThrow(expectedTestTarget.GetName()).Visit(
                [&expectedTestTarget](auto&& buildTarget)
                {
                    // Expect the build target to be valid
                    EXPECT_TRUE(buildTarget);

                    // Expect the retrieved build target to match the test target we queried
                    ValidateBuildTarget(buildTarget, expectedTestTarget);
                });
        }
    }

    TEST_F(DynamicDependencyMapFixture, GetBuildTarget_InvalidBuildTargets_ExpectEmpty)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // Given a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        const auto buildTarget = m_dynamicDependencyMap->GetBuildTargets()->GetBuildTarget("invalid");
        EXPECT_FALSE(buildTarget.has_value());
    }

    TEST_F(DynamicDependencyMapFixture, GetBuildTargetOrThrow_ValidBuildTargets_ExpectValidBuildTarget)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // Given a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        for (const auto& expectedProductionTarget : m_productionTargets->GetTargets())
        {
            m_dynamicDependencyMap->GetBuildTargets()->GetBuildTargetOrThrow(expectedProductionTarget.GetName()).Visit(
                [&expectedProductionTarget](auto&& buildTarget)
                {
                    // Expect the retrieved build target to match the production target we queried
                    ValidateBuildTarget(buildTarget, expectedProductionTarget);
                });
        }

        for (const auto& expectedTestTarget : m_testTargets->GetTargets())
        {
            m_dynamicDependencyMap->GetBuildTargets()->GetBuildTargetOrThrow(expectedTestTarget.GetName()).Visit(
                [&expectedTestTarget](auto&& buildTarget)
                {
                    // Expect the retrieved build target to match the test target we queried
                    ValidateBuildTarget(buildTarget, expectedTestTarget);
                });
        }
    }

    TEST_F(DynamicDependencyMapFixture, GetBuildTargetOrThrow_InvalidTargets_ExpectTargetException)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // Given a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        try
        {
            // When retrieving a target not in the dynamic dependency map
            m_dynamicDependencyMap->GetBuildTargets()->GetBuildTargetOrThrow("invalid");

            // Do not expect this statement to be reachable
            FAIL();
        } catch ([[maybe_unused]] const TestImpact::TargetException& e)
        {
            // Expect a target exception
            SUCCEED();
        } catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(DynamicDependencyMapFixture, GetTarget_ValidTargets_ExpectValidTargets)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // Given a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        for (const auto& expectedProductionTarget : m_productionTargets->GetTargets())
        {
            // When retrieving the production target in the dynamic dependency map
            auto buildTarget = m_dynamicDependencyMap->GetBuildTargets()->GetBuildTargetOrThrow(expectedProductionTarget.GetName());

            // Expect the retrieved production target to match the production target we queried
            ValidateBuildTarget(*buildTarget.GetProductionTarget(), expectedProductionTarget);
        }

        for (const auto& expectedTestTarget : m_testTargets->GetTargets())
        {
            // When retrieving the test target in the dynamic dependency map
            auto buildTarget = m_dynamicDependencyMap->GetBuildTargets()->GetBuildTargetOrThrow(expectedTestTarget.GetName());

            // Expect the retrieved production target to match the production target we queried
            ValidateBuildTarget(*buildTarget.GetTestTarget(), expectedTestTarget);
        }
    }

    TEST_F(DynamicDependencyMapFixture, GetTarget_InvalidBuildTargets_ExpectEmpty)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // Given a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        // When retrieving a target not in the dynamic dependency map
        auto invalidTarget = m_dynamicDependencyMap->GetBuildTargets()->GetBuildTarget("invalid");

        // Expect the retrieved target to be empty
        EXPECT_FALSE(invalidTarget.has_value());
    }

    TEST_F(DynamicDependencyMapFixture, GetTargetOrThrow_InvalidTargets_ExpectTargetException)
    {
        // Given a list of production and test targets representing the build system of a repository
        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(MicroRepo::CreateProductionTargetDescriptors());
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(MicroRepo::CreateTestTargetDescriptors());

        // Given a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptors(), MicroRepo::CreateProductionTargetDescriptors());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        try
        {
            // When retrieving a target not in the dynamic dependency map
            m_dynamicDependencyMap->GetBuildTargets()->GetBuildTargetOrThrow("invalid");

            // Do not expect this statement to be reachable
            FAIL();
        } catch ([[maybe_unused]] const TestImpact::TargetException& e)
        {
            // Expect a target exception
            SUCCEED();
        } catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(DynamicDependencyMapFixture, AddCommonSource_ExpectSourceHasTwoParents)
    {
        // Given a list of production and test targets with common sources between targets
        auto productionTargetDescriptors = MicroRepo::CreateProductionTargetDescriptorsWithSharedSources();
        auto testTargetDescriptors = MicroRepo::CreateTestTargetDescriptorsWithSharedSources();

        m_productionTargets = AZStd::make_unique<NativeProductionTargetList>(AZStd::move(productionTargetDescriptors));
        m_testTargets = AZStd::make_unique<NativeTestTargetList>(AZStd::move(testTargetDescriptors));

        // When constructing a dynamic dependency map with valid production and test targets
        m_buildTargets = AZStd::make_unique<NativeBuildTargetList>(
            MicroRepo::CreateTestTargetDescriptorsWithSharedSources(), MicroRepo::CreateProductionTargetDescriptorsWithSharedSources());
        m_dynamicDependencyMap = AZStd::make_unique<NativeDynamicDependencyMap>(m_buildTargets.get());

        // Expect the number of sources in the dynamic dependency map to match the total number of sources in the descriptors
        EXPECT_EQ(m_dynamicDependencyMap->GetNumSources(), CountSources(*m_productionTargets) + CountSources(*m_testTargets) - 3);

        // Expect no orphaned source files as each file belongs to at least one parent build target
        const auto orphans = m_dynamicDependencyMap->GetOrphanSourceFiles();
        EXPECT_TRUE(orphans.empty());

        // Expect each production source's parent to match that of the corresponding source dependency
        for (const auto& productionTarget : m_dynamicDependencyMap->GetBuildTargets()->GetProductionTargetList().GetTargets())
        {
            for (const auto& staticSource : productionTarget.GetSources().m_staticSources)
            {
                const auto sourceDependency = m_dynamicDependencyMap->GetSourceDependency(staticSource);
                EXPECT_TRUE(sourceDependency.has_value());
                EXPECT_TRUE(sourceDependency.value().GetCoveringTestTargets().empty());

                if (staticSource == "LibAux_2.cpp" || staticSource == "LibB_2.cpp")
                {
                    EXPECT_EQ(sourceDependency.value().GetNumParentTargets(), 2);
                    const auto& parentTargets = sourceDependency.value().GetParentTargets();
                    for (const auto parentTarget : parentTargets)
                    {
                        const auto& parentStaticSources = parentTarget.GetTarget()->GetSources().m_staticSources;
                        EXPECT_TRUE(
                            AZStd::find(parentStaticSources.begin(), parentStaticSources.end(), staticSource) != parentStaticSources.end());
                    }
                }
                else
                {
                    if (staticSource == "ProdAndTest.cpp")
                        continue;
                    EXPECT_EQ(sourceDependency.value().GetNumParentTargets(), 1);
                    ValidateBuildTarget(*sourceDependency.value().GetParentTargets().begin(), productionTarget);
                }
            }
        }

        // Expect each test source's parent to match that of the corresponding source dependency
        for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargets()->GetTestTargetList().GetTargets())
        {
            for (const auto& staticSource : testTarget.GetSources().m_staticSources)
            {
                const auto sourceDependency = m_dynamicDependencyMap->GetSourceDependency(staticSource);
                EXPECT_TRUE(sourceDependency.has_value());
                EXPECT_TRUE(sourceDependency.value().GetCoveringTestTargets().empty());

                if (staticSource == "LibAux_2.cpp" || staticSource == "LibB_2.cpp")
                {
                    EXPECT_EQ(sourceDependency.value().GetNumParentTargets(), 2);
                    const auto& parentTargets = sourceDependency.value().GetParentTargets();
                    for (const auto parentTarget : parentTargets)
                    {
                        const auto& parentStaticSources = parentTarget.GetTarget()->GetSources().m_staticSources;
                        EXPECT_TRUE(
                            AZStd::find(parentStaticSources.begin(), parentStaticSources.end(), staticSource) != parentStaticSources.end());
                    }
                }
                else
                {
                    if (staticSource == "ProdAndTest.cpp")
                        continue;
                    EXPECT_EQ(sourceDependency.value().GetNumParentTargets(), 1);
                    ValidateBuildTarget(*sourceDependency.value().GetParentTargets().begin(), testTarget);
                }
            }
        }
    }
} // namespace UnitTest
