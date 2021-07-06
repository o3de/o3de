/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <source/models/AssetBundlerAbstractFileTableModel.h>

#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzFramework/Platform/PlatformDefaults.h>

#include <QDateTime>
#include <QSharedPointer>

namespace AssetBundler
{
    class SeedListTableModel;

    class SeedTabWidget;

    struct SeedListFileInfo
    {
        /**
        * Stores information about a Seed List File on disk.
        *
        * @param absolutePath The absolute path of the Seed List File
        * @param fileName The name of the Seed List File. This does not include the ".seed" file extension
        * @param project The area of the codebase the Seed List File is from (ex: ProjectName, Engine, Gem)
        * @param loadFromFile Set to True if you wish to load an existing Seed List File into memory. Set to False if you are creating a new Seed List File.
        */
        SeedListFileInfo(
            const AZStd::string& absolutePath,
            const QString& fileName,
            const QString& project,
            bool loadFromFile,
            bool isDefaultSeedList = false,
            const AZStd::vector<AZStd::string>& defaultSeeds = AZStd::vector<AZStd::string>(),
            const AzFramework::PlatformFlags& platforms = AzFramework::PlatformFlags::Platform_NONE);

        bool SaveSeedFile();

        bool HasUnsavedChanges();

        AZStd::string m_absolutePath;
        bool m_isChecked = false;
        bool m_isDefaultSeedList = false;
        QString m_fileName;
        QString m_project;
        QDateTime m_fileModificationTime;

        QSharedPointer<SeedListTableModel> m_seedListModel;
    };

    using SeedListFileInfoPtr = AZStd::shared_ptr<SeedListFileInfo>;
    /// Stores SeedListFileInfo, using the absolute path (without the drive letter) of the Seed List file as the key
    using SeedListFileInfoMap = AZStd::unordered_map<AZStd::string, SeedListFileInfoPtr>;

    class SeedListFileTableModel
        : public AssetBundlerAbstractFileTableModel
    {

    public:
        explicit SeedListFileTableModel(SeedTabWidget* parentSeedTabWidget);
        virtual ~SeedListFileTableModel();

        void AddDefaultSeedsToInMemoryList(
            const AZStd::vector<AZStd::string>& defaultSeeds,
            const char* projectName,
            const AzFramework::PlatformFlags& platforms);

        AZStd::vector<AZStd::string> CreateNewFiles(
            const AZStd::string& absoluteFilePath,
            const AzFramework::PlatformFlags& platforms,
            const QString& project) override;

        bool DeleteFile(const QModelIndex& index) override;

        void Reload(
            const char* fileExtension,
            const QSet<QString>& watchedFolders,
            const QSet<QString>& watchedFiles = QSet<QString>(),
            const AZStd::unordered_map<AZStd::string, AZStd::string>& pathToProjectNameMap = AZStd::unordered_map<AZStd::string, AZStd::string>()) override;

        void LoadFile(
            const AZStd::string& absoluteFilePath,
            const AZStd::string& projectName = "",
            bool isDefaultFile = false) override;

        void SelectDefaultSeedLists(bool setSelected);

        AZStd::vector<AZStd::string> GenerateAssetLists(const AZStd::string& absoluteFilePath, const AzFramework::PlatformFlags& platforms);

        QSharedPointer<SeedListTableModel> GetSeedListFileContents(const QModelIndex& index);

        bool SetSeedPlatforms(const QModelIndex& seedFileIndex, const QModelIndex& seedIndex, const AzFramework::PlatformFlags& platforms);

        bool AddSeed(const QModelIndex& seedFileIndex, const AZStd::string& seedRelativePath, const AzFramework::PlatformFlags& platforms);

        bool RemoveSeed(const QModelIndex& seedFileIndex, const QModelIndex& seedIndex);

        bool WriteToDisk(const AZStd::string& key) override;

        AZStd::string GetFileAbsolutePath(const QModelIndex& index) const override;

        int GetFileNameColumnIndex() const override;

        int GetTimeStampColumnIndex() const override;

        //////////////////////////////////////////////////////////////////////////
        // QAbstractListModel overrides
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::CheckStateRole) override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        //////////////////////////////////////////////////////////////////////////

        enum Column
        {
            ColumnCheckBox,
            ColumnFileName,
            ColumnProject,
            ColumnFileModificationTime,
            Max
        };

    private:
        AZ::Outcome<SeedListFileInfoPtr, void> GetSeedFileInfo(const QModelIndex& index) const;

        SeedListFileInfoMap m_seedListFileInfoMap;
        AZStd::unordered_set<AZStd::string> m_checkedSeedListFiles;

        AZStd::string m_inMemoryDefaultSeedListKey = "InMemoryDefaultKey";
        SeedListFileInfoPtr m_inMemoryDefaultSeedList;

        SeedTabWidget* m_seedTabWidget = nullptr;
    };

} // namespace AssetBundler
