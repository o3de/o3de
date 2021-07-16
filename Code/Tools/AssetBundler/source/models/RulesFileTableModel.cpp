/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/models/RulesFileTableModel.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <QFileInfo>
#include <QFont>

namespace AssetBundler
{
    RulesFileInfo::RulesFileInfo(const AZStd::string& absolutePath, const QString& fileName, bool loadFromFile)
        : m_absolutePath(absolutePath)
        , m_fileName(fileName)
    {
        AzFramework::StringFunc::Path::Normalize(m_absolutePath);
        m_comparisonSteps = AZStd::make_shared<AzToolsFramework::AssetFileInfoListComparison>();

        if (loadFromFile)
        {
            auto outcome = AzToolsFramework::AssetFileInfoListComparison::Load(m_absolutePath);
            if (!outcome.IsSuccess())
            {
                AZ_Error("AssetBundler", false, outcome.GetError().c_str());
                return;
            }
            m_comparisonSteps = AZStd::make_unique<AzToolsFramework::AssetFileInfoListComparison>(outcome.TakeValue());

            QFileInfo fileInfo(m_absolutePath.c_str());
            m_fileModificationTime = fileInfo.fileTime(QFileDevice::FileModificationTime);
        }
        else
        {
            m_fileModificationTime = QDateTime::currentDateTime();
        }
    }

    bool RulesFileInfo::SaveRulesFile()
    {
        if (!m_comparisonSteps->Save(m_absolutePath))
        {
            return false;
        }

        m_hasUnsavedChanges = false;
        m_fileModificationTime = QDateTime::currentDateTime();
        return true;
    }



    RulesFileTableModel::RulesFileTableModel()
        : AssetBundlerAbstractFileTableModel()
    {
    }

    AZStd::vector<AZStd::string> RulesFileTableModel::CreateNewFiles(
        const AZStd::string& absoluteFilePath,
        const AzFramework::PlatformFlags& /*platforms*/,
        const QString& /*project*/)
    {
        if (absoluteFilePath.empty())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Input file path is empty");
            return {};
        }

