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

    TEST_F(PythonBindingsExampleTest, Application_Run_Fails)
    {
        EXPECT_FALSE(s_application->Run());
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
        params.m_pythonStatement = "import azlmbr.paths; print (azlmbr.paths.engroot); print (azlmbr.paths.devroot)";
        EXPECT_TRUE(s_application->RunWithParameters(params));
    }

}
