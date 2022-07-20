/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactTestUtils.h>

#include <Artifact/Static/TestImpactNativeTargetDescriptorCompiler.h>
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

            m_NativeTargetDescriptors.emplace_back(TestImpact::NativeTargetDescriptor{TestImpact::TargetDescriptor{"TestTargetA", "", }, {""}});
            m_NativeTargetDescriptors.emplace_back(TestImpact::NativeTargetDescriptor{TestImpact::TargetDescriptor{"TestTargetB", "", }, {""}});
            m_NativeTargetDescriptors.emplace_back(TestImpact::NativeTargetDescriptor{TestImpact::TargetDescriptor{"ProductionTargetA", "", }, {""}});
            m_NativeTargetDescriptors.emplace_back(TestImpact::NativeTargetDescriptor{TestImpact::TargetDescriptor{"ProductionTargetB", "", }, {""}});
            m_NativeTargetDescriptors.emplace_back(TestImpact::NativeTargetDescriptor{TestImpact::TargetDescriptor{"ProductionTargetC", "", }, {""}});

            m_NativeTestTargetMetaMap.emplace("TestTargetA", TestImpact::NativeTestTargetMeta{TestImpact::TestSuiteMeta{"", AZStd::chrono::milliseconds{0}}, "", TestImpact::LaunchMethod::TestRunner});
            m_NativeTestTargetMetaMap.emplace("TestTargetB", TestImpact::NativeTestTargetMeta{TestImpact::TestSuiteMeta{"", AZStd::chrono::milliseconds{0}}, "", TestImpact::LaunchMethod::StandAlone});
        }

    protected:
        AZStd::vector<TestImpact::NativeTargetDescriptor> m_NativeTargetDescriptors;
        TestImpact::NativeTestTargetMetaMap m_NativeTestTargetMetaMap;
    };

    TestImpact::NativeProductionTargetDescriptor ConstructProductionTargetDescriptor(const AZStd::string& name)
    {
        return TestImpact::NativeProductionTargetDescriptor{TestImpact::NativeTargetDescriptor{TestImpact::TargetDescriptor{name, "", }, {""}}};
    }

    TestImpact::NativeTestTargetDescriptor ConstructTestTargetDescriptor(const AZStd::string& name, TestImpact::LaunchMethod launchMethod)
    {
        return TestImpact::NativeTestTargetDescriptor{
            TestImpact::NativeTargetDescriptor{TestImpact::TargetDescriptor{name, "", }, {""}},
            TestImpact::NativeTestTargetMeta{ TestImpact::TestSuiteMeta{ "", AZStd::chrono::milliseconds{ 0 } }, "", launchMethod }
        };
    }

    TEST_F(TargetDescriptorCompilerTestFixture, EmptyNativeTargetDescriptorList_ExpectArtifactException)
    {
        try
        {
            // Given an empty build target descriptor list but valid test target meta map
            // When attempting to construct the test target
            const auto& [productionTargetDescriptors, testTargetDescriptors] =
                TestImpact::CompileTargetDescriptors({}, AZStd::move(m_NativeTestTargetMetaMap));

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

    TEST_F(TargetDescriptorCompilerTestFixture, EmptyNativeTestTargetMetaMap_ExpectArtifactException)
    {
        try
        {
            // Given a valid build target descriptor list but empty test target meta map
            // When attempting to construct the test target
            const auto& [productionTargetDescriptors, testTargetDescriptors] =
                TestImpact::CompileTargetDescriptors(AZStd::move(m_NativeTargetDescriptors), {});

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
            m_NativeTestTargetMetaMap.emplace("Orphan", TestImpact::NativeTestTargetMeta{TestImpact::TestSuiteMeta{"", AZStd::chrono::milliseconds{0}}, "", TestImpact::LaunchMethod::TestRunner});

            // When attempting to construct the test target
            const auto& [productionTargetDescriptors, testTargetDescriptors] =
                TestImpact::CompileTargetDescriptors(AZStd::move(m_NativeTargetDescriptors), {});

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
            TestImpact::CompileTargetDescriptors(AZStd::move(m_NativeTargetDescriptors), AZStd::move(m_NativeTestTargetMetaMap));

        // Expect the production targets to match the expected targets
        EXPECT_TRUE(productionTargetDescriptors.size() == 3);
        EXPECT_TRUE(*productionTargetDescriptors[0] == ConstructProductionTargetDescriptor("ProductionTargetA"));
        EXPECT_TRUE(*productionTargetDescriptors[1] == ConstructProductionTargetDescriptor("ProductionTargetB"));
        EXPECT_TRUE(*productionTargetDescriptors[2] == ConstructProductionTargetDescriptor("ProductionTargetC"));

        // Expect the test targets to match the expected targets
        EXPECT_TRUE(testTargetDescriptors.size() == 2);
        EXPECT_TRUE(*testTargetDescriptors[0] == ConstructTestTargetDescriptor("TestTargetA", TestImpact::LaunchMethod::TestRunner));
        EXPECT_TRUE(*testTargetDescriptors[1] == ConstructTestTargetDescriptor("TestTargetB", TestImpact::LaunchMethod::StandAlone));
    }

} // namespace UnitTest
