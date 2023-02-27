/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/FileIO.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/RTTI/RTTI.h>

////////////////////////////////////////////////////////////////////////////////////
//NetworkFileIO implements FileIOBase for serving all file system requests via the asset
//processor connection. The asset processor on the other side of the connection uses LocalFileIO
//to complete the task and sends the results back.
//NetworkFileIO uses no caching at all.
//RemoteFileIO derives from NetworkFileIO and adds caching to speed things up.

//This option defines RemoteFileIO as NetworkFileIO, an easy way to test with no caching at all
//#define REMOTEFILEIO_IS_NETWORKFILEIO

//REMOTEFILEIO_CACHE_FILETREE is an option that turns on file metadata caching of the cache tree.
//When on it queries the AP at startup for the state of the cache files and caches it. It also
//registers itself as a listener for file changes and recaches on changes.
//#define REMOTEFILEIO_CACHE_FILETREE

//REMOTEFILEIO_CACHE_FILETREE_FALLBACK is an option you can turn on that in the event the instance
//asks about file meta data that doesnt exist in the cache, we can fallback to NetworkFileIO and
//see if something changed. For example if someone adds a directory and the code asks
//if that directory exists, it wont be in the cache so it would normally report no, with the fall
//it would secondary query NetworkFileIO to get the real state and return it in an effort to be
//safer in edge cases in dynamic development.
//#define REMOTEFILEIO_CACHE_FILETREE_FALLBACK

///////////////////////////////////////////////////////////////////////////////////
//NETWORKFILEIO_LOG is a logging option that must tolerate high speed logging, so
//no output and no allocations, to not alter the timing of calls. On start it allocs
//a chunk of memmory and writes into it. It is a super verbose logging every file call.
//#define NETWORKFILEIO_LOG

//REMOTEFILEIO_SYNC_CHECK is an option to enable sync checks between the readahead cache
//and where the server currently thinks the file is at. It is very useful when debugging
//a combination of virtual file actions that result in a slightly different state. For example
//seek()'ing before the begining or past the end of a file is undefined in the standard, and
//different platforms handle what happens differently. One may allow it, others may not. When
//it is allowed sometime it means padding the file to a new size with zeros, sometime it just
//stops at the bounds... this can be used to figure out what is going on in wierd out of sync
//cases that arrise from a wierd combination of calls on different platforms.
//#define REMOTEFILEIO_SYNC_CHECK


#ifdef REMOTEFILEIO_CACHE_FILETREE
#include <AzFramework/Asset/AssetCatalogBus.h>
#endif
///////////////////////////////////////////////////////////////////////////////////
namespace AZ
{
    namespace IO
    {
        class NetworkFileIO
            : public FileIOBase
        {
        public:
            AZ_RTTI(NetworkFileIO, "{A863335E-9330-44E2-AD89-B5309F3B8B93}", FileIOBase);
            AZ_CLASS_ALLOCATOR(NetworkFileIO, OSAllocator);

            NetworkFileIO();
            virtual ~NetworkFileIO();

            ////////////////////////////////////////////////////////////////////////////////////////
            //implementation of FileIOBase

            Result Open(const char* filePath, OpenMode mode, HandleType& fileHandle) override;
            Result Close(HandleType fileHandle) override;
            Result Tell(HandleType fileHandle, AZ::u64& offset) override;
            Result Seek(HandleType fileHandle, AZ::s64 offset, SeekType type) override;
            Result Size(HandleType fileHandle, AZ::u64& size) override;
            Result Read(HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead = false, AZ::u64* bytesRead = nullptr) override;
            Result Write(HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten = nullptr) override;
            Result Flush(HandleType fileHandle) override;
            bool Eof(HandleType fileHandle) override;
            AZ::u64 ModificationTime(HandleType fileHandle) override;
            bool Exists(const char* filePath) override;
            Result Size(const char* filePath, AZ::u64& size) override;
            AZ::u64 ModificationTime(const char* filePath) override;
            bool IsDirectory(const char* filePath) override;
            bool IsReadOnly(const char* filePath) override;
            Result CreatePath(const char* filePath) override;
            Result DestroyPath(const char* filePath) override;
            Result Remove(const char* filePath) override;
            Result Copy(const char* sourceFilePath, const char* destinationFilePath) override;
            Result Rename(const char* sourceFilePath, const char* destinationFilePath) override;
            Result FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback) override;
            void SetAlias(const char* alias, const char* path) override;
            void ClearAlias(const char* alias) override;
            void SetDeprecatedAlias(AZStd::string_view oldAlias, AZStd::string_view newAlias) override;
            AZStd::optional<AZ::u64> ConvertToAlias(char* inOutBuffer, AZ::u64 bufferLength) const override;
            bool ConvertToAlias(AZ::IO::FixedMaxPath& convertedPath, const AZ::IO::PathView& path) const override;
            using FileIOBase::ConvertToAlias;
            const char* GetAlias(const char* alias) const override;
            bool ResolvePath(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize) const override;
            bool ResolvePath(AZ::IO::FixedMaxPath& resolvedPath, const AZ::IO::PathView& path) const override;
            using FileIOBase::ResolvePath;
            bool ReplaceAlias(AZ::IO::FixedMaxPath& replacedAliasPath, const AZ::IO::PathView& path) const override;
            bool GetFilename(HandleType fileHandle, char* filename, AZ::u64 filenameSize) const override;
            bool IsRemoteIOEnabled() override;
            ////////////////////////////////////////////////////////////////////////////////////////////

