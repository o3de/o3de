/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/Memory.h>

// This header file and CPP handles the platform specific implementation of code as defined by the FileIOBase interface class.
// In order to make your code portable and functional with both this and the RemoteFileIO class, use the interface to access
// the methods.

namespace AZ
{
    namespace IO
    {
        class SystemFile;
        class LocalFileIO
            : public FileIOBase
        {
        public:
            AZ_RTTI(LocalFileIO, "{87A8D32B-F695-4105-9A4D-D99BE15DFD50}", FileIOBase);
            AZ_CLASS_ALLOCATOR(LocalFileIO, SystemAllocator, 0);

            LocalFileIO();
            ~LocalFileIO();

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
            Result Rename(const char* originalFilePath, const char* newFilePath) override;
            Result FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback) override;

            void SetAlias(const char* alias, const char* path) override;
            void ClearAlias(const char* alias) override;
            const char* GetAlias(const char* alias) const override;
            void SetDeprecatedAlias(AZStd::string_view oldAlias, AZStd::string_view newAlias) override;

            AZStd::optional<AZ::u64> ConvertToAlias(char* inOutBuffer, AZ::u64 bufferLength) const override;
            bool ConvertToAlias(AZ::IO::FixedMaxPath& convertedPath, const AZ::IO::PathView& path) const override;
            using FileIOBase::ConvertToAlias;
            bool ResolvePath(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize) const override;
            bool ResolvePath(AZ::IO::FixedMaxPath& resolvedPath, const AZ::IO::PathView& path) const override;
            using FileIOBase::ResolvePath;
            bool ReplaceAlias(AZ::IO::FixedMaxPath& replacedAliasPath, const AZ::IO::PathView& path) const override;

            bool GetFilename(HandleType fileHandle, char* filename, AZ::u64 filenameSize) const override;
            bool ConvertToAbsolutePath(const char* path, char* absolutePath, AZ::u64 maxLength) const;

        private:
            SystemFile* GetFilePointerFromHandle(HandleType fileHandle);

            HandleType GetNextHandle();

            AZStd::optional<AZ::u64> ConvertToAliasBuffer(char* outBuffer, AZ::u64 outBufferLength, AZStd::string_view inBuffer) const;
            bool ResolveAliases(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize) const;

            bool LowerIfBeginsWith(char* inOutBuffer, AZ::u64 bufferLen, const char* alias) const;

        private:
            static AZStd::string RemoveTrailingSlash(const AZStd::string& pathStr);
            static AZStd::string CheckForTrailingSlash(const AZStd::string& pathStr);

            mutable AZStd::recursive_mutex m_openFileGuard;
            AZStd::atomic<HandleType> m_nextHandle;
            AZStd::unordered_map<HandleType, SystemFile> m_openFiles;
            AZStd::unordered_map<AZStd::string, AZStd::string> m_aliases;
            AZStd::unordered_map<AZStd::string, AZStd::string> m_deprecatedAliases;

            void CheckInvalidWrite(const char* path);
        };
    } // namespace IO
} // namespace AZ

