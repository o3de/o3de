/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/models/AssetBundlerAbstractFileTableModel.h>
#include <source/utils/utils.h>

#include <AzFramework/IO/LocalFileIO.h>

#include <QSetIterator>

namespace AssetBundler
{
    const char* DateTimeFormat = "hh:mm:ss MMM dd, yyyy";
    const char* ReadOnlyFileErrorMessage = "File (%s) is Read-Only. Please check your version control and try again.";

    AssetBundlerAbstractFileTableModel::AssetBundlerAbstractFileTableModel(QObject* parent)
        : QAbstractTableModel(parent)
    {
    }

    void AssetBundlerAbstractFileTableModel::Reload(
        const char* fileExtension,
        const QSet<QString>& watchedFolders,
        const QSet<QString>& watchedFiles,
        const AZStd::unordered_map<AZStd::string, AZStd::string>& pathToProjectNameMap)
    {
        AZStd::vector<AZStd::string> keysToRemove = m_fileListKeys;

        // Reload all the files in the watched folders
        QString filter = QString("*.%1").arg(fileExtension);

        QSetIterator<QString> itr(watchedFolders);
        while (itr.hasNext())
        {
            QDir filesDir(itr.next());
            filesDir.setNameFilters({ filter });
            for (const QString& fileNameAndExtension : filesDir.entryList(QDir::Files))
            {
                AZStd::string absolutePath = filesDir.absoluteFilePath(fileNameAndExtension).toUtf8().data();
                AZStd::string projectName;
                if (pathToProjectNameMap.contains(absolutePath))
                {
                    projectName = pathToProjectNameMap.at(absolutePath);
                }

                // If a project name is already specified, then the associated file is a default file
                LoadFile(absolutePath, projectName, !projectName.empty());
                keysToRemove.erase(
                    AZStd::remove(keysToRemove.begin(), keysToRemove.end(), AssetBundler::GenerateKeyFromAbsolutePath(absolutePath)),
                    keysToRemove.end());
            }
        }

        // Reload all the watched files
        for (const QString& filePath : watchedFiles)
        {
            AZStd::string absolutePath = filePath.toUtf8().data();
            if (AZ::IO::FileIOBase::GetInstance()->Exists(absolutePath.c_str()))
            {
                AZStd::string projectName;
                if (pathToProjectNameMap.contains(absolutePath))
                {
                    projectName = pathToProjectNameMap.at(absolutePath);
                }

                // If a project name is already specified, then the associated file is a default file
                LoadFile(absolutePath, projectName, !projectName.empty());
                keysToRemove.erase(
                    AZStd::remove(keysToRemove.begin(), keysToRemove.end(), AssetBundler::GenerateKeyFromAbsolutePath(absolutePath)),
                    keysToRemove.end());
            }
        }

        //Remove nonexistant files from the model
        for (const AZStd::string& key : keysToRemove)
        {
            DeleteFileByKey(key);
        }
    }

    void AssetBundlerAbstractFileTableModel::ReloadFiles(
        const AZStd::vector<AZStd::string>& absoluteFilePathList,
        AZStd::unordered_map<AZStd::string, AZStd::string> pathToProjectNameMap)
    {
        for (const AZStd::string& absoluteFilePath : absoluteFilePathList)
        {
            if (!AZ::IO::FileIOBase::GetInstance()->Exists(absoluteFilePath.c_str()))
            {
                // File is not present on-disk, make sure to remove it from the model
                DeleteFileByKey(AssetBundler::GenerateKeyFromAbsolutePath(absoluteFilePath));
                continue;
            }

            // If a project name is already specified, then the associated file is a default file
            AZStd::string projectName = pathToProjectNameMap[absoluteFilePath];
            LoadFile(absoluteFilePath, projectName, !projectName.empty());
        }
    }

