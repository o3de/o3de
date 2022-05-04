/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptAutomationApplicationFixture.h>

#include <ScriptAutomation/ScriptAutomationBus.h>

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <AzFramework/IO/LocalFileIO.h>


namespace UnitTest
{
    void ScriptAutomationApplicationFixture::SetUp()
    {
        AllocatorsFixture::SetUp();
    }

    void ScriptAutomationApplicationFixture::TearDown()
    {
        if (m_application)
        {
            DestroyApplication();
        }

        AllocatorsFixture::TearDown();
    }

    AzFramework::Application* ScriptAutomationApplicationFixture::CreateApplication(const char* scriptPath, bool exitOnFinish)
    {
        // The ScriptAutomation gem uses the AssetManager to load the script assets. The AssetManager will try to make the asset path relative to 
        // the cache root folder. If we pass in an abosolute path to the asset, the AssetManager ends up removing the leading slash on MacOS 
        // and Linux in the call to Application::MakePathRelative. So we need to override the cache path for these tests. First, we override 
        // the asset platform folder since we're going to be reading from the gem tests folder for all platforms.
        const auto cachePlatformOverride = 
            AZ::StringFunc::Path::FixedString::format("--regset=%s/%s_assets=.", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, AZ_TRAIT_OS_PLATFORM_CODENAME_LOWER);
        
        if (scriptPath)
        {
            m_args.push_back(cachePlatformOverride.c_str());
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

        // ensure the ScriptAutomation gem is active
        AZ::Test::AddActiveGem("ScriptAutomation", *AZ::SettingsRegistry::Get(), AZ::IO::FileIOBase::GetInstance());

        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_useExistingAllocator = true;

        AZ::DynamicModuleDescriptor dynamicModuleDescriptor;
        dynamicModuleDescriptor.m_dynamicLibraryPath = "ScriptAutomation";
        appDesc.m_modules.push_back(dynamicModuleDescriptor);

        // We need the resolved gem root folder since storing aliases in the settings registry will cause StorageDrive to try to read from 
        // unresolved paths. The resolved gem root can only be found after the Application is instantiated. So we override the project cache 
        // path here instead of passing it as a command line parameter above. The cache root folder is the <project_cache_path>/<asset_platform_folder>.
        if (auto registry = AZ::SettingsRegistry::Get(); registry != nullptr)
        {
            const auto gemPathKey = AZ::StringFunc::Path::FixedString::format("%s/ScriptAutomation/Path", AZ::SettingsRegistryMergeUtils::ManifestGemsRootKey);
            if (AZ::IO::Path gemRootPath; registry->Get(gemRootPath.Native(), gemPathKey))
            {
                AZ::IO::Path cachePath = gemRootPath / "Code/Tests/Scripts";
                const auto cachePathKey = AZ::StringFunc::Path::FixedString::format("%s/project_cache_path", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey);
                registry->Set(cachePathKey, cachePath.Native());
            }
        }

        m_application->Start(appDesc);

        return m_application;
    }

    void ScriptAutomationApplicationFixture::DestroyApplication()
    {
        m_application->Stop();
        delete m_application;
        m_application = nullptr;
    }
} // namespace UnitTest