        protected:
            mutable AZStd::recursive_mutex m_remoteFilesGuard;
            AZStd::unordered_map<HandleType, AZ::OSString, AZStd::hash<HandleType>, AZStd::equal_to<HandleType>, AZ::OSStdAllocator> m_remoteFiles;
        };

#ifdef REMOTEFILEIO_IS_NETWORKFILEIO
        #define RemoteFileIO NetworkFileIO 
#else
        //////////////////////////////////////////////////////////////////////////
        class RemoteFileCache
        {
        public:
            AZ_CLASS_ALLOCATOR(RemoteFileCache, OSAllocator);

            RemoteFileCache() = default;
            RemoteFileCache(const RemoteFileCache& other) = default;
            RemoteFileCache(RemoteFileCache&& other);
            RemoteFileCache& operator=(RemoteFileCache&& other);

            void Invalidate();
            AZ::u64 RemainingBytes();
            AZ::u64 CacheFilePosition();
            AZ::u64 CacheStartFilePosition();
            AZ::u64 CacheEndFilePosition();
            bool IsFilePositionInCache(AZ::u64 filePosition);
            void SetCachePositionFromFilePosition(AZ::u64 filePosition);
            void SetFilePosition(AZ::u64 filePosition);
            void OffsetFilePosition(AZ::s64 offset);
            bool Eof();

            void SyncCheck();

            AZStd::vector<char, AZ::OSStdAllocator> m_cacheLookaheadBuffer;
            AZ::u64 m_cacheLookaheadPos = 0;

            AZ::u64 m_fileSize = 0;
            AZ::u64 m_fileSizeTime = 0;

            // note that m_filePosition caches the actual physical pointer location of the file - not the cache pos.
            // the actual 'tell position' should be this number minus the number of bytes its ahead by in the cache.
            // Whenever something happens that will actually move the cursor on the remote host occurs, like
            // NetworkFileIO read, this should be updated. 
            AZ::u64 m_filePosition = 0;

            HandleType m_fileHandle = AZ::IO::InvalidHandle;

            OpenMode m_openMode = OpenMode::Invalid;
        };


        //////////////////////////////////////////////////////////////////////////

        class RemoteFileIO
            : public NetworkFileIO
#ifdef REMOTEFILEIO_CACHE_FILETREE
            , private AzFramework::AssetCatalogEventBus::Handler
#endif
        {
        public:
            AZ_RTTI(RemoteFileIO, "{E2939E15-3B83-402A-A6DA-A436EDAB2ED2}", NetworkFileIO);
            AZ_CLASS_ALLOCATOR(RemoteFileIO, OSAllocator);

            RemoteFileIO(FileIOBase* excludedFileIO = nullptr);
            virtual ~RemoteFileIO();
            
            ////////////////////////////////////////////////////////////////////////////////////////
            //implementation of NetworkFileIO

            Result Open(const char* filePath, OpenMode mode, HandleType& fileHandle) override;
            Result Close(HandleType fileHandle) override;
            Result Tell(HandleType fileHandle, AZ::u64& offset) override;
            Result Seek(HandleType fileHandle, AZ::s64 offset, SeekType type) override;
            Result Size(HandleType fileHandle, AZ::u64& size) override;
            Result Read(HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead = false, AZ::u64* bytesRead = nullptr) override;
            Result Write(HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten = nullptr) override;
            bool Eof(HandleType fileHandle) override;
            Result Size(const char* filePath, AZ::u64& size) override { return NetworkFileIO::Size(filePath, size); }
            void SetAlias(const char* alias, const char* path) override;
            const char* GetAlias(const char* alias) const override;
            void ClearAlias(const char* alias) override;
            void SetDeprecatedAlias(AZStd::string_view oldAlias, AZStd::string_view newAlias) override;
            AZStd::optional<AZ::u64> ConvertToAlias(char* inOutBuffer, AZ::u64 bufferLength) const override;
            bool ConvertToAlias(AZ::IO::FixedMaxPath& convertedPath, const AZ::IO::PathView& path) const override;
            using FileIOBase::ConvertToAlias;
            bool ResolvePath(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize) const override;
            bool ResolvePath(AZ::IO::FixedMaxPath& resolvedPath, const AZ::IO::PathView& path) const override;
            using FileIOBase::ResolvePath;
            bool ReplaceAlias(AZ::IO::FixedMaxPath& replacedAliasPath, const AZ::IO::PathView& path) const override;

#ifdef REMOTEFILEIO_CACHE_FILETREE
            bool Exists(const char* filePath) override;
            bool IsDirectory(const char* filePath) override;
            Result FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback) override;
            
            Result CacheFileTree();
            AZStd::vector<AZStd::string> m_remoteFileTreeCache;
            AZStd::vector<AZStd::string> m_remoteFolderTreeCache;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            //implementation of AssetCatalogEventBus
            void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;
            void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId) override;
            //////////////////////////////////////////////////////////////////////////
#endif
            mutable AZStd::recursive_mutex m_remoteFileCacheGuard;
            AZStd::unordered_map<HandleType, RemoteFileCache, AZStd::hash<HandleType>, AZStd::equal_to<HandleType>, AZ::OSStdAllocator> m_remoteFileCache;
            // (assumes you're inside a remote files guard lock already for creation, which is true since we create one on open)
            RemoteFileCache& GetCache(HandleType fileHandle);

        private:
            FileIOBase* m_excludedFileIO;
        };
#endif
    } // namespace IO
} // namespace AZ
