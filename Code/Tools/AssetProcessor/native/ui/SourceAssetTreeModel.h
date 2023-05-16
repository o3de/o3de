/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AssetTreeModel.h"
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzCore/std/containers/unordered_map.h>
#include <native/assetprocessor.h>
#include <native/utilities/ApplicationManagerAPI.h>
#include <QDir>

namespace AssetProcessor
{
    class SourceAssetTreeModel : public AssetTreeModel
    {
    public:
        SourceAssetTreeModel(AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> sharedDbConnection, QObject *parent = nullptr);
        ~SourceAssetTreeModel();

        // AssetDatabaseNotificationBus::Handler
        void OnSourceFileChanged(const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry) override;
        void OnSourceFileRemoved(AZ::s64 sourceId) override;

        // Overriding AssetTreeModel for displaying analysis job duration header
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

        QModelIndex GetIndexForSource(const AZStd::string& source, AZ::s64 scanFolderID);

        void SetOnlyShowIntermediateAssets() { m_intermediateAssets = true; }

    public Q_SLOTS:
        void OnCreateJobsDurationChanged(QString sourceName, AZ::s64 scanFolderID);

    protected:
        void ResetModel() override;

        void AddOrUpdateEntry(
            const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& source,
            const AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& scanFolder,
            bool modelIsResetting, AZ::s64 analysisJobDuration = -1);

        void RemoveAssetTreeItem(AssetTreeItem* assetToRemove);
        void RemoveFoldersIfEmpty(AssetTreeItem* itemToCheck);

        AZStd::unordered_map<SourceAndScanID, AssetTreeItem*> m_sourceToTreeItem;
        AZStd::unordered_map<AZ::s64, AssetTreeItem*> m_sourceIdToTreeItem;
        QDir m_assetRoot;
        bool m_assetRootSet = false;
        bool m_intermediateAssets = false;
    };
}
