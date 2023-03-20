/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QDir>
#include <QTemporaryDir>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <native/utilities/assetUtils.h>

namespace AssetProcessor
{
    // Mock the AssetDatabaseRequests handler for retrieving the asset database location in unit tests
    class MockAssetDatabaseRequestsHandler
        : public AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler
    {
    public:
        MockAssetDatabaseRequestsHandler()
        {
            // the canonicalization of the path here is to get around the fact that on some platforms
            // the "temporary" folder location could be junctioned into some other folder and getting "QDir::current()"
            // and other similar functions may actually return a different string but still be referring to the same folder
            QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(QDir(m_temporaryDir.path()).canonicalPath());
            m_assetDatabasePath = QDir(canonicalTempDirPath).absoluteFilePath("test_database.sqlite").toUtf8().data();

            BusConnect();
        }

        ~MockAssetDatabaseRequestsHandler()
        {
            BusDisconnect();
        }

        bool GetAssetDatabaseLocation(AZStd::string& location) override
        {
            location = m_assetDatabasePath;

            return true;
        }

        AZStd::string GetAssetRootDir()
        {
            return QFileInfo(m_assetDatabasePath.c_str()).dir().path().toUtf8().data();
        }

        AZStd::string m_assetDatabasePath;

    private:
        QTemporaryDir m_temporaryDir;
    };
}
