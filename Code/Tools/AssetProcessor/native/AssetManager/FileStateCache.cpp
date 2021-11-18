/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FileStateCache.h"
#include "native/utilities/assetUtils.h"
#include <AssetProcessor_Traits_Platform.h>

#include <QDir>

namespace AssetProcessor
{

    bool FileStateCache::GetFileInfo(const QString& absolutePath, FileStateInfo* foundFileInfo) const
    {
        LockGuardType scopeLock(m_mapMutex);
        auto itr = m_fileInfoMap.find(PathToKey(absolutePath));

        if (itr != m_fileInfoMap.end())
        {
            *foundFileInfo = itr.value();
            return true;
        }

        return false;
    }

    bool FileStateCache::Exists(const QString& absolutePath) const
    {
        LockGuardType scopeLock(m_mapMutex);
        auto itr = m_fileInfoMap.find(PathToKey(absolutePath));

        return itr != m_fileInfoMap.end();
    }

    bool FileStateCache::GetHash(const QString& absolutePath, FileHash* foundHash)
    {
        LockGuardType scopeLock(m_mapMutex);
        auto fileInfoItr = m_fileInfoMap.find(PathToKey(absolutePath));

        if(fileInfoItr == m_fileInfoMap.end())
        {
            // No info on this file, return false
            return false;
        }

        auto itr = m_fileHashMap.find(PathToKey(absolutePath));

        if (itr != m_fileHashMap.end())
        {
            *foundHash = itr.value();
            return true;
        }

        // There's no hash stored yet or its been invalidated, calculate it
        *foundHash = AssetUtilities::GetFileHash(absolutePath.toUtf8().constData(), true);

        m_fileHashMap[PathToKey(absolutePath)] = *foundHash;
        return true;
    }

    void FileStateCache::RegisterForDeleteEvent(AZ::Event<FileStateInfo>::Handler& handler)
    {
        handler.Connect(m_deleteEvent);
    }

    void FileStateCache::AddInfoSet(QSet<AssetFileInfo> infoSet)
    {
        LockGuardType scopeLock(m_mapMutex);
        for (const AssetFileInfo& info : infoSet)
        {
            m_fileInfoMap[PathToKey(info.m_filePath)] = FileStateInfo(info);
        }
    }

    void FileStateCache::AddFile(const QString& absolutePath)
    {
        QFileInfo fileInfo(absolutePath);
        LockGuardType scopeLock(m_mapMutex);

        AddOrUpdateFileInternal(fileInfo);
        InvalidateHash(absolutePath);

        if(fileInfo.isDir())
        {
            ScanFolder(absolutePath);
        }
    }

    void FileStateCache::UpdateFile(const QString& absolutePath)
    {
        QFileInfo fileInfo(absolutePath);
        LockGuardType scopeLock(m_mapMutex);

        AddOrUpdateFileInternal(fileInfo);
        InvalidateHash(absolutePath);
    }

    void FileStateCache::RemoveFile(const QString& absolutePath)
    {
        LockGuardType scopeLock(m_mapMutex);

        auto itr = m_fileInfoMap.find(PathToKey(absolutePath));

        if (itr != m_fileInfoMap.end())
        {
            m_deleteEvent.Signal(itr.value());

            bool isDirectory = itr.value().m_isDirectory;
            QString parentPath = itr.value().m_absolutePath;
            m_fileInfoMap.erase(itr);

            if (isDirectory)
            {
                for (itr = m_fileInfoMap.begin(); itr != m_fileInfoMap.end(); )
                {
                    if (itr.value().m_absolutePath.startsWith(parentPath))
                    {
                        itr = m_fileInfoMap.erase(itr);
                        continue;
                    }

                    ++itr;
                }
            }
        }

        InvalidateHash(absolutePath);
    }

    void FileStateCache::InvalidateHash(const QString& absolutePath)
    {
        auto fileHashItr = m_fileHashMap.find(PathToKey(absolutePath));

        if (fileHashItr != m_fileHashMap.end())
        {
            m_fileHashMap.erase(fileHashItr);
        }
    }

    //////////////////////////////////////////////////////////////////////////

    QString FileStateCache::PathToKey(const QString& absolutePath) const
    {
        QString normalized = AssetUtilities::NormalizeFilePath(absolutePath);

        if constexpr (!ASSETPROCESSOR_TRAIT_CASE_SENSITIVE_FILESYSTEM)
        {
            return normalized.toLower();
        }
        else
        {
            return normalized;
        }
    }

    void FileStateCache::AddOrUpdateFileInternal(QFileInfo fileInfo)
    {
        m_fileInfoMap[PathToKey(fileInfo.absoluteFilePath())] = FileStateInfo(fileInfo.absoluteFilePath(), fileInfo.lastModified(), fileInfo.size(), fileInfo.isDir());
    }

    void FileStateCache::ScanFolder(const QString& absolutePath)
    {
        QDir inputFolder(absolutePath);
        QFileInfoList entries = inputFolder.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files);

        for (const QFileInfo& entry : entries)
        {
            AddOrUpdateFileInternal(entry);

            if (entry.isDir())
            {
                ScanFolder(entry.absoluteFilePath());
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////

    bool FileStatePassthrough::GetFileInfo(const QString& absolutePath, FileStateInfo* foundFileInfo) const
    {
        QFileInfo fileInfo(absolutePath);

        if (fileInfo.exists())
        {
            *foundFileInfo = FileStateInfo(fileInfo.absoluteFilePath(), fileInfo.lastModified(), fileInfo.size(), fileInfo.isDir());

            return true;
        }

        return false;
    }

    bool FileStatePassthrough::Exists(const QString& absolutePath) const
    {
        return QFile(absolutePath).exists();
    }

    bool FileStatePassthrough::GetHash(const QString& absolutePath, FileHash* foundHash)
    {
        if(!Exists(absolutePath))
        {
            return false;
        }

        *foundHash = AssetUtilities::GetFileHash(absolutePath.toUtf8().constData(), true);

        return true;
    }

    void FileStatePassthrough::RegisterForDeleteEvent(AZ::Event<FileStateInfo>::Handler& handler)
    {
        handler.Connect(m_deleteEvent);
    }

    void FileStatePassthrough::SignalDeleteEvent(const QString& absolutePath) const
    {
        FileStateInfo info;

        if (GetFileInfo(absolutePath, &info))
        {
            m_deleteEvent.Signal(info);
        }
    }

    bool FileStateInfo::operator==(const FileStateInfo& rhs) const
    {
        return m_absolutePath == rhs.m_absolutePath
            && m_modTime == rhs.m_modTime
            && m_fileSize == rhs.m_fileSize
            && m_isDirectory == rhs.m_isDirectory;
    }

} // namespace AssetProcessor
