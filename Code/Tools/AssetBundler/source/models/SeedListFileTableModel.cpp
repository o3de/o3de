/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/models/SeedListFileTableModel.h>
#include <source/models/SeedListTableModel.h>

#include <source/ui/SeedTabWidget.h>
#include <source/utils/utils.h>

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalog.h>

#include <QFileInfo>
#include <QFont>

namespace AssetBundler
{

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // SeedListFileInfo
    //////////////////////////////////////////////////////////////////////////////////////////////////
    SeedListFileInfo::SeedListFileInfo(
        const AZStd::string& absolutePath,
        const QString& fileName,
        const QString& project,
        bool loadFromFile,
        bool isDefaultSeedList,
        const AZStd::vector<AZStd::string>& defaultSeeds,
        const AzFramework::PlatformFlags& platforms)
        : m_absolutePath(absolutePath)
        , m_fileName(fileName)
        , m_project(project)
        , m_isDefaultSeedList(isDefaultSeedList)
    {
        AzFramework::StringFunc::Path::Normalize(m_absolutePath);

        // Load the contents of the seed file into memory
        if (loadFromFile)
        {
            m_seedListModel.reset(new SeedListTableModel(nullptr, absolutePath));
            QFileInfo fileInfo(m_absolutePath.c_str());
            m_fileModificationTime = fileInfo.fileTime(QFileDevice::FileModificationTime);
        }
        else
        {
            m_seedListModel.reset(new SeedListTableModel(nullptr, "", defaultSeeds, platforms));
            m_fileModificationTime = QDateTime::currentDateTime();
        }
    }

    bool SeedListFileInfo::SaveSeedFile()
    {
        if (m_seedListModel->Save(m_absolutePath))
        {
            m_fileModificationTime = QDateTime::currentDateTime();
            return true;
        }
        return false;
    }

    bool SeedListFileInfo::HasUnsavedChanges()
    {
        return m_seedListModel->HasUnsavedChanges();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // SeedListFileTableModel
    //////////////////////////////////////////////////////////////////////////////////////////////////

    SeedListFileTableModel::SeedListFileTableModel(SeedTabWidget* parentSeedTabWidget)
        : AssetBundlerAbstractFileTableModel()
        , m_seedTabWidget(parentSeedTabWidget)
    {
        m_inMemoryDefaultSeedList.reset(new SeedListFileInfo("", "", "", false));
    }

    SeedListFileTableModel::~SeedListFileTableModel()
    {
        m_seedTabWidget = nullptr;
    }

    void SeedListFileTableModel::AddDefaultSeedsToInMemoryList(
        const AZStd::vector<AZStd::string>& defaultSeeds,
        const char* projectName,
        const AzFramework::PlatformFlags& platforms)
    {
        m_inMemoryDefaultSeedList.reset(
            new SeedListFileInfo(m_inMemoryDefaultSeedListKey, tr("DefaultSeeds"), QString(projectName), false, true, defaultSeeds, platforms));
    }

    AZStd::vector<AZStd::string> SeedListFileTableModel::CreateNewFiles(
        const AZStd::string& absoluteFilePath,
        const AzFramework::PlatformFlags& /*platforms*/,
        const QString& project)
    {
        if (absoluteFilePath.empty())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Input file path is empty.");
            return {};
        }

        // Get the file name without the extension for display purposes
        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFileName(absoluteFilePath.c_str(), fileName);

        // Create a Seed List File and save it to disk
        AZStd::string key = AssetBundler::GenerateKeyFromAbsolutePath(absoluteFilePath);

        AZStd::shared_ptr<SeedListFileInfo> newSeedListFile =
            AZStd::make_shared<SeedListFileInfo>(absoluteFilePath, QString(fileName.c_str()), project, false);
        newSeedListFile->m_seedListModel->SetHasUnsavedChanges(true);
        bool saveResult = newSeedListFile->SaveSeedFile();
        if (!saveResult)
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Unable to create Seed List File: %s", absoluteFilePath.c_str());
            return {};
        }

        // Add new file to the model
        m_seedListFileInfoMap[key] = newSeedListFile;
        AddFileKey(key);

