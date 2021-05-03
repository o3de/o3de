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

#pragma once

#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzFramework/Platform/PlatformDefaults.h>

#include <QAbstractTableModel>

namespace AssetBundler
{
    class AssetListTableModel
        : public QAbstractTableModel
    {
    public:
        explicit AssetListTableModel(QObject* parent = nullptr, const AZStd::string& absolutePath = AZStd::string(), const AZStd::string& platform = "");
        virtual ~AssetListTableModel() {}

        AZStd::shared_ptr<AzToolsFramework::AssetSeedManager> GetSeedListManager() { return m_seedListManager; }

        //////////////////////////////////////////////////////////////////////////
        // QAbstractListModel overrides
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        //////////////////////////////////////////////////////////////////////////

        enum Column
        {
            ColumnAssetName,
            ColumnRelativePath,
            ColumnAssetId,
            Max
        };

    private:
        AZ::Outcome<AzToolsFramework::AssetFileInfo&, AZStd::string> GetAssetFileInfo(const QModelIndex& index) const;

        AZStd::shared_ptr<AzToolsFramework::AssetSeedManager> m_seedListManager;
        AzToolsFramework::AssetFileInfoList m_assetFileInfoList;
        AzFramework::PlatformId m_platformId;
    };
}
