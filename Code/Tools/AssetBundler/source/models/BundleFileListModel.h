/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <source/models/AssetBundlerAbstractFileTableModel.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/Asset/AssetBundler.h>

#include <QDateTime>

namespace AssetBundler
{

    struct BundleFileInfo
    {
        AZStd::string m_absolutePath;
        QString m_fileName;
        QDateTime m_fileCreationTime;
        AZ::u64 m_compressedSize;
        QStringList m_relatedBundles;

        BundleFileInfo(const AZStd::string& absolutePath);
    };
    
    using BundleFileInfoPtr = AZStd::shared_ptr<BundleFileInfo>;
    using BundleFileInfoMap = AZStd::unordered_map<AZStd::string, BundleFileInfoPtr>;

    class BundleFileListModel
        : public AssetBundlerAbstractFileTableModel
    {
    public:
        explicit BundleFileListModel();
        virtual ~BundleFileListModel() {}

        AZStd::vector<AZStd::string> CreateNewFiles(
            const AZStd::string& /*absoluteFilePath*/,
            const AzFramework::PlatformFlags& /*platforms*/,
            const QString& /*project*/) override { return {}; }
        bool DeleteFile(const QModelIndex& index) override;
        void LoadFile(const AZStd::string& absoluteFilePath, const AZStd::string& projectName = "", bool isDefaultFile = false) override;
        bool WriteToDisk(const AZStd::string& /*key*/) override { return true; }

        AZStd::string GetFileAbsolutePath(const QModelIndex& index) const override;
        int GetFileNameColumnIndex() const override;
        int GetTimeStampColumnIndex() const override;

        AZ::Outcome<BundleFileInfoPtr, void> GetBundleInfo(const QModelIndex& index) const;

        //////////////////////////////////////////////////////////////////////////
        // QAbstractTableModel overrides
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        //////////////////////////////////////////////////////////////////////////

        enum Column
        {
            ColumnFileName,
            ColumnFileCreationTime,
            Max
        };

    private:
        BundleFileInfoMap m_bundleFileInfoMap;
    };
}