        // Create a platform-specific file path
        if (AZ::IO::FileIOBase::GetInstance()->Exists(absoluteFilePath.c_str()))
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Cannot Create New File: (%s) already exists", absoluteFilePath.c_str());
            return {};
        }

        // Get the file name without the extension for display purposes
        AZStd::string fileName = absoluteFilePath;
        AzToolsFramework::RemovePlatformIdentifier(fileName);
        AzFramework::StringFunc::Path::GetFileName(fileName.c_str(), fileName);

        // Create a Rules File and save it to disk
        AZStd::string key = AssetBundler::GenerateKeyFromAbsolutePath(absoluteFilePath);
        RulesFileInfoPtr newRulesFile = AZStd::make_shared<RulesFileInfo>(absoluteFilePath, QString(fileName.c_str()), false);

        if (!newRulesFile->SaveRulesFile())
        {
            return {};
        }

        // Add the new file to the model
        m_rulesFileInfoMap[key] = newRulesFile;
        AddFileKey(key);
        return { absoluteFilePath };
    }

    bool RulesFileTableModel::DeleteFile(const QModelIndex& index)
    {
        AZStd::string key = GetFileKey(index);
        if (key.empty())
        {
            // Error has already been thrown
            return false;
        }

        RulesFileInfoPtr rulesFileInfo = m_rulesFileInfoMap[key];

        // Remove file from disk
        if (AZ::IO::FileIOBase::GetInstance()->IsReadOnly(rulesFileInfo->m_absolutePath.c_str()))
        {
            AZ_Error(AssetBundler::AppWindowName, false, ReadOnlyFileErrorMessage, rulesFileInfo->m_absolutePath.c_str());
            return false;
        }

        auto deleteResult = AZ::IO::FileIOBase::GetInstance()->Remove(rulesFileInfo->m_absolutePath.c_str());
        if (!deleteResult)
        {
            AZ_Error(AssetBundler::AppWindowName, false,
                "Unable to delete (%s). Result code: %u", rulesFileInfo->m_absolutePath.c_str(),
                deleteResult.GetResultCode());
            return false;
        }

        // Remove file from model
        m_rulesFileInfoMap.erase(key);
        RemoveFileKey(index);

        return true;
    }

    void RulesFileTableModel::LoadFile(
        const AZStd::string& absoluteFilePath,
        const AZStd::string& /*projectName*/,
        bool /*isDefaultFile*/)
    {
        // Get the file name without the extension for display purposes
        AZStd::string fileName(absoluteFilePath);
        AzFramework::StringFunc::Path::GetFileName(fileName.c_str(), fileName);

        // Read the Rules file into memory and store it 
        AZStd::string key = AssetBundler::GenerateKeyFromAbsolutePath(absoluteFilePath);
        auto fileInfoIt = m_rulesFileInfoMap.find(key);
        if (fileInfoIt != m_rulesFileInfoMap.end() && fileInfoIt->second->m_hasUnsavedChanges)
        {
            AZ_Warning(AssetBundler::AppWindowName, false,
                "Rules File %s has unsaved changes and couldn't be reloaded", absoluteFilePath.c_str());
            return;
        }

        m_rulesFileInfoMap[key] = AZStd::make_shared<RulesFileInfo>(absoluteFilePath, QString(fileName.c_str()), true);
        AddFileKey(key);
    }

    bool RulesFileTableModel::WriteToDisk(const AZStd::string& key)
    {
        auto rulesFileIt = m_rulesFileInfoMap.find(key);
        return rulesFileIt != m_rulesFileInfoMap.end() && rulesFileIt->second->SaveRulesFile();
    }

    AZStd::string RulesFileTableModel::GetFileAbsolutePath(const QModelIndex& index) const
    {
        RulesFileInfoPtr rulesFileInfo = GetRulesFileInfoPtr(index);
        if (!rulesFileInfo)
        {
            return AZStd::string();
        }

        return rulesFileInfo->m_absolutePath;
    }

    int RulesFileTableModel::GetFileNameColumnIndex() const
    {
        return Column::ColumnFileName;
    }

    int RulesFileTableModel::GetTimeStampColumnIndex() const
    {
        return Column::ColumnFileModificationTime;
    }

    AZStd::shared_ptr<AzToolsFramework::AssetFileInfoListComparison> RulesFileTableModel::GetComparisonSteps(const QModelIndex& index) const
    {
        RulesFileInfoPtr rulesFileInfo = GetRulesFileInfoPtr(index);
        if (!rulesFileInfo)
        {
            return nullptr;
        }

        return rulesFileInfo->m_comparisonSteps;
    }

    bool RulesFileTableModel::MarkFileChanged(const QModelIndex& index)
    {
        AZStd::string key = GetFileKey(index);
        if (key.empty())
        {
            // Error has already been thrown
            return false;
        }

        m_rulesFileInfoMap[key]->m_hasUnsavedChanges = true;

        // Update display so the user knows there are unsaved changes
        m_keysWithUnsavedChanges.emplace(key);
        QModelIndex changedIndex = QAbstractTableModel::index(index.row(), Column::ColumnFileName);
        emit dataChanged(changedIndex, changedIndex, { Qt::DisplayRole, Qt::FontRole });

        return true;
    }

    int RulesFileTableModel::columnCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : Column::Max;
    }

    QVariant RulesFileTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case Column::ColumnFileName:
                return QString(tr("Name"));
            case Column::ColumnFileModificationTime:
                return QString(tr("Modification Time"));
            default:
                break;
            }
        }

        return QVariant();
    }

    QVariant RulesFileTableModel::data(const QModelIndex& index, int role) const
    {
        AZStd::string key = GetFileKey(index);
        if (key.empty())
        {
            return QVariant();
        }

        RulesFileInfoPtr rulesFileInfo = m_rulesFileInfoMap.at(key);

        int col = index.column();
        bool hasUnsavedChanges = rulesFileInfo->m_hasUnsavedChanges;

        switch (role)
        {
        case Qt::DisplayRole:
            [[fallthrough]];
        case SortRole:
        {
            if (col == Column::ColumnFileName)
            {
                QString displayName = rulesFileInfo->m_fileName;
                if (hasUnsavedChanges)
                {
                    displayName.append("*");
                }
                return displayName;
            }
            else if (col == Column::ColumnFileModificationTime)
            {
                if (role == SortRole)
                {
                    return rulesFileInfo->m_fileModificationTime;
                }
                else
                {
                    return rulesFileInfo->m_fileModificationTime.toString(DateTimeFormat);
                }
            }
            break;
        }
        case Qt::FontRole:
        {
            if (col == Column::ColumnFileName && hasUnsavedChanges)
            {
                QFont boldFont;
                boldFont.setBold(true);
                return boldFont;
            }
            break;
        }
        default:
            break;
        }

        return QVariant();
    }

    RulesFileInfoPtr RulesFileTableModel::GetRulesFileInfoPtr(const QModelIndex& index) const
    {
        AZStd::string key = GetFileKey(index);
        if (key.empty())
        {
            return nullptr;
        }

        return m_rulesFileInfoMap.at(key);
    }

} // namespace AssetBundler
