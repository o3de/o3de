/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <AzCore/Memory/SystemAllocator.h>
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
        // Create application and descriptor
        m_application = aznew AZ::ComponentApplication;
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_useExistingAllocator = true;

        // Create system entity
        AZ::ComponentApplication::StartupParameters startupParams;
        m_systemEntity = m_application->Create(appDesc, startupParams);
        AZ_TEST_ASSERT(m_systemEntity);
        m_systemEntity->Init();
        m_systemEntity->Activate();
    }

    void NumericalMethodsTestEnvironment::TeardownEnvironment()
    {
        delete m_application;
    }

} // namespace NumericalMethods

AZ_UNIT_TEST_HOOK(new NumericalMethods::NumericalMethodsTestEnvironment);
