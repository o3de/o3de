/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/models/SeedListTableModel.h>

#include <source/utils/utils.h>

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalog.h>

#include <QFont>

namespace AssetBundler
{


    //////////////////////////////////////////////////////////////////////////////////////////////////
    // AdditionalSeedInfo
    //////////////////////////////////////////////////////////////////////////////////////////////////
    AdditionalSeedInfo::AdditionalSeedInfo(const QString& relativePath, const QString& platformList)
        : m_relativePath(relativePath)
        , m_platformList(platformList)
    {
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // SeedListTableModel
    //////////////////////////////////////////////////////////////////////////////////////////////////
    SeedListTableModel::SeedListTableModel(
        QObject* parent,
        const AZStd::string& absolutePath,
        const AZStd::vector<AZStd::string>& defaultSeeds,
        const AzFramework::PlatformFlags& platforms)
        : QAbstractTableModel(parent)
    {
        m_seedListManager.reset(new AzToolsFramework::AssetSeedManager());

        if (absolutePath.empty() && defaultSeeds.empty())
        {
            return;
        }

        if (!defaultSeeds.empty())
        {
            for (const AZStd::string& seed : defaultSeeds)
            {
                m_seedListManager->AddSeedAssetForValidPlatforms(seed, platforms);
            }

            m_isFileOnDisk = false;
        }
        else
        {
            m_seedListManager->Load(absolutePath);
        }

        AZ::Data::AssetInfo assetInfo;
        QString platformList;
        for (const auto& seed : m_seedListManager->GetAssetSeedList())
        {
            assetInfo = AzToolsFramework::AssetSeedManager::GetAssetInfoById(
                seed.m_assetId,
                AzFramework::PlatformHelper::GetPlatformIndicesInterpreted(seed.m_platformFlags)[0],
                absolutePath,
                seed.m_assetRelativePath);
            platformList = QString(m_seedListManager->GetReadablePlatformList(seed).c_str());

            m_additionalSeedInfoMap[seed.m_assetId].reset(new AdditionalSeedInfo(assetInfo.m_relativePath.c_str(), platformList));
        }
    }

    AZ::Outcome<AzFramework::PlatformFlags, void> SeedListTableModel::GetSeedPlatforms(const QModelIndex& index) const
    {
        auto seedOutcome = GetSeedInfo(index);
        if (!seedOutcome.IsSuccess())
        {
            // Error has already been thrown
            return AZ::Failure();
        }

        return AZ::Success(seedOutcome.GetValue().m_platformFlags);
    }

    bool SeedListTableModel::Save(const AZStd::string& absolutePath)
    {
        if (!HasUnsavedChanges())
        {
            // There are no changes, so there is nothing to save
            return true;
        }

        SetHasUnsavedChanges(!m_seedListManager->Save(absolutePath));
        return !HasUnsavedChanges();
    }

    bool SeedListTableModel::SetSeedPlatforms(const QModelIndex& index, const AzFramework::PlatformFlags& platforms)
    {
        auto seedOutcome = GetSeedInfo(index);
        if (!seedOutcome.IsSuccess())
        {
            // Error has already been thrown
            return false;
        }

        if (platforms == AzFramework::PlatformFlags::Platform_NONE)
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Cannot Edit Platforms: No platforms were selected");
            return false;
        }

        auto setPlatformOutcome = m_seedListManager->SetSeedPlatformFlags(index.row(), platforms);
        if (!setPlatformOutcome.IsSuccess())
        {
            AZ_Error(AssetBundler::AppWindowName, false, setPlatformOutcome.GetError().c_str());
            return false;
        }

        // Update the cached display info
        auto additionalSeedInfo = m_additionalSeedInfoMap.find(seedOutcome.GetValue().m_assetId);
        if (additionalSeedInfo == m_additionalSeedInfoMap.end())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Unable to find additional Seed info");
            return false;
        }
        additionalSeedInfo->second->m_platformList =
            QString(AzFramework::PlatformHelper::GetCommaSeparatedPlatformList(platforms).c_str());

        SetHasUnsavedChanges(true);

