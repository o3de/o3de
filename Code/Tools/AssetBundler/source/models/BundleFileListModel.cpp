/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/models/BundleFileListModel.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/AssetBundle/AssetBundleComponent.h>

#include <source/utils/utils.h>

#include <QFileInfo>


namespace AssetBundler
{
    BundleFileInfo::BundleFileInfo(const AZStd::string& absolutePath)
        : m_absolutePath(absolutePath)
        , m_compressedSize(0u)
    {
        AzFramework::StringFunc::Path::Normalize(m_absolutePath);

        AZStd::string fileNameStr;
        if (AzFramework::StringFunc::Path::GetFileName(m_absolutePath.c_str(), fileNameStr))
        {
            m_fileName = QString(fileNameStr.c_str());
        }
        else
        {
            AZ_Error("AssetBundler", false, "Failed to get file name from %s", m_absolutePath.c_str());
        }

        // Modification time will either give us the time the file was last overwritten
        // (or the time it was created if it has never been overwritten)
        QFileInfo fileInfo(m_absolutePath.c_str());
        m_fileCreationTime = fileInfo.fileTime(QFileDevice::FileModificationTime);
    }

    BundleFileListModel::BundleFileListModel()
        : AssetBundlerAbstractFileTableModel()
    {
    }

    bool BundleFileListModel::DeleteFile(const QModelIndex& index)
    {
        AZStd::string fileKey = GetFileKey(index);
        if (fileKey.empty())
        {
            // Error has already been thrown
            return false;
        }

        auto bundleFileInfoIt = m_bundleFileInfoMap.find(fileKey);
        if (bundleFileInfoIt == m_bundleFileInfoMap.end())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Unable to find Bundle Info with key ( %s )", fileKey.c_str());
            return false;
        }

        BundleFileInfoPtr bundleFileInfo = bundleFileInfoIt->second;

        // Remove file from disk
        const char* absolutePath = bundleFileInfo->m_absolutePath.c_str();
        if (AZ::IO::FileIOBase::GetInstance()->Exists(absolutePath))
        {
            if (AZ::IO::FileIOBase::GetInstance()->IsReadOnly(absolutePath))
            {
                AZ_Error(AssetBundler::AppWindowName, false, ReadOnlyFileErrorMessage, absolutePath);
                return false;
            }

            auto deleteResult = AZ::IO::FileIOBase::GetInstance()->Remove(absolutePath);
            if (!deleteResult)
            {
                AZ_Error(AssetBundler::AppWindowName, false,
                    "Unable to delete (%s). Result code: %u", absolutePath, deleteResult.GetResultCode());
                return false;
            }
        }

        // Remove file from Model
        m_bundleFileInfoMap.erase(fileKey);
        RemoveFileKey(index);

        return true;
    }

    int BundleFileListModel::columnCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : Column::Max;
    }

    QVariant BundleFileListModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case Column::ColumnFileName:
                return QString(tr("Name"));
            case Column::ColumnFileCreationTime:
                return QString(tr("Creation Time"));
            default:
                break;
            }
        }
        return QVariant();
    }

    QVariant BundleFileListModel::data(const QModelIndex& index, int role) const
    {
        auto bundleInfoOutcome = GetBundleInfo(index);
        if (!bundleInfoOutcome.IsSuccess())
        {
            return QVariant();
        }

        int col = index.column();

        switch (role)
        {
        case Qt::DisplayRole:
            [[fallthrough]];
        case SortRole:
        {
            if (col == Column::ColumnFileName)
            {
                return bundleInfoOutcome.GetValue()->m_fileName;
            }
            else if (col == Column::ColumnFileCreationTime)
            {
                if (role == SortRole)
                {
                    return bundleInfoOutcome.GetValue()->m_fileCreationTime;
                }
                else
                {
                    return bundleInfoOutcome.GetValue()->m_fileCreationTime.toString(DateTimeFormat);
                }
            }
        }
        default:
            break;
        }

        return QVariant();
    }

    AZStd::string BundleFileListModel::GetFileAbsolutePath(const QModelIndex& index) const
    {
        AZStd::string key = GetFileKey(index);
        if (key.empty())
        {
            // Error has already been thrown
            return key;
        }

        return m_bundleFileInfoMap.at(key)->m_absolutePath;
    }

    int BundleFileListModel::GetFileNameColumnIndex() const
    {
        return Column::ColumnFileName;
    }

    int BundleFileListModel::GetTimeStampColumnIndex() const
    {
        return Column::ColumnFileCreationTime;
    }

    void BundleFileListModel::LoadFile(
        const AZStd::string& absoluteFilePath,
        const AZStd::string& /*projectName*/,
        bool /*isDefaultFile*/)
    {
        AZStd::string key = AssetBundler::GenerateKeyFromAbsolutePath(absoluteFilePath);

        BundleFileInfo* newInfo = new BundleFileInfo(absoluteFilePath);
        
        QFile bundleFile(absoluteFilePath.c_str());
        if (bundleFile.open(QIODevice::ReadOnly))
        {
            newInfo->m_compressedSize = bundleFile.size();
            bundleFile.close();
        }
        else
        {
            AZ_Error("AssetBundler", false, "Failed to open file at %s", absoluteFilePath.c_str());
        }

        auto manifest = AzToolsFramework::AssetBundleComponent::GetManifestFromBundle(absoluteFilePath);
        if (manifest != nullptr)
        {
            for (auto& bundleName : manifest->GetDependentBundleNames())
            {
                newInfo->m_relatedBundles.push_back(bundleName.c_str());
            }
        }
        else
        {
            AZ_Error("AssetBundler", false, "Failed to get manifest from bundle at %s", absoluteFilePath.c_str());
        }

        m_bundleFileInfoMap[key].reset(newInfo);
        // Add it to the list that gets displayed by AssetBundlerAbstractFileTableModel. Make sure that
        // AddFileKey is called after m_bundleFileInfoMap is reset since the filterAcceptsRow funtion
        // of the filter model will be called when a new row is inserted and it could cause a crash
        // without valid data being set
        AddFileKey(key);
    }

    AZ::Outcome<BundleFileInfoPtr, void> BundleFileListModel::GetBundleInfo(const QModelIndex& index) const
    {
        AZStd::string key = GetFileKey(index);
        if (key.empty())
        {
            // Error has already been thrown
            return AZ::Failure();
        }

        if (m_bundleFileInfoMap.find(key) != m_bundleFileInfoMap.end())
        {
            return AZ::Success(m_bundleFileInfoMap.at(key));
        }
        else
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Cannot find Bundle File Info");
            return AZ::Failure();
        }
    }

}
