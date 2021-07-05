/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <source/models/AssetBundlerAbstractFileTableModel.h>

#include <QDateTime>
#include <QSharedPointer>

namespace AssetBundler
{
    class AssetListTableModel;

    struct AssetListFileInfo
    {
        /**
        * Stores information about an Asset List File on disk.
        *
        * @param absolutePath The absolute path of the Asset List File
        * @param fileName The name of the Asset List File. This does not include the platform ID
        * @param platform The platform for the Asset List File
        */
        AssetListFileInfo(const AZStd::string& absolutePath, const QString& fileName, const AZStd::string& platform);

        AZStd::string m_absolutePath;
        QDateTime m_fileCreationTime;
        /// Use QString for display purpose. This can help to avoid losts of string conversion
        QString m_fileName;
        QString m_platform;

        QSharedPointer<AssetListTableModel> m_assetListModel;
    };

    using AssetListFileInfoPtr = AZStd::shared_ptr<AssetListFileInfo>;
    /// Stores AssetListFileInfo, using the absolute path (without the drive letter) of the Asset List file as the key
    using AssetListFileInfoMap = AZStd::unordered_map<AZStd::string, AssetListFileInfoPtr>;

    class AssetListFileTableModel
        : public AssetBundlerAbstractFileTableModel
    {
    public:
        explicit AssetListFileTableModel();
        virtual ~AssetListFileTableModel() {}

        QSharedPointer<AssetListTableModel> GetAssetListFileContents(const QModelIndex& index);

        //////////////////////////////////////////////////////////////////////////
        // AssetBundlerAbstractFileTableModel overrides
        AZStd::vector<AZStd::string> CreateNewFiles(
            const AZStd::string& /*absoluteFilePath*/,
            const AzFramework::PlatformFlags& /*platforms*/,
            const QString& /*project*/) override { return {}; }
        bool DeleteFile(const QModelIndex& index) override;
        void LoadFile(const AZStd::string& absoluteFilePath, const AZStd::string& projectName = "", bool isDefaultFile = false) override;
        bool WriteToDisk(const AZStd::string& key) override;
        AZStd::string GetFileAbsolutePath(const QModelIndex& index) const override;
        int GetFileNameColumnIndex() const override;
        int GetTimeStampColumnIndex() const override;

        //////////////////////////////////////////////////////////////////////////
        // QAbstractListModel overrides
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        //////////////////////////////////////////////////////////////////////////

        enum Column
        {
            ColumnFileName,
            ColumnPlatform,
            ColumnFileCreationTime,
            Max
        };

    private:
        AZ::Outcome<AssetListFileInfoPtr, void> GetAssetFileInfo(const QModelIndex& index) const;

        AssetListFileInfoMap m_assetListFileInfoMap;
    };

} // namespace AssetBundler
