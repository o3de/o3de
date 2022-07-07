/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/FileIO.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/parallel/mutex.h>


namespace AZ::IO
{
    inline constexpr size_t ArchiveFileIoMaxBuffersize = 16 * 1024;
    struct IArchive;
    //! ArchiveFileIO
    //! An implementation of the FileIOBase which pipes all operations via Archive,
    //! which itself pipes all operations via Local or RemoteFileIO.
    //! this allows us to talk to files inside packfiles, without having to change the interface.

    class ArchiveFileIO
        : public AZ::IO::FileIOBase
    {
    public:
        AZ_RTTI(ArchiveFileIO, "{679F8DB8-CC61-4BC8-ADDB-170E3D428B5D}", FileIOBase);
        AZ_CLASS_ALLOCATOR(ArchiveFileIO, OSAllocator, 0);

        ArchiveFileIO(IArchive* archive);
        ~ArchiveFileIO();

        void SetArchive(IArchive* archive);
        IArchive* GetArchive() const;

        ////////////////////////////////////////////////////////////////////////////////////////
        //implementation of FileIOBase

        IO::Result Open(const char* filePath, IO::OpenMode mode, IO::HandleType& fileHandle) override;
        IO::Result Close(IO::HandleType fileHandle) override;
        IO::Result Tell(IO::HandleType fileHandle, AZ::u64& offset) override;
        IO::Result Seek(IO::HandleType fileHandle, AZ::s64 offset, IO::SeekType type) override;
        IO::Result Size(IO::HandleType fileHandle, AZ::u64& size) override;
        IO::Result Read(IO::HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead = false, AZ::u64* bytesRead = nullptr) override;
        IO::Result Write(IO::HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten = nullptr) override;
        IO::Result Flush(IO::HandleType fileHandle) override;
        bool Eof(IO::HandleType fileHandle) override;
        AZ::u64 ModificationTime(IO::HandleType fileHandle) override;
        bool Exists(const char* filePath) override;
        IO::Result Size(const char* filePath, AZ::u64& size) override;
        AZ::u64 ModificationTime(const char* filePath) override;
        bool IsDirectory(const char* filePath) override;
        bool IsReadOnly(const char* filePath) override;
        IO::Result CreatePath(const char* filePath) override;
        IO::Result DestroyPath(const char* filePath) override;
        IO::Result Remove(const char* filePath) override;
        IO::Result Copy(const char* sourceFilePath, const char* destinationFilePath) override;
        IO::Result Rename(const char* sourceFilePath, const char* destinationFilePath) override;
        IO::Result FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback) override;
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
        bool GetFilename(IO::HandleType fileHandle, char* filename, AZ::u64 filenameSize) const override;
        ////////////////////////////////////////////////////////////////////////////////////////////

    protected:
        // we keep a list of file names ever opened so that we can easily return it.
        mutable AZStd::recursive_mutex m_operationGuard;
        AZStd::unordered_map<IO::HandleType, AZ::IO::Path> m_trackedFiles;
        AZStd::fixed_vector<char, ArchiveFileIoMaxBuffersize> m_copyBuffer;
        IArchive* m_archive;
    };
} // namespace AZ::IO