    bool AssetBundlerAbstractFileTableModel::Save(const QModelIndex& selectedIndex)
    {
        if (!selectedIndex.isValid() || m_keysWithUnsavedChanges.empty())
        {
            // There is nothing to save
            return true;
        }

        AZStd::string key = GetFileKey(selectedIndex);
        if (key.empty())
        {
            // Error has already been thrown
            return false;
        }

        // Save the file
        if (!WriteToDisk(key))
        {
            return false;
        }

        // Update the display 
        m_keysWithUnsavedChanges.erase(key);
        QModelIndex topLeftIndex = index(selectedIndex.row(), 0);
        QModelIndex bottomRightIndex = index(selectedIndex.row(), columnCount() - 1);
        emit dataChanged(topLeftIndex, bottomRightIndex, { Qt::DisplayRole, Qt::FontRole });

        return true;
    }

    bool AssetBundlerAbstractFileTableModel::SaveAll()
    {
        if (!HasUnsavedChanges())
        {
            // No need to update all of the elements if we are not changing anything
            return true;
        }

        bool hasSaveErrors = false;

        // Save every file with unsaved changes
        AZStd::unordered_set<AZStd::string> keysWithErrors;
        for (const AZStd::string& key : m_keysWithUnsavedChanges)
        {
            if (!WriteToDisk(key))
            {
                // Error has already been thrown
                hasSaveErrors = true;
                keysWithErrors.emplace(key);
            }
        }

        m_keysWithUnsavedChanges = keysWithErrors;

        // Update the display of all elements
        QModelIndex firstIndex = index(0, 0);
        QModelIndex lastIndex = index(rowCount() - 1, columnCount() - 1);
        emit dataChanged(firstIndex, lastIndex, { Qt::DisplayRole, Qt::FontRole });

        return !hasSaveErrors;
    }

    bool AssetBundlerAbstractFileTableModel::HasUnsavedChanges() const
    {
        return !m_keysWithUnsavedChanges.empty();
    }

    int AssetBundlerAbstractFileTableModel::rowCount(const QModelIndex& /*parent*/) const
    {
        return static_cast<int>(m_fileListKeys.size());
    }

    AZStd::string AssetBundlerAbstractFileTableModel::GetFileKey(const QModelIndex& index) const
    {
        int row = index.row();
        int col = index.column();
        if (row >= rowCount() || row < 0 || col >= columnCount() || col < 0)
        {
            AZ_Error("AssetBundler", false, "Selected index (%i, %i) is out of range.", row, col);
            return {};
        }

        return m_fileListKeys.at(row);
    }

    const AZStd::vector<AZStd::string>& AssetBundlerAbstractFileTableModel::GetAllFileKeys() const
    {
        return m_fileListKeys;
    }

    int AssetBundlerAbstractFileTableModel::GetIndexRowByKey(const AZStd::string& key) const
    {
        auto it = AZStd::find(m_fileListKeys.begin(), m_fileListKeys.end(), key);
        return it == m_fileListKeys.end() ? -1 : static_cast<int>(AZStd::distance(m_fileListKeys.begin(), it));
    }

    void AssetBundlerAbstractFileTableModel::AddFileKey(const AZStd::string& key)
    {
        if (AZStd::find(m_fileListKeys.begin(), m_fileListKeys.end(), key) != m_fileListKeys.end())
        {
            // Key already exists. This could happen when we update existing entires.
            return;
        }

        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        m_fileListKeys.push_back(key);
        endInsertRows();
    }

    bool AssetBundlerAbstractFileTableModel::RemoveFileKey(const QModelIndex& index)
    {
        AZStd::string key = GetFileKey(index);
        if (key.empty())
        {
            // Error has already been thrown
            return false;
        }

        int row = index.row();
        beginRemoveRows(QModelIndex(), row, row);

        m_fileListKeys.erase(m_fileListKeys.begin() + row);
        m_keysWithUnsavedChanges.erase(key);

        endRemoveRows();

        return true;
    }

    bool AssetBundlerAbstractFileTableModel::DeleteFileByKey(const AZStd::string& key)
    {
        int indexRow = GetIndexRowByKey(key);
        if (indexRow >= 0)
        {
            return DeleteFile(index(indexRow, 0));
        }

        return false;     
    }
} // namespace AssetBundler
