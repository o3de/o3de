/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/models/AssetListTableModel.h>

#include <source/utils/utils.h>

#include <AzFramework/StringFunc/StringFunc.h>

namespace AssetBundler
{
    //////////////////////////////////////////////////////////////////////////////////////////////////
    // AssetListTableModel
    //////////////////////////////////////////////////////////////////////////////////////////////////
    AssetListTableModel::AssetListTableModel(QObject* parent, const AZStd::string& absolutePath, const AZStd::string& platform)
        : QAbstractTableModel(parent)
    {
        m_seedListManager.reset(new AzToolsFramework::AssetSeedManager());

        if (absolutePath.empty() || platform.empty())
        {
            return;
        }

        AZ::Outcome<AzToolsFramework::AssetFileInfoList, AZStd::string> outcome = m_seedListManager->LoadAssetFileInfo(absolutePath);
        if (!outcome.IsSuccess())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Failed to load the asset file info for %s", absolutePath.c_str());
            return;
        }

        m_assetFileInfoList = outcome.TakeValue();
        m_platformId = static_cast<AzFramework::PlatformId>(AzFramework::PlatformHelper::GetPlatformIndexFromName(platform.c_str()));
    }

    int AssetListTableModel::rowCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : static_cast<int>(m_assetFileInfoList.m_fileInfoList.size());
    }

    int AssetListTableModel::columnCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : Column::Max;
    }

    QVariant AssetListTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case Column::ColumnAssetName:
                return QString("Asset Name");
            case Column::ColumnRelativePath:
                return QString("Relative Path");
            case Column::ColumnAssetId:
                return QString("Asset ID");
            default:
                break;
            }
        }

        return QVariant();
    }

    QVariant AssetListTableModel::data(const QModelIndex& index, int role) const
    {
        auto assetFileInfoOutcome = GetAssetFileInfo(index);
        if (!assetFileInfoOutcome.IsSuccess())
        {
            return QVariant();
        }

        switch (role)
        {
        case Qt::DisplayRole:
        {
            switch (index.column())
            {
            case Column::ColumnAssetName:
            {
                AZStd::string fileName = assetFileInfoOutcome.GetValue().get().m_assetRelativePath;
                AzFramework::StringFunc::Path::GetFullFileName(fileName.c_str(), fileName);

                return fileName.c_str();
            }
            case Column::ColumnRelativePath:
            {
                return assetFileInfoOutcome.GetValue().get().m_assetRelativePath.c_str();
            }
            case Column::ColumnAssetId:
            {
                AZStd::string assetIdStr;
                assetFileInfoOutcome.GetValue().get().m_assetId.ToString(assetIdStr);

                return assetIdStr.c_str();
            }
            default:
                break;
            }
        }
        default:
            break;
        }

        return QVariant();
    }

    AZ::Outcome<AZStd::reference_wrapper<const AzToolsFramework::AssetFileInfo>, AZStd::string> AssetListTableModel::GetAssetFileInfo(const QModelIndex& index) const
    {
        int row = index.row();
        int col = index.column();
        if (row >= rowCount() || row < 0 || col >= columnCount() || col < 0)
        {
            return AZ::Failure(AZStd::string::format("Selected index (%i, %i) is out of range", row, col));
        }

        return m_assetFileInfoList.m_fileInfoList.at(row);
    }
}
