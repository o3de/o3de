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
#include <AzToolsFramework/Asset/AssetUtils.h>

#include <QDir>

namespace AssetProcessor
{
    // Note that the file state cache operates on the assumption that it is automatically loaded and kept
    // up to date by the file scanner (initially) and the file watcher (thereafter).  This is why all these
    // functions do not check the physical device for the file state, but rather rely on the cache.
    bool FileStateCache::GetFileInfo(const QString& absolutePath, FileStateInfo* foundFileInfo) const
    {
        AZ_Assert(!m_fileInfoMap.empty(), "FileStateCache::GetFileInfo called before cache is initialized!");

        LockGuardType scopeLock(m_mapMutex);
        auto itr = m_fileInfoMap.find(PathToKey(absolutePath));

        if (itr != m_fileInfoMap.end())
        {
            if (foundFileInfo)
            {
                *foundFileInfo = itr.value();
            }
            return true;
        }

        return false;
    }

    bool FileStateCache::Exists(const QString& absolutePath) const
    {
        LockGuardType scopeLock(m_mapMutex);
        AZ_Assert(!m_fileInfoMap.empty(), "FileStateCache::Exists called before cache is initialized!");
        return GetFileInfo(absolutePath, nullptr);
    }

    void FileStateCache::WarmUpCache(const AssetFileInfo& existingInfo, const FileHash hash)
    {
        LockGuardType scopeLock(m_mapMutex);
        QString key = PathToKey(existingInfo.m_filePath);
        m_fileInfoMap[key] = FileStateInfo(existingInfo);

        // it is possible to update the cache so that the info is known, but the hash is not.
        if (hash == InvalidFileHash)
        {
            m_fileHashMap.remove(key);
        }
        else
        {
            m_fileHashMap[key] = hash;
        }
    }

    bool FileStateCache::GetHash(const QString& absolutePath, FileHash* foundHash)
    {
        AZ_Assert(!m_fileInfoMap.empty(), "FileStateCache::Exists called before cache is initialized!");
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
        m_keyCache = {}; // Clear the key cache, its only really intended to help speedup the startup phase

        auto fileHashItr = m_fileHashMap.find(PathToKey(absolutePath));

        if (fileHashItr != m_fileHashMap.end())
        {
            m_fileHashMap.erase(fileHashItr);
        }
    }

    //////////////////////////////////////////////////////////////////////////

    QString FileStateCache::PathToKey(const QString& absolutePath) const
    {
        auto cached = m_keyCache.find(absolutePath);

        if (cached != m_keyCache.end())
        {
            return cached.value();
        }

        QString normalized = AssetUtilities::NormalizeFilePath(absolutePath);

        // Its possible for this API to be called on a case sensitive and case-insensitive file system for files
        // with the wrong case.  For example, a source asset might have another source asset listed in its dependency json
        // but with incorrect case.  If it were to call "Exists" or "GetFileInfo" with the wrong case, it would fail even
        // though the file actually does exist, and its a case insensitive system.  The API contract for this class demands
        // that it act as if case-insensitive, so the map MUST be lowercase.
        normalized = normalized.toLower();

        m_keyCache[absolutePath] = normalized;
        return normalized;
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
        if (QFileInfo fileInfo(absolutePath); fileInfo.exists())
        {
            if (foundFileInfo)
            {
                *foundFileInfo = FileStateInfo(fileInfo.absoluteFilePath(), fileInfo.lastModified(), fileInfo.size(), fileInfo.isDir());
            }
            return true;
        }
        
        if constexpr (ASSETPROCESSOR_TRAIT_CASE_SENSITIVE_FILESYSTEM)
        {
            // The above is done as a quick initial result, if the file exists, great, no need to do anything further.

            // However, its quite possible that the input absolute path has the wrong case.  On case-sensitive operating systems
            // we need to actually check if the case needs fixing...
            AZStd::string correctedPath = absolutePath.toUtf8().constData();
            AZStd::string rootPath = AZ::IO::PathView(correctedPath).RootPath().Native();
            correctedPath = correctedPath.substr(rootPath.size());
            if (AzToolsFramework::AssetUtils::UpdateFilePathToCorrectCase(rootPath.c_str(), correctedPath, true))
            {
                QString reassembledPath = QString::fromUtf8( (AZ::IO::Path(rootPath) / correctedPath).Native().c_str());
                QFileInfo correctedFileInfo(reassembledPath);
                if (correctedFileInfo.exists())
                {
                    if (foundFileInfo)
                    {
                        *foundFileInfo = FileStateInfo(correctedFileInfo.absoluteFilePath(), correctedFileInfo.lastModified(), correctedFileInfo.size(), correctedFileInfo.isDir());
                    }
                    return true;
                }
            }
        }

        return false;
    }

    bool FileStatePassthrough::Exists(const QString& absolutePath) const
    {
        return GetFileInfo(absolutePath, nullptr);
    }

    bool FileStatePassthrough::GetHash(const QString& absolutePath, FileHash* foundHash)
    {
        FileStateInfo fileInfo;
        if(!GetFileInfo(absolutePath, &fileInfo))
        {
            return false;
        }

        *foundHash = AssetUtilities::GetFileHash(fileInfo.m_absolutePath.toUtf8().constData(), true);

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
