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

#include <Artifact/Static/TestImpactTargetDescriptorCompiler.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class TargetDescriptorCompilerTestFixture
        : public AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            m_buildTargetDescriptors.emplace_back(TestImpact::BuildTargetDescriptor{{"TestTargetA", "", ""}, {}});
            m_buildTargetDescriptors.emplace_back(TestImpact::BuildTargetDescriptor{{"TestTargetB", "", ""}, {}});
            m_buildTargetDescriptors.emplace_back(TestImpact::BuildTargetDescriptor{{"ProductionTargetA", "", ""}, {}});
            m_buildTargetDescriptors.emplace_back(TestImpact::BuildTargetDescriptor{{"ProductionTargetB", "", ""}, {}});
            m_buildTargetDescriptors.emplace_back(TestImpact::BuildTargetDescriptor{{"ProductionTargetC", "", ""}, {}});

            m_testTargetMetaMap.emplace("TestTargetA", TestImpact::TestTargetMeta{"", TestImpact::LaunchMethod::TestRunner});
            m_testTargetMetaMap.emplace("TestTargetB", TestImpact::TestTargetMeta{"", TestImpact::LaunchMethod::StandAlone});
        }

    protected:
        AZStd::vector<TestImpact::BuildTargetDescriptor> m_buildTargetDescriptors;
        TestImpact::TestTargetMetaMap m_testTargetMetaMap;
    };

    TestImpact::ProductionTargetDescriptor ConstructProductionTargetDescriptor(const AZStd::string& name)
    {
        return TestImpact::ProductionTargetDescriptor{TestImpact::BuildTargetDescriptor{{name, "", ""}, {{}, {}}}};
    }

    TestImpact::TestTargetDescriptor ConstructTestTargetDescriptor(const AZStd::string& name, TestImpact::LaunchMethod launchMethod)
    {
        return TestImpact::TestTargetDescriptor{
            TestImpact::BuildTargetDescriptor{{name, "", ""}, {{}, {}}}, TestImpact::TestTargetMeta{"", launchMethod}};
    }

    TEST_F(TargetDescriptorCompilerTestFixture, EmptyBuildTargetDescriptorList_ExpectArtifactException)
    {
        try
        {
            // Given an empty build target descriptor list but valid test target meta map
            // When attempting to construct the test target
            const auto& [productionTargetDescriptors, testTargetDescriptors] =
                TestImpact::CompileTargetDescriptors({}, AZStd::move(m_testTargetMetaMap));

            // Do not expect this statement to be reachable
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

    TEST_F(TargetDescriptorCompilerTestFixture, EmptyTestTargetMetaMap_ExpectArtifactException)
    {
        try
        {
            // Given a valid build target descriptor list but empty test target meta map
            // When attempting to construct the test target
            const auto& [productionTargetDescriptors, testTargetDescriptors] =
                TestImpact::CompileTargetDescriptors(AZStd::move(m_buildTargetDescriptors), {});

            // Do not expect this statement to be reachable
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

    TEST_F(TargetDescriptorCompilerTestFixture, TestTargetWithNoMatchingMeta_ExpectArtifactException)
    {
        try
        {
            // Given a valid build target descriptor list but a test target meta map with an orphan entry
            m_testTargetMetaMap.emplace("Orphan", TestImpact::TestTargetMeta{"", TestImpact::LaunchMethod::TestRunner});

            // When attempting to construct the test target
            const auto& [productionTargetDescriptors, testTargetDescriptors] =
                TestImpact::CompileTargetDescriptors(AZStd::move(m_buildTargetDescriptors), {});

            // Do not expect this statement to be reachable
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

    TEST_F(TargetDescriptorCompilerTestFixture, ValidProductionTargetsAndTestTargetMetas_ExpectValidProductionAndTestTargets)
    {
        // Given a valid build target descriptor list and a valid test target meta map
        // When attempting to construct the test target
        const auto& [productionTargetDescriptors, testTargetDescriptors] =
            TestImpact::CompileTargetDescriptors(AZStd::move(m_buildTargetDescriptors), AZStd::move(m_testTargetMetaMap));

        // Expect the production targets to match the expected targets
        EXPECT_TRUE(productionTargetDescriptors.size() == 3);
        EXPECT_TRUE(productionTargetDescriptors[0] == ConstructProductionTargetDescriptor("ProductionTargetA"));
        EXPECT_TRUE(productionTargetDescriptors[1] == ConstructProductionTargetDescriptor("ProductionTargetB"));
        EXPECT_TRUE(productionTargetDescriptors[2] == ConstructProductionTargetDescriptor("ProductionTargetC"));

        // Expect the test targets to match the expected targets
        EXPECT_TRUE(testTargetDescriptors.size() == 2);
        EXPECT_TRUE(testTargetDescriptors[0] == ConstructTestTargetDescriptor("TestTargetA", TestImpact::LaunchMethod::TestRunner));
        EXPECT_TRUE(testTargetDescriptors[1] == ConstructTestTargetDescriptor("TestTargetB", TestImpact::LaunchMethod::StandAlone));
    }

} // namespace UnitTest
