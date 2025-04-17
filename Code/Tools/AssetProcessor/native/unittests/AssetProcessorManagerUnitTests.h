/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#if !defined(Q_MOC_RUN)
#include <native/FileWatcher/FileWatcher.h>
#include <native/unittests/UnitTestUtils.h>
#include <native/unittests/AssetProcessorUnitTests.h>
#include <native/tests/UnitTestUtilities.h>
#include <native/utilities/AssetUtilEBusHelper.h>
#include <native/utilities/UuidManager.h>
#include <AzToolsFramework/Metadata/MetadataManager.h>
#endif

namespace AssetProcessor
{
    class AssetProcessorManagerUnit_Test;

    class AssetProcessorManagerUnitTests
        : public QObject
        , public UnitTest::AssetProcessorUnitTestBase
    {
        Q_OBJECT

    protected:
        AssetProcessorManagerUnitTests() = default;
        ~AssetProcessorManagerUnitTests();

        void SetUp() override;
        void TearDown() override;

        AZStd::string AbsProductPathToRelative(const QString& absolutePath);
        void VerifyProductPaths(const JobDetails& jobDetails);

        void ProcessAssetBlockUntilComplete(QString& assetToProcess);

        QDir m_sourceRoot;
        QDir m_cacheRoot;

        AZStd::unique_ptr<AssetProcessor::FileStatePassthrough> m_fileStateCache;
        AZStd::unique_ptr<UnitTestUtils::ScopedDir> m_changeDir;
        FileWatcher m_fileWatcher;
        ::UnitTests::MockVirtualFileIO m_virtualFileIO;
        AzToolsFramework::UuidUtilComponent m_uuidUtil;
        AzToolsFramework::MetadataManager m_metadataManager;
        AssetProcessor::UuidManager m_uuidManager;

        QList<QMetaObject::Connection> m_assetProcessorConnections;
        bool m_idling = false;
        QList<JobDetails> m_processResults;
        QList<QPair<QString, QString>> m_changedInputResults;
        QList<AzFramework::AssetSystem::AssetNotificationMessage> m_assetMessages;

        AZStd::unique_ptr<AssetProcessorManagerUnit_Test> m_assetProcessorManager;
        PlatformConfiguration m_config;
    };
} // namespace assetprocessor