        // Update the display
        QModelIndex firstChangedIndex = QAbstractTableModel::index(index.row(), Column::ColumnRelativePath);
        QModelIndex lastChangedIndex = QAbstractTableModel::index(index.row(), Column::Max - 1);
        emit dataChanged(firstChangedIndex, lastChangedIndex, { Qt::DisplayRole });

        return true;
    }

    bool SeedListTableModel::AddSeed(const AZStd::string& seedRelativePath, const AzFramework::PlatformFlags& platforms)
    {
        AZStd::pair<AZ::Data::AssetId, AzFramework::PlatformFlags> addSeedsResult =
            m_seedListManager->AddSeedAssetForValidPlatforms(seedRelativePath, platforms);

        if (!addSeedsResult.first.IsValid() || addSeedsResult.second == AzFramework::PlatformFlags::Platform_NONE)
        {
            // Error has already been thrown
            return false;
        }

        QString platformList = QString(AzFramework::PlatformHelper::GetCommaSeparatedPlatformList(addSeedsResult.second).c_str());

        int lastRowIndex = AZStd::max(rowCount() - 1, 0);
        beginInsertRows(QModelIndex(), lastRowIndex, lastRowIndex);

        m_additionalSeedInfoMap[addSeedsResult.first].reset(new AdditionalSeedInfo(QString(seedRelativePath.c_str()), platformList));

        endInsertRows();

        SetHasUnsavedChanges(true);
        return true;
    }

    bool SeedListTableModel::RemoveSeed(const QModelIndex& seedIndex)
    {
        auto seedOutcome = GetSeedInfo(seedIndex);
        if (!seedOutcome.IsSuccess())
        {
            // Error has already been thrown
            return false;
        }

        int row = seedIndex.row();
        beginRemoveRows(QModelIndex(), row, row);
        m_seedListManager->RemoveSeedAsset(seedOutcome.GetValue().m_assetId, seedOutcome.GetValue().m_platformFlags);
        m_additionalSeedInfoMap.erase(seedOutcome.GetValue().m_assetId);
        endRemoveRows();

        SetHasUnsavedChanges(true);
        return true;
    }

    int SeedListTableModel::rowCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : static_cast<int>(m_additionalSeedInfoMap.size());
    }

    int SeedListTableModel::columnCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : Column::Max;
    }

    QVariant SeedListTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case Column::ColumnRelativePath:
                return QString("Seed");
            case Column::ColumnPlatformList:
                return QString("Platforms");
            default:
                break;
            }
        }

        return QVariant();
    }

    QVariant SeedListTableModel::data(const QModelIndex& index, int role) const
    {
        auto additionalSeedInfoOutcome = GetAdditionalSeedInfo(index);
        if (!additionalSeedInfoOutcome.IsSuccess())
        {
            return QVariant();
        }

        switch (role)
        {
        case Qt::DisplayRole:
        {
            if (index.column() == Column::ColumnRelativePath)
            {
                return additionalSeedInfoOutcome.GetValue()->m_relativePath;
            }
            else if (index.column() == Column::ColumnPlatformList)
            {
                return additionalSeedInfoOutcome.GetValue()->m_platformList;
            }
        }
        default:
            break;
        }

        return QVariant();
    }

    AZ::Outcome<AzFramework::SeedInfo&, void> SeedListTableModel::GetSeedInfo(const QModelIndex& index) const
    {
        int row = index.row();
        int col = index.column();
        if (row >= rowCount() || row < 0 || col >= columnCount() || col < 0)
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Selected index (%i, %i) is out of range", row, col);
            return AZ::Failure();
        }

        return AZ::Success(m_seedListManager->GetAssetSeedList().at(row));
    }

    AZ::Outcome<AdditionalSeedInfoPtr, void> SeedListTableModel::GetAdditionalSeedInfo(const QModelIndex& index) const
    {
        auto seedInfoOutcome = GetSeedInfo(index);
        if (!seedInfoOutcome.IsSuccess())
        {
            // Error has already been thrown
            return AZ::Failure();
        }

        auto additionalSeedInfoIt = m_additionalSeedInfoMap.find(seedInfoOutcome.GetValue().m_assetId);
        if (additionalSeedInfoIt == m_additionalSeedInfoMap.end())
        {
            return AZ::Failure();
        }

        return AZ::Success(additionalSeedInfoIt->second);
    }

} // namespace AssetBundler
