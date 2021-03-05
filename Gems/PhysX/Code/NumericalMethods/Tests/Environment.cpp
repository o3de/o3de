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

#include <NumericalMethods_precompiled.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzFramework/Application/Application.h>
#include <Tests/Environment.h>

namespace NumericalMethods
{
    void ExpectClose(const VectorVariable& actual, const VectorVariable& expected, double tolerance)
    {
        ASSERT_TRUE(actual.GetDimension() == expected.GetDimension());
        for (AZ::u32 i = 0; i < actual.GetDimension(); i++)
        {
            EXPECT_NEAR(actual[i], expected[i], tolerance);
        }
    }

    void ExpectClose(const AZStd::vector<double>& actual, const AZStd::vector<double>& expected, double tolerance)
    {
        ASSERT_TRUE(actual.size() == expected.size());
        for (AZ::u32 i = 0; i < actual.size(); i++)
        {
            EXPECT_NEAR(actual[i], expected[i], tolerance);
        }
    }

    // Global test environment.
    class NumericalMethodsTestEnvironment
        : public AZ::Test::ITestEnvironment
    {
    protected:
        void SetupEnvironment() override;
        void TeardownEnvironment() override;

        AZ::ComponentApplication* m_application;
        AZ::Entity* m_systemEntity;
    };

    void NumericalMethodsTestEnvironment::SetupEnvironment()
    {
#if AZ_TRAIT_UNITTEST_USE_TEST_RUNNER_ENVIRONMENT
        AZ::EnvironmentInstance inst = AZ::Test::GetPlatform().GetTestRunnerEnvironment();
        AZ::Environment::Attach(inst);
#endif
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

        // Create application and descriptor
        m_application = aznew AZ::ComponentApplication;
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_useExistingAllocator = true;

        // Create system entity
        AZ::ComponentApplication::StartupParameters startupParams;
        m_systemEntity = m_application->Create(appDesc, startupParams);
        AZ_TEST_ASSERT(m_systemEntity);
        m_systemEntity->AddComponent(aznew AZ::MemoryComponent());
        m_systemEntity->Init();
        m_systemEntity->Activate();
    }

    void NumericalMethodsTestEnvironment::TeardownEnvironment()
    {
        delete m_application;
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }

} // namespace NumericalMethods

AZ_UNIT_TEST_HOOK(new NumericalMethods::NumericalMethodsTestEnvironment);
