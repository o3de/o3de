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
#include <native/unittests/UnitTestRunner.h> // for the assert absorber.
#include <AssetManager/FileStateCache.h>

namespace AssetProcessor
{
    // This is an utility class for Asset Processor Tests
    // Any gmock based fixture class can derived from this class and this will automatically do system allocation and teardown for you
    // It is important to note that if you are overriding Setup and Teardown functions of your fixture class than please call the base class functions.
    class AssetProcessorTest
        : public ::testing::Test
    {
    protected:
        AZStd::unique_ptr<UnitTestUtils::AssertAbsorber> m_errorAbsorber{};
        AZStd::unique_ptr<FileStatePassthrough> m_fileStateCache{};

        void SetUp() override
        {
            if (!AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
            {
                m_ownsOSAllocator = true;
                AZ::AllocatorInstance<AZ::OSAllocator>::Create();
            }
            if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                m_ownsSysAllocator = true;
                AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
            }

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
            }
        }

        void TearDown() override
        {
            AssetUtilities::ResetAssetRoot();

            m_fileStateCache.reset();
            m_application.reset();
            m_errorAbsorber.reset();

            if (m_ownsSysAllocator)
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
                m_ownsSysAllocator = false;
            }
            if (m_ownsOSAllocator)
            {
                AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
                m_ownsOSAllocator = false;
            }
        }
        bool m_ownsOSAllocator = false;
        bool m_ownsSysAllocator = false;

        AZStd::unique_ptr<AzFramework::Application> m_application;
    };
}

