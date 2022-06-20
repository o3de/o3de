/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <native/AssetManager/assetScanFolderInfo.h>
#include <QString>
#include <QSet>
#include <QFileInfo>
#include <AzCore/Interface/Interface.h>
#include <AzCore/EBus/Event.h>

namespace AssetProcessor
{
    struct FileStateInfo
    {
        FileStateInfo() = default;
        FileStateInfo(QString filePath, QDateTime modTime, AZ::u64 fileSize, bool isDirectory)
            : m_absolutePath(filePath), m_modTime(modTime), m_fileSize(fileSize), m_isDirectory(isDirectory) {}

        explicit FileStateInfo(const AssetFileInfo& assetFileInfo)
            : m_absolutePath(assetFileInfo.m_filePath), m_fileSize(assetFileInfo.m_fileSize), m_isDirectory(assetFileInfo.m_isDirectory), m_modTime(assetFileInfo.m_modTime)
        {

        }

        bool operator==(const FileStateInfo& rhs) const;

        QString m_absolutePath{};
        QDateTime m_modTime{};
        AZ::u64 m_fileSize{};
        bool m_isDirectory{};
    };

    struct IFileStateRequests
    {
        AZ_RTTI(IFileStateRequests, "{2D883B3A-DCA3-4CE0-976C-4511C3277371}");

        IFileStateRequests() = default;
        virtual ~IFileStateRequests() = default;

        using FileHash = AZ::u64;

        /// Fetches info on the file/directory if it exists.  Returns true if it exists, false otherwise
        virtual bool GetFileInfo(const QString& absolutePath, FileStateInfo* foundFileInfo) const = 0;
        /// Convenience function to check if a file or directory exists.
        virtual bool Exists(const QString& absolutePath) const = 0;
        virtual bool GetHash(const QString& absolutePath, FileHash* foundHash) = 0;
        virtual void RegisterForDeleteEvent(AZ::Event<FileStateInfo>::Handler& handler) = 0;

        AZ_DISABLE_COPY_MOVE(IFileStateRequests);
    };

    class FileStateBase
        : public IFileStateRequests
    {
    public:
        FileStateBase()
        {
            AZ::Interface<IFileStateRequests>::Register(this);
        }

        virtual ~FileStateBase()
        {
            AZ::Interface<IFileStateRequests>::Unregister(this);
        }

        /// Bulk adds file state to the cache
        virtual void AddInfoSet(QSet<AssetFileInfo> /*infoSet*/) {}

        /// Adds a single file to the cache.  This will query the OS for the current state
        virtual void AddFile(const QString& /*absolutePath*/) {}

        /// Updates a single file in the cache.  This will query the OS for the current state
        virtual void UpdateFile(const QString& /*absolutePath*/) {}

        /// Removes a file from the cache
        virtual void RemoveFile(const QString& /*absolutePath*/) {}
    };

    /// Caches file state information retrieved by the file scanner and file watcher
    /// Profiling has shown it is faster (at least on windows) compared to asking the OS for file information every time
    class FileStateCache final :
        public FileStateBase
    {
    public:

        // FileStateRequestBus implementation
        bool GetFileInfo(const QString& absolutePath, FileStateInfo* foundFileInfo) const override;
        bool Exists(const QString& absolutePath) const override;
        bool GetHash(const QString& absolutePath, FileHash* foundHash) override;
        void RegisterForDeleteEvent(AZ::Event<FileStateInfo>::Handler& handler) override;

        void AddInfoSet(QSet<AssetFileInfo> infoSet) override;
        void AddFile(const QString& absolutePath) override;
        void UpdateFile(const QString& absolutePath) override;
        void RemoveFile(const QString& absolutePath) override;

    private:

        /// Invalidates the hash for a file so it will be re-computed next time it's requested
        void InvalidateHash(const QString& absolutePath);

        /// Handles converting a file path into a uniform format for use as a map key
        QString PathToKey(const QString& absolutePath) const;

        /// Add/Update a single file
        void AddOrUpdateFileInternal(QFileInfo fileInfo);

        /// Recursively collects all the files contained in the directory specified by absolutePath
        void ScanFolder(const QString& absolutePath);

        mutable AZStd::recursive_mutex m_mapMutex;
        QHash<QString, FileStateInfo> m_fileInfoMap;

        QHash<QString, FileHash> m_fileHashMap;

        AZ::Event<FileStateInfo> m_deleteEvent;

        using LockGuardType = AZStd::lock_guard<decltype(m_mapMutex)>;
    };

    /// Pass through version of the FileStateCache which does not cache anything.  Every request is redirected to the OS
    class FileStatePassthrough final :
        public FileStateBase
    {
    public:
        // FileStateRequestBus implementation
        bool GetFileInfo(const QString& absolutePath, FileStateInfo* foundFileInfo) const override;
        bool Exists(const QString& absolutePath) const override;
        bool GetHash(const QString& absolutePath, FileHash* foundHash) override;
        void RegisterForDeleteEvent(AZ::Event<FileStateInfo>::Handler& handler) override;

        void SignalDeleteEvent(const QString& absolutePath) const;
    protected:
        AZ::Event<FileStateInfo> m_deleteEvent;
    };
} // namespace AssetProcessor
