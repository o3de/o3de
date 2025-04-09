/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/FileProcessor/FileProcessor.h>

#include <native/utilities/PlatformConfiguration.h>

#include <QDir>

namespace FileProcessorPrivate
{
    bool FinishedScanning(AssetProcessor::AssetScanningStatus status)
    {
        return status == AssetProcessor::AssetScanningStatus::Completed ||
               status == AssetProcessor::AssetScanningStatus::Stopped;
    }

    QString GenerateUniqueFileKey(AZ::s64 scanFolder, const char* fileName)
    {
        return QString("%1:%2").arg(scanFolder).arg(fileName);
    }
}

namespace AssetProcessor
{
    using namespace FileProcessorPrivate;

    FileProcessor::FileProcessor(PlatformConfiguration* config)
        : m_platformConfig(config)
    {
        m_connection = AZStd::shared_ptr<AssetDatabaseConnection>(aznew AssetDatabaseConnection());
        m_connection->OpenDatabase();

        QDir cacheRootDir;
        if (!AssetUtilities::ComputeProjectCacheRoot(cacheRootDir))
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to compute cache root folder");
        }
        m_normalizedCacheRootPath = AssetUtilities::NormalizeDirectoryPath(cacheRootDir.absolutePath());
    }

    FileProcessor::~FileProcessor() = default;

    void FileProcessor::OnAssetScannerStatusChange(AssetScanningStatus status)
    {
        //when AssetScanner finished processing, synchronize Files table
        if (FileProcessorPrivate::FinishedScanning(status))
        {
            QMetaObject::invokeMethod(this, "Sync", Qt::QueuedConnection);
        }
    }

    void FileProcessor::AssessFilesFromScanner(QSet<AssetFileInfo> files)
    {
        for (const AssetFileInfo& file : files)
        {
            m_filesInAssetScanner.append(file);
        }
    }

    void FileProcessor::AssessFoldersFromScanner(QSet<AssetFileInfo> folders)
    {
        for (const AssetFileInfo& folder : folders)
        {
            m_filesInAssetScanner.append(folder);
        }
    }

    void FileProcessor::AssessAddedFile(QString filePath)
    {
        using namespace AzToolsFramework;

        if (m_shutdownSignalled)
        {
            return;
        }

        QString relativeFileName;
        QString scanFolderPath;
        if (!GetRelativePath(filePath, relativeFileName, scanFolderPath))
        {
            return;
        }

        const ScanFolderInfo* scanFolderInfo = m_platformConfig->GetScanFolderByPath(scanFolderPath);
        if (!scanFolderInfo)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to find the scan folder for file %s", filePath.toUtf8().constData());
            return;
        }

        AssetDatabase::FileDatabaseEntry file;
        file.m_scanFolderPK = scanFolderInfo->ScanFolderID();
        file.m_fileName = relativeFileName.toUtf8().constData();
        file.m_isFolder = QFileInfo(filePath).isDir();

        bool entryAlreadyExists;

        if (m_connection->InsertFile(file, entryAlreadyExists) && !entryAlreadyExists)
        {
            AssetSystem::FileInfosNotificationMessage message;
            message.m_type = AssetSystem::FileInfosNotificationMessage::FileAdded;
            message.m_fileID = file.m_fileID;
            ConnectionBus::Broadcast(&ConnectionBusTraits::Send, 0, message);
        }

        if (file.m_isFolder)
        {
            QDir folder(filePath);
            for (const QFileInfo& subFile : folder.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
            {
                AssessAddedFile(subFile.absoluteFilePath());
            }
        }
    }

    void FileProcessor::AssessDeletedFile(QString filePath)
    {
        using namespace AzToolsFramework;

        if (m_shutdownSignalled)
        {
            return;
        }

        QString relativeFileName;
        QString scanFolderPath;
        if (!GetRelativePath(filePath, relativeFileName, scanFolderPath))
        {
            return;
        }

        const ScanFolderInfo* scanFolderInfo = m_platformConfig->GetScanFolderByPath(scanFolderPath);
        if (!scanFolderInfo)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to find the scan folder for file %s", filePath.toUtf8().constData());
            return;
        }

        AssetDatabase::FileDatabaseEntry file;
        if (m_connection->GetFileByFileNameAndScanFolderId(relativeFileName, scanFolderInfo->ScanFolderID(), file) && DeleteFileRecursive(file))
        {
            AssetSystem::FileInfosNotificationMessage message;
            message.m_type = AssetSystem::FileInfosNotificationMessage::FileRemoved;
            message.m_fileID = file.m_fileID;
            ConnectionBus::Broadcast(&ConnectionBusTraits::Send, 0, message);
        }
    }

    void FileProcessor::Sync()
    {
        using namespace AzToolsFramework;

        if (m_shutdownSignalled)
        {
            return;
        }

        QMap<QString, AZ::s64> filesInDatabase;

        //query all current files from Files table
        auto filesFunction = [&filesInDatabase](AzToolsFramework::AssetDatabase::FileDatabaseEntry& entry)
        {
            QString uniqueKey = GenerateUniqueFileKey(entry.m_scanFolderPK, entry.m_fileName.c_str());
            filesInDatabase[uniqueKey] = entry.m_fileID;
            return true;
        };
        m_connection->QueryFilesTable(filesFunction);

        //first collect all fileIDs in Files table
        QSet<AZ::s64> missingFileIDs;
        for (AZ::s64 fileID : filesInDatabase.values())
        {
            missingFileIDs.insert(fileID);
        }

        AzToolsFramework::AssetDatabase::FileDatabaseEntryContainer filesToInsert;

        for (const AssetFileInfo& fileInfo : m_filesInAssetScanner)
        {
            bool isDir = fileInfo.m_isDirectory;
            QString scanFolderPath;
            QString relativeFileName;

            if (!m_platformConfig->ConvertToRelativePath(fileInfo.m_filePath, relativeFileName, scanFolderPath))
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to convert full path to relative for file %s", fileInfo.m_filePath.toUtf8().constData());
                continue;
            }

            const ScanFolderInfo* scanFolderInfo = m_platformConfig->GetScanFolderByPath(scanFolderPath);
            if (!scanFolderInfo)
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to find the scan folder for file %s", fileInfo.m_filePath.toUtf8().constData());
                continue;
            }

            AssetDatabase::FileDatabaseEntry file;
            file.m_scanFolderPK = scanFolderInfo->ScanFolderID();
            file.m_fileName = relativeFileName.toUtf8().constData();
            file.m_isFolder = isDir;
            file.m_modTime = 0;

            //when file is found by AssetScanner, remove it from the "missing" set
            QString uniqueKey = GenerateUniqueFileKey(file.m_scanFolderPK, relativeFileName.toUtf8().constData());
            if (filesInDatabase.contains(uniqueKey))
            {
                // found it, its not missing anymore.  (Its also already in the db)
                missingFileIDs.remove(filesInDatabase[uniqueKey]);
            }
            else
            {
                // its a new file we were previously unaware of.
                filesToInsert.push_back(AZStd::move(file));
            }
        }

        m_connection->InsertFiles(filesToInsert);

        // remove remaining files from the database as they no longer exist on hard drive
        for (AZ::s64 fileID : missingFileIDs)
        {
            m_connection->RemoveFile(fileID);
        }

        AssetSystem::FileInfosNotificationMessage message;
        ConnectionBus::Broadcast(&ConnectionBusTraits::Send, 0, message);

        // It's important to clear this out since rescanning will end up filling this up with duplicates otherwise
        QList<AssetFileInfo> emptyList;
        m_filesInAssetScanner.swap(emptyList);
    }

    // note that this function normalizes the path and also returns true only if the file is 'relevant'
    // meaning something we care about tracking (ignore list/ etc taken into account).
    bool FileProcessor::GetRelativePath(QString& filePath, QString& relativeFileName, QString& scanFolderPath) const
    {
        filePath = AssetUtilities::NormalizeFilePath(filePath);
        if (AssetUtilities::IsInCacheFolder(filePath.toUtf8().constData(), m_normalizedCacheRootPath.toUtf8().constData()))
        {
            // modifies/adds to the cache are irrelevant.  Deletions are all we care about
            return false;
        }

        if (m_platformConfig->IsFileExcluded(filePath))
        {
            return false; // we don't care about this kind of file.
        }

        if (!m_platformConfig->ConvertToRelativePath(filePath, relativeFileName, scanFolderPath))
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to convert full path to relative for file %s", filePath.toUtf8().constData());
            return false;
        }
        return true;
    }

    bool FileProcessor::DeleteFileRecursive(const AzToolsFramework::AssetDatabase::FileDatabaseEntry& file) const
    {
        using namespace AzToolsFramework;

        if (m_shutdownSignalled)
        {
            return false;
        }

        if (file.m_isFolder)
        {
            AssetDatabase::FileDatabaseEntryContainer container;
            AZStd::string searchStr = file.m_fileName + AZ_CORRECT_DATABASE_SEPARATOR;
            m_connection->GetFilesLikeFileNameScanFolderId(
                searchStr.c_str(),
                AssetDatabaseConnection::LikeType::StartsWith,
                file.m_scanFolderPK,
                container);
            for (const auto& subFile : container)
            {
                DeleteFileRecursive(subFile);
            }
        }

        return m_connection->RemoveFile(file.m_fileID);
    }

    void FileProcessor::QuitRequested()
    {
        m_shutdownSignalled = true;
        Q_EMIT ReadyToQuit(this);
    }

} // namespace AssetProcessor

#include "native/FileProcessor/moc_FileProcessor.cpp"
