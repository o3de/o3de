/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <tests/assetmanager/AssetManagerTestingBase.h>
#include <AzCore/Utils/Utils.h>
#include <native/AssetManager/assetScanFolderInfo.h>

namespace UnitTests
{
    using InitialScanSkipTests = AssetManagerTestingBase;

    TEST_F(InitialScanSkipTests, SanityTest_SkippingDisabled_ProcessesAFile)
    {
        using namespace AssetProcessor;
        // This pair of tests are regression tests for an issue where having scan skipping turned on would result in AP never processing
        // files This particular test verifies everything works when scan skipping is off

        QSet<AssetProcessor::AssetFileInfo> filePaths = {};

        // Simulate running through the scanning phase
        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(),
            "OnAssetScannerStatusChange",
            Qt::QueuedConnection,
            Q_ARG(AssetProcessor::AssetScanningStatus, AssetProcessor::AssetScanningStatus::Started));
        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessFilesFromScanner", Qt::QueuedConnection, Q_ARG(QSet<AssetFileInfo>, filePaths));
        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(),
            "OnAssetScannerStatusChange",
            Qt::QueuedConnection,
            Q_ARG(AssetProcessor::AssetScanningStatus, AssetProcessor::AssetScanningStatus::Completed));

        CreateBuilder("stage1", "*.stage1", "stage2", false, AssetBuilderSDK::ProductOutputFlags::ProductAsset);

        // Test with scan skipping off
        m_assetProcessorManager->SetInitialScanSkippingFeature(false);

        // Update the file
        AZ::Utils::WriteFile("unit test file", m_testFilePath.c_str());

        // Process the file
        ProcessFileMultiStage(1, true);
    }

    TEST_F(InitialScanSkipTests, SkippingEnabled_ProcessesAFile)
    {
        using namespace AssetProcessor;
        // This pair of tests are regression tests for an issue where having scan skipping turned on would result in AP never processing
        // files This particular test verifies files can be processed after scanning is done when scan skipping is enabled

        QSet<AssetProcessor::AssetFileInfo> filePaths = {};
        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(),
            "OnAssetScannerStatusChange",
            Qt::QueuedConnection,
            Q_ARG(AssetProcessor::AssetScanningStatus, AssetProcessor::AssetScanningStatus::Started));
        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessFilesFromScanner", Qt::QueuedConnection, Q_ARG(QSet<AssetFileInfo>, filePaths));
        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(),
            "OnAssetScannerStatusChange",
            Qt::QueuedConnection,
            Q_ARG(AssetProcessor::AssetScanningStatus, AssetProcessor::AssetScanningStatus::Completed));

        CreateBuilder("stage1", "*.stage1", "stage2", false, AssetBuilderSDK::ProductOutputFlags::ProductAsset);

        m_assetProcessorManager->SetInitialScanSkippingFeature(true);
        AZ::Utils::WriteFile("unit test file", m_testFilePath.c_str());
        ProcessFileMultiStage(1, true);
    }
} // namespace UnitTests
