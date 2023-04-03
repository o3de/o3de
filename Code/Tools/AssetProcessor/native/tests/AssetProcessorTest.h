/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/Application/Application.h>
#include <native/utilities/assetUtils.h>
#include <native/unittests/UnitTestUtils.h> // for the assert absorber.
#include <AssetManager/FileStateCache.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <tests/ApplicationManagerTests.h>
#include "UnitTestUtilities.h"

namespace AssetProcessor
{
    struct IUnitTestAppManager
    {
        AZ_RTTI(IUnitTestAppManager, "{37578207-790A-4928-BD47-B9C4F4B49C3A}");

        virtual PlatformConfiguration& GetConfig() = 0;
    };

    // This is an utility class for Asset Processor Tests
    // Any gmock based fixture class can derived from this class and this will automatically do system allocation and teardown for you
    // It is important to note that if you are overriding Setup and Teardown functions of your fixture class than please call the base class functions.
    class AssetProcessorTest
        : public ::UnitTest::LeakDetectionFixture
    {
    protected:
        AZStd::unique_ptr<UnitTestUtils::AssertAbsorber> m_errorAbsorber{};
        AZStd::unique_ptr<FileStatePassthrough> m_fileStateCache{};

        void SetUp() override
        {
            m_errorAbsorber = AZStd::make_unique<UnitTestUtils::AssertAbsorber>();
            m_application = AZStd::make_unique<AzFramework::Application>();
            m_fileStateCache = AZStd::make_unique<FileStatePassthrough>();

            // Inject the AutomatedTesting project as a project path into test fixture
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            constexpr auto projectPathKey = FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey)
                + "/project_path";
            if(auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                AZ::IO::FixedMaxPath enginePath;
                settingsRegistry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
                settingsRegistry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*settingsRegistry);

                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "SystemComponentsActivated");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "SystemComponentsDeactivated");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "ReflectionManagerAvailable");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "ReflectionManagerUnavailable");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "SystemAllocatorCreated");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "SystemAllocatorPendingDestruction");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "SettingsRegistryAvailable");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "SettingsRegistryUnavailable");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "ConsoleAvailable");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "ConsoleUnavailable");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "GemsLoaded");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "GemsUnloaded");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "FileIOAvailable");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "FileIOUnavailable");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "LegacySystemInterfaceCreated");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "CriticalAssetsCompiled");
                AZ::ComponentApplicationLifecycle::RegisterEvent(*settingsRegistry, "LegacyCommandLineProcessed");
            }
        }

        void TearDown() override
        {
            AssetUtilities::ResetAssetRoot();

            m_fileStateCache.reset();
            m_application.reset();
            m_errorAbsorber.reset();
        }

        AZStd::unique_ptr<AzFramework::Application> m_application;
    };
}

