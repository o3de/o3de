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

#include <source/models/AssetBundlerAbstractFileTableModel.h>
#include <source/Utils/utils.h>

#include <AzToolsFramework/Asset/AssetBundler.h>

#include <QDateTime>

namespace AssetBundler
{

    //! Stores information about a Rules file on disk. 
    struct RulesFileInfo
    {
        RulesFileInfo(const AZStd::string& absolutePath, const QString& fileName, bool loadFromFile);

        bool SaveRulesFile();

        AZStd::string m_absolutePath;
        QString m_fileName;
        QDateTime m_fileModificationTime;
        bool m_hasUnsavedChanges = false;

        AZStd::shared_ptr<AzToolsFramework::AssetFileInfoListComparison> m_comparisonSteps;
    };

    using RulesFileInfoPtr = AZStd::shared_ptr<RulesFileInfo>;
    using RulesFileInfoMap = AZStd::unordered_map<AZStd::string, RulesFileInfoPtr>;


    class RulesFileTableModel
        : public AssetBundlerAbstractFileTableModel
    {
    public:
        RulesFileTableModel();
        virtual ~RulesFileTableModel() {}

        AZStd::vector<AZStd::string> CreateNewFiles(const AZStd::string& absoluteFilePath, const AzFramework::PlatformFlags& platforms = AzFramework::PlatformFlags::Platform_NONE, const QString& project = QString()) override;

        bool DeleteFile(const QModelIndex& index) override;

        void LoadFile(const AZStd::string& absoluteFilePath, const AZStd::string& projectName = "", bool isDefaultFile = false) override;

        bool WriteToDisk(const AZStd::string& key) override;

        AZStd::string GetFileAbsolutePath(const QModelIndex& index) const override;

        int GetFileNameColumnIndex() const override;

        int GetTimeStampColumnIndex() const override;

        AZStd::shared_ptr<AzToolsFramework::AssetFileInfoListComparison> GetComparisonSteps(const QModelIndex& index) const;

        bool MarkFileChanged(const QModelIndex& index);

        //////////////////////////////////////////////////////////////////////////
        // QAbstractTableModel overrides
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        //////////////////////////////////////////////////////////////////////////

        enum Column
        {
            ColumnFileName,
            ColumnFileModificationTime,
            Max
        };

    private:
        RulesFileInfoMap m_rulesFileInfoMap;

        RulesFileInfoPtr GetRulesFileInfoPtr(const QModelIndex& index) const;
    };
} // namespace AssetBundler
