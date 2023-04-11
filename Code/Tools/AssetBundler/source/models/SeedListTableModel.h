/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <source/models/AssetBundlerAbstractFileTableModel.h>

#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzFramework/Platform/PlatformDefaults.h>

#include <QIcon>
#include <QSharedPointer>

namespace AssetBundler
{
    struct AdditionalSeedInfo
    {
        AdditionalSeedInfo(const QString& relativePath, const QString& platformList);

        QString m_relativePath;
        QString m_platformList;
        QString m_errorMessage; // If the asset isn't available, this will contain an error message to display instead.
    };

    using AdditionalSeedInfoPtr = AZStd::shared_ptr<AdditionalSeedInfo>;
    using AdditionalSeedInfoMap = AZStd::unordered_map<AZ::Data::AssetId, AdditionalSeedInfoPtr>;

    class SeedListTableModel
        : public QAbstractTableModel
    {
    public:
        explicit SeedListTableModel(
            QObject* parent = nullptr,
            const AZStd::string& absolutePath = AZStd::string(),
            const AZStd::vector<AZStd::string>& defaultSeeds = AZStd::vector<AZStd::string>(),
            const AzFramework::PlatformFlags& platforms = AzFramework::PlatformFlags::Platform_NONE);
        virtual ~SeedListTableModel() {}

        AZStd::shared_ptr<AzToolsFramework::AssetSeedManager> GetSeedListManager() { return m_seedListManager; }

        bool HasUnsavedChanges() { return m_hasUnsavedChanges && m_isFileOnDisk; }

        void SetHasUnsavedChanges(bool hasUnsavedChanges) { m_hasUnsavedChanges = hasUnsavedChanges; }

        bool Save(const AZStd::string& absolutePath);

        AZ::Outcome<AzFramework::PlatformFlags, void> GetSeedPlatforms(const QModelIndex& index) const;

        bool SetSeedPlatforms(const QModelIndex& index, const AzFramework::PlatformFlags& platforms);

        bool AddSeed(const AZStd::string& seedRelativePath, const AzFramework::PlatformFlags& platforms);

        bool RemoveSeed(const QModelIndex& seedIndex);

        //////////////////////////////////////////////////////////////////////////
        // QAbstractListModel overrides
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        //////////////////////////////////////////////////////////////////////////

        enum Column
        {
            ColumnRelativePath,
            ColumnPlatformList,
            Max
        };

    private:
        AZ::Outcome<AZStd::reference_wrapper<const AzFramework::SeedInfo>, void> GetSeedInfo(const QModelIndex& index) const;

        AZ::Outcome<AdditionalSeedInfoPtr, void> GetAdditionalSeedInfo(const QModelIndex& index) const;

        AZStd::shared_ptr<AzToolsFramework::AssetSeedManager> m_seedListManager;
        AdditionalSeedInfoMap m_additionalSeedInfoMap;

        QIcon m_errorImage;

        bool m_hasUnsavedChanges = false;
        bool m_isFileOnDisk = true;
    };
} // namespace AssetBundler