        AZStd::vector<AZStd::string> createdFiles = { absoluteFilePath };
        return createdFiles;
    }

    bool SeedListFileTableModel::DeleteFile(const QModelIndex& index)
    {
        AZStd::string key = GetFileKey(index);
        if (key.empty())
        {
            // Error has already been thrown
            return false;
        }

        if (m_seedListFileInfoMap.find(key) == m_seedListFileInfoMap.end())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Unable to find Seed File Info with key ( %s )", key.c_str());
            return false;
        }

        SeedListFileInfoPtr seedFileInfo = m_seedListFileInfoMap[key];

        // Remove file from disk
        const char* absolutePath = seedFileInfo->m_absolutePath.c_str();
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
        m_seedListFileInfoMap.erase(key);
        RemoveFileKey(index);

        return true;
    }

    void SeedListFileTableModel::Reload(
        const char* fileExtension,
        const QSet<QString>& watchedFolders,
        const QSet<QString>& watchedFiles,
        const AZStd::unordered_map<AZStd::string, AZStd::string>& pathToProjectNameMap)
    {
        // Load in the Seed List files from disk
        AssetBundlerAbstractFileTableModel::Reload(fileExtension, watchedFolders, watchedFiles, pathToProjectNameMap);

        // Add the in-memory Default Seed List to the model
        m_seedListFileInfoMap[m_inMemoryDefaultSeedListKey] = m_inMemoryDefaultSeedList;
        AddFileKey(m_inMemoryDefaultSeedListKey);
    }

    void SeedListFileTableModel::LoadFile(const AZStd::string& absoluteFilePath, const AZStd::string& projectName, bool isDefaultFile)
    {
        // Get the file name without the extension for display purposes
        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFileName(absoluteFilePath.c_str(), fileName);

        // Read the SeedListFile into memory and store it
        AZStd::string key = AssetBundler::GenerateKeyFromAbsolutePath(absoluteFilePath);
        auto fileInfoIt = m_seedListFileInfoMap.find(key);
        if (fileInfoIt != m_seedListFileInfoMap.end() && fileInfoIt->second->HasUnsavedChanges())
        {
            AZ_Warning(AssetBundler::AppWindowName, false,
                "Seed List File %s has unsaved changes and couldn't be reloaded", absoluteFilePath.c_str());
            return;
        }

        AZStd::string projectNameOnDisplay = projectName;
        if (projectName.empty())
        {
            auto outcome = AssetBundler::GetCurrentProjectName();
            if (!outcome.IsSuccess())
            {
                AZ_Error(AssetBundler::AppWindowName, false, outcome.TakeError().c_str());
                return;
            }

            projectNameOnDisplay = outcome.TakeValue();
        }

        m_seedListFileInfoMap[key].reset(
            new SeedListFileInfo(absoluteFilePath, QString(fileName.c_str()), QString(projectNameOnDisplay.c_str()), true, isDefaultFile));
        AddFileKey(key);
    }

    void SeedListFileTableModel::SelectDefaultSeedLists(bool setSelected)
    {
        for (const AZStd::string& key : GetAllFileKeys())
        {
            auto seedFileInfo = m_seedListFileInfoMap[key];
            if (!seedFileInfo->m_isDefaultSeedList)
            {
                continue;
            }

            seedFileInfo->m_isChecked = setSelected;
            if(setSelected)
            {
                m_checkedSeedListFiles.insert(key);
            }
            else
            {
                m_checkedSeedListFiles.erase(key);
            }
        }

        m_seedTabWidget->SetGenerateAssetListsButtonEnabled(!m_checkedSeedListFiles.empty());

        // Update the Check State display of all elements
        QModelIndex firstIndex = index(0, 0);
        QModelIndex lastIndex = index(rowCount() - 1, columnCount() - 1);
        emit dataChanged(firstIndex, lastIndex, { Qt::CheckStateRole });
    }

    AZStd::vector<AZStd::string> SeedListFileTableModel::GenerateAssetLists(
        const AZStd::string& absoluteFilePath,
        const AzFramework::PlatformFlags& platforms)
    {
        if (!m_checkedSeedListFiles.size())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Cannot Generate Asset List File(s): No Seed List Files are selected");
            return {};
        }

        if (absoluteFilePath.empty())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "File path cannot be empty");
            return {};
        }
        
        if(platforms == AzFramework::PlatformFlags::Platform_NONE)
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Input platform cannot be empty");
            return {};
        }

        // Get all of the Seeds into one AssetSeedManager
        AzToolsFramework::AssetSeedManager assetSeedManager;

        if (m_checkedSeedListFiles.find(m_inMemoryDefaultSeedListKey) != m_checkedSeedListFiles.end())
        {
            // The In-Memory Default Seed List can't be loaded from a file on disk, it is a special case
            assetSeedManager = *m_inMemoryDefaultSeedList->m_seedListModel->GetSeedListManager().get();
            m_checkedSeedListFiles.erase(m_inMemoryDefaultSeedListKey);
        }

        for (const auto& checkedSeedFileKey : m_checkedSeedListFiles)
        {
            assetSeedManager.Load(m_seedListFileInfoMap[checkedSeedFileKey]->m_absolutePath);
        }

        // Generate an AssetList for every input platform
        AZStd::vector<AZStd::string> createdFiles;
        for (const auto& platformIndex : AzFramework::PlatformHelper::GetPlatformIndicesInterpreted(platforms))
        {
            AZStd::string platformSpecificCachePath =
                AzToolsFramework::PlatformAddressedAssetCatalog::GetCatalogRegistryPathForPlatform(platformIndex);
            AzFramework::StringFunc::Path::StripFullName(platformSpecificCachePath);

            FilePath platformSpecificPath(absoluteFilePath, AZStd::string(AzFramework::PlatformHelper::GetPlatformName(platformIndex)));
            if (assetSeedManager.SaveAssetFileInfo(platformSpecificPath.AbsolutePath(), AzFramework::PlatformHelper::GetPlatformFlagFromPlatformIndex(platformIndex)))
            {
                createdFiles.emplace_back(platformSpecificPath.AbsolutePath());
            }
        }

        return createdFiles;
    }

    QSharedPointer<SeedListTableModel> SeedListFileTableModel::GetSeedListFileContents(const QModelIndex& index)
    {
        auto seedFileInfoOutcome = GetSeedFileInfo(index);
        if (!seedFileInfoOutcome.IsSuccess())
        {
            return QSharedPointer<SeedListTableModel>();
        }

        return seedFileInfoOutcome.GetValue()->m_seedListModel;
    }

    bool SeedListFileTableModel::SetSeedPlatforms(
        const QModelIndex& seedFileIndex,
        const QModelIndex& seedIndex,
        const AzFramework::PlatformFlags& platforms)
    {
        AZStd::string key = GetFileKey(seedFileIndex);
        if (key.empty())
        {
            // Error has already been thrown
            return false;
        }

        if (m_seedListFileInfoMap.find(key) == m_seedListFileInfoMap.end())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Unable to find Seed File Info with key ( %s )", key.c_str());
            return false;
        }

        SeedListFileInfoPtr seedFileInfo = m_seedListFileInfoMap[key];
        if (!seedFileInfo->m_seedListModel->SetSeedPlatforms(seedIndex, platforms))
        {
            // Error has already been thrown
            return false;
        }

        // Update display so the user knows there are unsaved changes
        m_keysWithUnsavedChanges.insert(key);
        QModelIndex changedIndex = QAbstractTableModel::index(seedFileIndex.row(), Column::ColumnFileName);
        emit dataChanged(changedIndex, changedIndex, { Qt::DisplayRole, Qt::FontRole });

        return true;
    }

    bool SeedListFileTableModel::AddSeed(
        const QModelIndex& seedFileIndex,
        const AZStd::string& seedRelativePath,
        const AzFramework::PlatformFlags& platforms)
    {
        AZStd::string key = GetFileKey(seedFileIndex);
        if (key.empty())
        {
            // Error has already been thrown
            return false;
        }

        if (m_seedListFileInfoMap.find(key) == m_seedListFileInfoMap.end())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Unable to find Seed File Info with key ( %s )", key.c_str());
            return false;
        }

        SeedListFileInfoPtr seedFileInfo = m_seedListFileInfoMap[key];
        if (!seedFileInfo->m_seedListModel->AddSeed(seedRelativePath, platforms))
        {
            // Error has already been thrown
            return false;
        }

        // Update display so the user knows there are unsaved changes
        m_keysWithUnsavedChanges.insert(key);
        QModelIndex changedIndex = QAbstractTableModel::index(seedFileIndex.row(), Column::ColumnFileName);
        emit dataChanged(changedIndex, changedIndex, { Qt::DisplayRole, Qt::FontRole });

        return true;
    }

    bool SeedListFileTableModel::RemoveSeed(const QModelIndex& seedFileIndex, const QModelIndex& seedIndex)
    {
        AZStd::string key = GetFileKey(seedFileIndex);
        if (key.empty())
        {
            // Error has already been thrown
            return false;
        }

        if (m_seedListFileInfoMap.find(key) == m_seedListFileInfoMap.end())
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Unable to find Seed File Info with key ( %s )", key.c_str());
            return false;
        }

        SeedListFileInfoPtr seedFileInfo = m_seedListFileInfoMap[key];
        if (!seedFileInfo->m_seedListModel->RemoveSeed(seedIndex))
        {
            // Error has already been thrown
            return false;
        }

        // Update display so the user knows there are unsaved changes
        m_keysWithUnsavedChanges.insert(key);
        QModelIndex changedIndex = QAbstractTableModel::index(seedFileIndex.row(), Column::ColumnFileName);
        emit dataChanged(changedIndex, changedIndex, { Qt::DisplayRole, Qt::FontRole });

        return true;
    }

    bool SeedListFileTableModel::WriteToDisk(const AZStd::string& key)
    {
        if (key == m_inMemoryDefaultSeedListKey)
        {
            return true;
        }

        auto seedListFileIt = m_seedListFileInfoMap.find(key);
        return seedListFileIt != m_seedListFileInfoMap.end() && seedListFileIt->second->SaveSeedFile();
    }

    AZStd::string SeedListFileTableModel::GetFileAbsolutePath(const QModelIndex& index) const
    {
        auto seedFileInfoOutcome = GetSeedFileInfo(index);
        if (!seedFileInfoOutcome.IsSuccess() || seedFileInfoOutcome.GetValue()->m_absolutePath == m_inMemoryDefaultSeedListKey)
        {
            // Error has already been thrown
            return AZStd::string();
        }
        return seedFileInfoOutcome.GetValue()->m_absolutePath;
    }

    int SeedListFileTableModel::GetFileNameColumnIndex() const
    {
        return Column::ColumnFileName;
    }

    int SeedListFileTableModel::GetTimeStampColumnIndex() const
    {
        return Column::ColumnFileModificationTime;
    }

    int SeedListFileTableModel::columnCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : Column::Max;
    }

    QVariant SeedListFileTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case Column::ColumnFileName:
                return QString(tr("Seed List File"));
            case Column::ColumnProject:
                return QString(tr("Project Source"));
            case Column::ColumnFileModificationTime:
                return QString(tr("Modification Time"));
            default:
                break;
            }
        }

        return QVariant();
    }

    QVariant SeedListFileTableModel::data(const QModelIndex& index, int role) const
    {
        auto seedFileInfoOutcome = GetSeedFileInfo(index);
        if (!seedFileInfoOutcome.IsSuccess())
        {
            return QVariant();
        }

        int col = index.column();
        bool hasUnsavedChanges = seedFileInfoOutcome.GetValue()->m_seedListModel->HasUnsavedChanges();

        switch (role)
        {
        case Qt::DisplayRole:
            [[fallthrough]];
        case SortRole:
        {
            if (col == Column::ColumnFileName)
            {
                QString displayName = seedFileInfoOutcome.GetValue()->m_fileName;
                if (hasUnsavedChanges)
                {
                    displayName.append("*");
                }
                return displayName;
            }
            else if (col == Column::ColumnProject)
            {
                return seedFileInfoOutcome.GetValue()->m_project;
            }
            else if (col == Column::ColumnFileModificationTime)
            {
                if (role == SortRole)
                {
                    return seedFileInfoOutcome.GetValue()->m_fileModificationTime;
                }
                else
                {
                    return seedFileInfoOutcome.GetValue()->m_fileModificationTime.toString(DateTimeFormat);
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
            if (col == Column::ColumnFileName && hasUnsavedChanges)
            {
                QFont boldFont;
                boldFont.setBold(true);
                return boldFont;
            }
        }
        case Qt::CheckStateRole:
        {
            if (col == Column::ColumnCheckBox)
            {
                if (seedFileInfoOutcome.GetValue()->m_isChecked)
                {
                    return Qt::Checked;
                }
                else
                {
                    return Qt::Unchecked;
                }
            }
        }
        default:
            break;
        }

        return QVariant();
    }

    bool SeedListFileTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (role != Qt::CheckStateRole || index.column() != Column::ColumnCheckBox)
        {
            return false;
        }

        AZStd::string key = GetFileKey(index);
        if (key.empty())
        {
            return false;
        }

        auto seedFileInfo = m_seedListFileInfoMap.find(key);
        if (seedFileInfo == m_seedListFileInfoMap.end())
        {
            return false;
        }

        if (value == Qt::CheckState::Unchecked)
        {
            seedFileInfo->second->m_isChecked = false;
            m_checkedSeedListFiles.erase(key);

            if (seedFileInfo->second->m_isDefaultSeedList)
            {
                m_seedTabWidget->UncheckSelectDefaultSeedListsCheckBox();
            }
        }
        else
        {
            seedFileInfo->second->m_isChecked = true;
            m_checkedSeedListFiles.insert(key);
        }

        m_seedTabWidget->SetGenerateAssetListsButtonEnabled(!m_checkedSeedListFiles.empty());

        return true;
    }

    Qt::ItemFlags SeedListFileTableModel::flags(const QModelIndex& index) const
    {
        if (index.column() == Column::ColumnCheckBox)
        {
            return Qt::ItemIsUserCheckable | QAbstractTableModel::flags(index);
        }

        return QAbstractTableModel::flags(index);
    }

    AZ::Outcome<SeedListFileInfoPtr, void> SeedListFileTableModel::GetSeedFileInfo(const QModelIndex& index) const
    {
        AZStd::string key = GetFileKey(index);
        if (key.empty())
        {
            // Error has already been thrown
            return AZ::Failure();
        }

        if (m_seedListFileInfoMap.find(key) != m_seedListFileInfoMap.end())
        {
            return AZ::Success(m_seedListFileInfoMap.at(key));
        }
        else
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Cannot find Seed File Info");
            return AZ::Failure();
        }
    }

} // namespace AssetBundler
