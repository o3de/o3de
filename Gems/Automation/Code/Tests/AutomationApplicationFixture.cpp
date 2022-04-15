/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AutomationApplicationFixture.h>

#include <Automation/AutomationBus.h>

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <AzFramework/IO/LocalFileIO.h>


namespace UnitTest
{
    void AutomationApplicationFixture::SetUp()
    {
        AllocatorsFixture::SetUp();
    }

    void AutomationApplicationFixture::TearDown()
    {
        if (m_application)
        {
            DestroyApplication();
        }

        AllocatorsFixture::TearDown();
    }

    AzFramework::Application* AutomationApplicationFixture::CreateApplication(const char* scriptPath, bool exitOnFinish)
    {
        if (scriptPath)
        {
            m_args.push_back("--run-automation-suite");
            m_args.push_back(scriptPath);

            if (exitOnFinish)
            {
                m_args.push_back("--exit-on-automation-end");
            }
        }

        int argc = aznumeric_cast<int>(m_args.size());
        char** argv = const_cast<char**>(m_args.data());

        m_application = aznew AzFramework::Application(&argc, &argv);

        // ensure the Automation gem is active
        AZ::Test::AddActiveGem("Automation", *AZ::SettingsRegistry::Get(), AZ::IO::FileIOBase::GetInstance());

        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_useExistingAllocator = true;

        AZ::DynamicModuleDescriptor dynamicModuleDescriptor;
        dynamicModuleDescriptor.m_dynamicLibraryPath = "Automation";
        appDesc.m_modules.push_back(dynamicModuleDescriptor);

        m_application->Start(appDesc);

        return m_application;
    }

    void AutomationApplicationFixture::DestroyApplication()
    {
        m_application->Stop();
        delete m_application;
        m_application = nullptr;
    }
} // namespace UnitTest
