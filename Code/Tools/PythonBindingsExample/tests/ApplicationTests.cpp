/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <source/Application.h>
#include <source/ApplicationParameters.h>

namespace PythonBindingsExample
{
    //! This function returns the build system target name
    AZStd::string_view GetBuildTargetName()
    {
#if !defined (LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return AZStd::string_view{ LY_CMAKE_TARGET };
    }

    class PythonBindingsExampleTest
        : public ::testing::Test
        , public ::UnitTest::AllocatorsBase
    {
    public:

        static void SetUpTestCase()
        {
            if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
            }

            s_application = AZStd::make_unique<PythonBindingsExample::Application>(nullptr, nullptr);
            // The AZ::ComponentApplication constructor creates the settings registry
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
                *AZ::SettingsRegistry::Get(), GetBuildTargetName());
            s_application->SetUp();
        }

        static void TearDownTestCase()
        {
            s_application->TearDown();
            s_application.reset();

            if (AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            }
        }

        static AZStd::unique_ptr<PythonBindingsExample::Application> s_application;
    };

    AZStd::unique_ptr<PythonBindingsExample::Application> PythonBindingsExampleTest::s_application;

    TEST_F(PythonBindingsExampleTest, Application_Run_Succeeds)
    {
        EXPECT_TRUE(s_application->Run());
    }

    TEST_F(PythonBindingsExampleTest, Application_RunWithParameters_Works)
    {
        ApplicationParameters params;
        EXPECT_TRUE(s_application->RunWithParameters(params));
    }

    TEST_F(PythonBindingsExampleTest, Application_ImportSys_Works)
    {
        ApplicationParameters params;
        params.m_pythonStatement = "import sys";
        EXPECT_TRUE(s_application->RunWithParameters(params));
    }

    TEST_F(PythonBindingsExampleTest, Application_ImportAzLmbr_Works)
    {
        ApplicationParameters params;
        params.m_pythonStatement = "import azlmbr";
        EXPECT_TRUE(s_application->RunWithParameters(params));
    }

    TEST_F(PythonBindingsExampleTest, Application_ImportAzLmbrPaths_Works)
    {
        ApplicationParameters params;
        params.m_pythonStatement = "import azlmbr.paths; print (azlmbr.paths.engroot)";
        EXPECT_TRUE(s_application->RunWithParameters(params));
    }

    TEST_F(PythonBindingsExampleTest, Application_SystemExit_Blocked)
    {
        int exceptions = 0;
        int errors = 0;

        s_application->GetErrorCount(exceptions, errors);
        ASSERT_EQ(exceptions, 0);
        ASSERT_EQ(errors, 0);

        // expects a clean "error" from this statement
        // the whole program should not exit()
        {
            ApplicationParameters params;
            params.m_pythonStatement = "import sys; sys.exit(0)";
            EXPECT_FALSE(s_application->RunWithParameters(params));

            s_application->GetErrorCount(exceptions, errors);
            EXPECT_EQ(exceptions, 0);
            EXPECT_GE(errors, 1);
        }
        s_application->ResetErrorCount();

        // should be able to run more statements
        {
            ApplicationParameters params;
            params.m_pythonStatement = "import sys";
            EXPECT_TRUE(s_application->RunWithParameters(params));

            s_application->GetErrorCount(exceptions, errors);
            EXPECT_EQ(exceptions, 0);
            EXPECT_EQ(errors, 0);
        }
    }
}
