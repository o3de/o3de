/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/models/AssetListFileTableModel.h>
#include <source/models/AssetListTableModel.h>

#include <source/utils/utils.h>

#include <AzCore/Outcome/Outcome.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Asset/AssetBundler.h>

#include <QFileInfo>
#include <QFont>

namespace AssetBundler
{

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // AssetListFileInfo
    //////////////////////////////////////////////////////////////////////////////////////////////////
    AssetListFileInfo::AssetListFileInfo(const AZStd::string& absolutePath, const QString& fileName, const AZStd::string& platform)
        : m_absolutePath(absolutePath)
        , m_fileName(fileName)
        , m_platform(QString(platform.c_str()))
    {
        AzFramework::StringFunc::Path::Normalize(m_absolutePath);

        // Modification time will either give us the time the file was last overwritten
        // (or the time it was created if it has never been overwritten)
        QFileInfo fileInfo(m_absolutePath.c_str());
        m_fileCreationTime = fileInfo.fileTime(QFileDevice::FileModificationTime);

        // Load the contents of the asset list file into memory
        m_assetListModel.reset(new AssetListTableModel(nullptr, absolutePath, platform));
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // AssetListFileTableModel
    //////////////////////////////////////////////////////////////////////////////////////////////////
    AssetListFileTableModel::AssetListFileTableModel()
        : AssetBundlerAbstractFileTableModel()
    {
    }

    QSharedPointer<AssetListTableModel> AssetListFileTableModel::GetAssetListFileContents(const QModelIndex& index)
    {
        auto assetListFileInfoOutcome = GetAssetFileInfo(index);
        if (!assetListFileInfoOutcome.IsSuccess())
        {
            return QSharedPointer<AssetListTableModel>();
        }

        return assetListFileInfoOutcome.GetValue()->m_assetListModel;
    }

    bool AssetListFileTableModel::DeleteFile(const QModelIndex& index)
    {
        auto assetFileInfoOutcome = GetAssetFileInfo(index);
        if (!assetFileInfoOutcome.IsSuccess())
        {
            return false;
        }

        AZStd::string key = AssetBundler::GenerateKeyFromAbsolutePath(assetFileInfoOutcome.GetValue()->m_absolutePath);
        if (m_assetListFileInfoMap.find(key) == m_assetListFileInfoMap.end())
        {
            return false;
        }

        AssetListFileInfoPtr assetFileInfo = m_assetListFileInfoMap[key];

        // Remove file from disk
        const char* absolutePath = assetFileInfo->m_absolutePath.c_str();
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

        // Remove file from model
        m_assetListFileInfoMap.erase(key);
        RemoveFileKey(index);

        return true;
    }

    void AssetListFileTableModel::LoadFile(
        const AZStd::string& absoluteFilePath,
        const AZStd::string& /*projectName*/,
        bool /*isDefaultFile*/)
    {
        AZStd::string fullFileName;
        AzFramework::StringFunc::Path::GetFullFileName(absoluteFilePath.c_str(), fullFileName);

        // Get the file name without the platform for display purposes
        AZStd::string baseFileName;
        AZStd::string platformIdentifier;
        AzToolsFramework::SplitFilename(fullFileName, baseFileName, platformIdentifier);

        // Read the AssetListFile into memory and store it
        AZStd::string key = AssetBundler::GenerateKeyFromAbsolutePath(absoluteFilePath);
        m_assetListFileInfoMap[key].reset(new AssetListFileInfo(absoluteFilePath, QString(baseFileName.c_str()), platformIdentifier));
        AddFileKey(key);
    }

    bool AssetListFileTableModel::WriteToDisk(const AZStd::string& /*key*/)
    {
        return true;
    }

    AZStd::string AssetListFileTableModel::GetFileAbsolutePath(const QModelIndex& index) const
    {
        auto assetFileInfoOutcome = GetAssetFileInfo(index);
        if (!assetFileInfoOutcome.IsSuccess())
        {
            // Error has already been thrown
            return AZStd::string();
        }
        return assetFileInfoOutcome.GetValue()->m_absolutePath;
    }

    int AssetListFileTableModel::GetFileNameColumnIndex() const
    {
        return Column::ColumnFileName;
    }

    int AssetListFileTableModel::GetTimeStampColumnIndex() const
    {
        return Column::ColumnFileCreationTime;
    }

    int AssetListFileTableModel::columnCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : Column::Max;
    }

    QVariant AssetListFileTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case Column::ColumnFileName:
                return QString(tr("Asset List File"));
            case Column::ColumnPlatform:
                return QString(tr("Platform"));
            case Column::ColumnFileCreationTime:
                return QString(tr("Creation Time"));
            default:
                break;
            }
        }

        return QVariant();
    }

    QVariant AssetListFileTableModel::data(const QModelIndex& index, int role) const
    {
        auto assetListFileInfoOutcome = GetAssetFileInfo(index);
        if (!assetListFileInfoOutcome.IsSuccess())
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
                return assetListFileInfoOutcome.GetValue()->m_fileName;
            }
            else if (col == Column::ColumnPlatform)
            {
                return assetListFileInfoOutcome.GetValue()->m_platform;
            }
            else if (col == Column::ColumnFileCreationTime)
            {
                if (role == SortRole)
                {
                    return assetListFileInfoOutcome.GetValue()->m_fileCreationTime;
                }
                else
                {
                    return assetListFileInfoOutcome.GetValue()->m_fileCreationTime.toString(DateTimeFormat);
                }
            }
            else
            {
                // Returning an empty QString will ensure the checkboxes do not have any text displayed next to them
                return QString();
            }
        }
        case Qt::FontRole:
        {
            if (col == Column::ColumnFileName)
            {
                QFont boldFont;
                boldFont.setBold(true);
                return boldFont;
            }
        }
        default:
            break;
        }

        return QVariant();
    }

    AZ::Outcome<AssetListFileInfoPtr, void> AssetListFileTableModel::GetAssetFileInfo(const QModelIndex& index) const
    {
        AZStd::string key = GetFileKey(index);
        if (key.empty())
        {
            // Error has already been thrown
            return AZ::Failure();
        }

        if (m_assetListFileInfoMap.find(key) != m_assetListFileInfoMap.end())
        {
            return AZ::Success(m_assetListFileInfoMap.at(key));
        }
        else
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Cannot find Asset List File Info");
            return AZ::Failure();
        }
    }

} // namespace AssetBundler
