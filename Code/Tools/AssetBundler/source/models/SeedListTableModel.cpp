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
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalogManager.h>

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
        , m_errorImage(QStringLiteral(":/stylesheet/img/logging/error.svg"))
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
        const auto& enabledPlatforms = AzToolsFramework::PlatformAddressedAssetCatalogManager::GetEnabledPlatforms();
        [[maybe_unused]] const bool hasEnabledPlatforms = !enabledPlatforms.empty();
        AZ_Error(AssetBundler::AppWindowName, hasEnabledPlatforms, "Unable to find any enabled asset platforms. Please verify the Asset Processor has run and generated assets successfully.");

        [[maybe_unused]] bool missingAssets = false;

        for (const auto& seed : m_seedListManager->GetAssetSeedList())
        {
            for (AZ::PlatformId platformId : enabledPlatforms)
            {
                if (AZ::PlatformHelper::HasPlatformFlag(seed.m_platformFlags, platformId))
                {
                    assetInfo = AzToolsFramework::AssetSeedManager::GetAssetInfoById(
                        seed.m_assetId,
                        platformId,
                        absolutePath,
                        seed.m_assetRelativePath);
                    if (assetInfo.m_assetId.IsValid())
                    {
                        break;
                    }
                }
            }

            platformList = QString(m_seedListManager->GetReadablePlatformList(seed).c_str());

            m_additionalSeedInfoMap[seed.m_assetId].reset(new AdditionalSeedInfo(assetInfo.m_relativePath.c_str(), platformList));

            // Missing assets still show up in the seed list view. Display An error message where the blank filename would otherwise be.
            if (!assetInfo.m_assetId.IsValid())
            {
                const AZStd::string assetIdStr(seed.m_assetId.ToString<AZStd::string>());
                m_additionalSeedInfoMap[seed.m_assetId]->m_errorMessage = tr("Asset not found for enabled platforms: path hint '%1', asset ID '%2'").arg(seed.m_assetRelativePath.c_str()).arg(assetIdStr.c_str());
                missingAssets = true;
            }
        }

        AZ_Warning(AssetBundler::AppWindowName, !missingAssets, "Not all assets were found. Please verify the Asset Processor has run for the enabled platforms and generated assets successfully.");
    }

    AZ::Outcome<AzFramework::PlatformFlags, void> SeedListTableModel::GetSeedPlatforms(const QModelIndex& index) const
    {
        auto seedOutcome = GetSeedInfo(index);
        if (!seedOutcome.IsSuccess())
        {
            // Error has already been thrown
            return AZ::Failure();
        }

        return AZ::Success(seedOutcome.GetValue().get().m_platformFlags);
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
        auto additionalSeedInfo = m_additionalSeedInfoMap.find(seedOutcome.GetValue().get().m_assetId);
        if (additionalSeedInfo == m_additionalSeedInfoMap.end())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Unable to find additional Seed info");
            return false;
        }

        auto visiblePlatforms = platforms;
#ifndef AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
        // don't include restricted platforms when they are not enabled
        visiblePlatforms &= AzFramework::PlatformFlags::UnrestrictedPlatforms;
#endif
        additionalSeedInfo->second->m_platformList =
            QString(AzFramework::PlatformHelper::GetCommaSeparatedPlatformList(visiblePlatforms).c_str());

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

        AzFramework::PlatformFlags validPlatforms = addSeedsResult.second;
        if (!addSeedsResult.first.IsValid() || validPlatforms == AzFramework::PlatformFlags::Platform_NONE)
        {
            // Error has already been thrown
            return false;
        }

#ifndef AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
        // don't include restricted platforms when they are not enabled
        validPlatforms &= AzFramework::PlatformFlags::UnrestrictedPlatforms;
#endif
        QString platformList = QString(AzFramework::PlatformHelper::GetCommaSeparatedPlatformList(validPlatforms).c_str());

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
        m_seedListManager->RemoveSeedAsset(seedOutcome.GetValue().get().m_assetId, seedOutcome.GetValue().get().m_platformFlags);
        m_additionalSeedInfoMap.erase(seedOutcome.GetValue().get().m_assetId);
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
        case Qt::DecorationRole:
            if (index.column() == Column::ColumnRelativePath)
            {
                if (!additionalSeedInfoOutcome.GetValue()->m_errorMessage.isEmpty())
                {
                    return m_errorImage;
                }
            }
            break;
        case Qt::DisplayRole:
        {
            if (index.column() == Column::ColumnRelativePath)
            {
                // If this seed has an error, display that instead of the path.
                if (!additionalSeedInfoOutcome.GetValue()->m_errorMessage.isEmpty())
                {
                    return additionalSeedInfoOutcome.GetValue()->m_errorMessage;
                }
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

    AZ::Outcome<AZStd::reference_wrapper<const AzFramework::SeedInfo>, void> SeedListTableModel::GetSeedInfo(const QModelIndex& index) const
    {
        int row = index.row();
        int col = index.column();
        if (row >= rowCount() || row < 0 || col >= columnCount() || col < 0)
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Selected index (%i, %i) is out of range", row, col);
            return AZ::Failure();
        }

        return m_seedListManager->GetAssetSeedList().at(row);
    }

    AZ::Outcome<AdditionalSeedInfoPtr, void> SeedListTableModel::GetAdditionalSeedInfo(const QModelIndex& index) const
    {
        auto seedInfoOutcome = GetSeedInfo(index);
        if (!seedInfoOutcome.IsSuccess())
        {
            // Error has already been thrown
            return AZ::Failure();
        }

        auto additionalSeedInfoIt = m_additionalSeedInfoMap.find(seedInfoOutcome.GetValue().get().m_assetId);
        if (additionalSeedInfoIt == m_additionalSeedInfoMap.end())
        {
            return AZ::Failure();
        }

        return AZ::Success(additionalSeedInfoIt->second);
    }

} // namespace AssetBundler
