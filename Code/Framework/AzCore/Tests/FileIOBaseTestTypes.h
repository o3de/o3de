/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/functional.h>
#include <AzCore/UnitTest/UnitTest.h>

// This header file and CPP handles the platform specific implementation of code as defined by the FileIOBase interface class.
// In order to make your code portable and functional with both this and the RemoteFileIO class, use the interface to access
// the methods.

namespace UnitTest
{
class TestFileIOBase : public AZ::IO::FileIOBase
{
    mutable AZStd::recursive_mutex m_openFileGuard;
    AZStd::atomic<AZ::IO::HandleType> m_nextHandle;
    AZStd::map<AZ::IO::HandleType, AZ::IO::SystemFile, AZStd::less< AZ::IO::HandleType>, AZ::OSStdAllocator> m_openFiles;
    const AZ::IO::HandleType LocalHandleStartValue = 1000000; //start the local file io handles at 1 million
public:

    TestFileIOBase()
    {
        m_nextHandle = LocalHandleStartValue;
    }

    ~TestFileIOBase() override
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
        while (!m_openFiles.empty())
        {
            const auto& handlePair = *m_openFiles.begin();
            Close(handlePair.first);
        }

        AZ_Assert(m_openFiles.empty(), "Trying to shutdown filing system with files still open");
    }


    AZ::IO::Result Open(const char* filePath, AZ::IO::OpenMode mode, AZ::IO::HandleType& fileHandle) override
    {
        char resolvedPath[AZ_MAX_PATH_LEN];
        ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

        AZ::IO::UpdateOpenModeForReading(mode);

        // Generate open modes for SystemFile
        int systemFileMode = 0;
        {
            bool read = AnyFlag(mode & AZ::IO::OpenMode::ModeRead) || AnyFlag(mode & AZ::IO::OpenMode::ModeUpdate);
            bool write = AnyFlag(mode & AZ::IO::OpenMode::ModeWrite) || AnyFlag(mode & AZ::IO::OpenMode::ModeUpdate) || AnyFlag(mode & AZ::IO::OpenMode::ModeAppend);
            if (write)
            {
                // If writing the file, create the file in all cases (except r+)
                if (!Exists(resolvedPath) && !(AnyFlag(mode & AZ::IO::OpenMode::ModeRead) && AnyFlag(mode & AZ::IO::OpenMode::ModeUpdate)))
                {
                    // LocalFileIO creates by default
                    systemFileMode |= AZ::IO::SystemFile::SF_OPEN_CREATE;
                }

                // If writing and appending, append.
                if (AnyFlag(mode & AZ::IO::OpenMode::ModeAppend))
                {
                    systemFileMode |= AZ::IO::SystemFile::SF_OPEN_APPEND;
                }
                // If writing and NOT updating, empty the file.
                else if (!AnyFlag(mode & AZ::IO::OpenMode::ModeUpdate))
                {
                    systemFileMode |= AZ::IO::SystemFile::SF_OPEN_TRUNCATE;
                }

                // If reading, set read/write, otherwise just write
                if (read)
                {
                    systemFileMode |= AZ::IO::SystemFile::SF_OPEN_READ_WRITE;
                }
                else
                {
                    systemFileMode |= AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY;
                }
            }
            else if (read)
            {
                systemFileMode |= AZ::IO::SystemFile::SF_OPEN_READ_ONLY;
            }
        }

        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);

            fileHandle = GetNextHandle();

            // Construct a new SystemFile in the map (SystemFiles don't copy/move very well).
            auto newPair = m_openFiles.emplace(fileHandle);
            // Check for successful insert
            if (!newPair.second)
            {
                fileHandle = AZ::IO::InvalidHandle;
                return  AZ::IO::ResultCode::Error;
            }
            // Attempt to open the newly created file
            if (newPair.first->second.Open(resolvedPath, systemFileMode, 0))
            {
                return AZ::IO::ResultCode::Success;
            }
            else
            {
                // Remove file, it's not actually open
                m_openFiles.erase(fileHandle);
                // On failure, ensure the fileHandle returned is invalid
                //  some code does not check return but handle value (equivalent to checking for nullptr FILE*)
                fileHandle = AZ::IO::InvalidHandle;
                return AZ::IO::ResultCode::Error;
            }
        }
    }

    AZ::IO::Result Close(AZ::IO::HandleType fileHandle) override
    {
        auto filePointer = GetFilePointerFromHandle(fileHandle);
        if (!filePointer)
        {
            return AZ::IO::ResultCode::Error_HandleInvalid;
        }

        filePointer->Close();

        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
            m_openFiles.erase(fileHandle);
        }
        return AZ::IO::ResultCode::Success;
    }

    AZ::IO::Result Read(AZ::IO::HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead, AZ::u64* bytesRead) override
    {
        auto filePointer = GetFilePointerFromHandle(fileHandle);
        if (!filePointer)
        {
            return AZ::IO::ResultCode::Error_HandleInvalid;
        }

        AZ::IO::SizeType readResult = filePointer->Read(size, buffer);

        if (bytesRead)
        {
            *bytesRead = aznumeric_cast<AZ::u64>(readResult);
        }

        if (static_cast<AZ::u64>(readResult) != size)
        {
            if (failOnFewerThanSizeBytesRead)
            {
                return AZ::IO::ResultCode::Error;
            }

            // Reading less than desired is valid if ferror is not set
            AZ_Assert(Eof(fileHandle), "End of file unexpectedly reached before all data was read");
        }

        return AZ::IO::ResultCode::Success;
    }

    AZ::IO::Result Write(AZ::IO::HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten) override
    {
        auto filePointer = GetFilePointerFromHandle(fileHandle);
        if (!filePointer)
        {
            return AZ::IO::ResultCode::Error_HandleInvalid;
        }

        AZ::IO::SizeType writeResult = filePointer->Write(buffer, size);

        if (bytesWritten)
        {
            *bytesWritten = writeResult;
        }

        if (static_cast<AZ::u64>(writeResult) != size)
        {
            return AZ::IO::ResultCode::Error;
        }

        return AZ::IO::ResultCode::Success;
    }

    AZ::IO::Result Flush(AZ::IO::HandleType fileHandle) override
    {
        auto filePointer = GetFilePointerFromHandle(fileHandle);
        if (!filePointer)
        {
            return AZ::IO::ResultCode::Error_HandleInvalid;
        }

        filePointer->Flush();

        return AZ::IO::ResultCode::Success;
    }

    AZ::IO::Result Tell(AZ::IO::HandleType fileHandle, AZ::u64& offset) override
    {
        auto filePointer = GetFilePointerFromHandle(fileHandle);
        if (!filePointer)
        {
            return AZ::IO::ResultCode::Error_HandleInvalid;
        }

        AZ::IO::SizeType resultValue = filePointer->Tell();
        offset = static_cast<AZ::u64>(resultValue);
        return AZ::IO::ResultCode::Success;
    }

    AZ::IO::Result Seek(AZ::IO::HandleType fileHandle, AZ::s64 offset, AZ::IO::SeekType type) override
    {
        auto filePointer = GetFilePointerFromHandle(fileHandle);
        if (!filePointer)
        {
            return AZ::IO::ResultCode::Error_HandleInvalid;
        }

        AZ::IO::SystemFile::SeekMode mode = AZ::IO::SystemFile::SF_SEEK_BEGIN;
        if (type == AZ::IO::SeekType::SeekFromCurrent)
        {
            mode = AZ::IO::SystemFile::SF_SEEK_CURRENT;
        }
        else if (type == AZ::IO::SeekType::SeekFromEnd)
        {
            mode = AZ::IO::SystemFile::SF_SEEK_END;
        }

        filePointer->Seek(offset, mode);

        return AZ::IO::ResultCode::Success;
    }

    AZ::IO::Result Size(AZ::IO::HandleType fileHandle, AZ::u64& size) override
    {
        auto filePointer = GetFilePointerFromHandle(fileHandle);
        if (!filePointer)
        {
            return AZ::IO::ResultCode::Error_HandleInvalid;
        }

        size = filePointer->Length();
        return AZ::IO::ResultCode::Success;
    }

    AZ::IO::Result Size(const char* filePath, AZ::u64& size) override
    {
        char resolvedPath[AZ_MAX_PATH_LEN];
        ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

        size = AZ::IO::SystemFile::Length(resolvedPath);
        if (!size)
        {
            return AZ::IO::SystemFile::Exists(resolvedPath) ? AZ::IO::ResultCode::Success : AZ::IO::ResultCode::Error;
        }

        return AZ::IO::ResultCode::Success;
    }

    bool IsReadOnly(const char* filePath) override
    {
        char resolvedPath[AZ_MAX_PATH_LEN];
        ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

        return !AZ::IO::SystemFile::IsWritable(resolvedPath);
    }

    bool Eof(AZ::IO::HandleType fileHandle) override
    {
        auto filePointer = GetFilePointerFromHandle(fileHandle);
        if (!filePointer)
        {
            return false;
        }

        return filePointer->Eof();
    }

    AZ::u64 ModificationTime(const char* filePath) override
    {
        char resolvedPath[AZ_MAX_PATH_LEN];
        ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

        return AZ::IO::SystemFile::ModificationTime(resolvedPath);
    }

    AZ::u64 ModificationTime(AZ::IO::HandleType fileHandle) override
    {
        auto filePointer = GetFilePointerFromHandle(fileHandle);
        if (!filePointer)
        {
            return 0;
        }

        return filePointer->ModificationTime();
    }

    bool Exists(const char* filePath) override
    {
        char resolvedPath[AZ_MAX_PATH_LEN];
        ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

        return AZ::IO::SystemFile::Exists(resolvedPath);
    }

    AZ::IO::Result Remove(const char* filePath) override
    {
        char resolvedPath[AZ_MAX_PATH_LEN];

        ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

        if (IsDirectory(resolvedPath))
        {
            return AZ::IO::ResultCode::Error;
        }

        return AZ::IO::SystemFile::Delete(resolvedPath) ? AZ::IO::ResultCode::Success : AZ::IO::ResultCode::Error;
    }

    AZ::IO::Result Rename(const char* originalFilePath, const char* newFilePath) override
    {
        char resolvedOldPath[AZ_MAX_PATH_LEN];
        char resolvedNewPath[AZ_MAX_PATH_LEN];

        ResolvePath(originalFilePath, resolvedOldPath, AZ_MAX_PATH_LEN);
        ResolvePath(newFilePath, resolvedNewPath, AZ_MAX_PATH_LEN);

        if (!AZ::IO::SystemFile::Rename(resolvedOldPath, resolvedNewPath))
        {
            return AZ::IO::ResultCode::Error;
        }

        return AZ::IO::ResultCode::Success;
    }


    AZ::IO::SystemFile* GetFilePointerFromHandle(AZ::IO::HandleType fileHandle)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
        auto openFileIterator = m_openFiles.find(fileHandle);
        if (openFileIterator != m_openFiles.end())
        {
            AZ::IO::SystemFile& file = openFileIterator->second;
            return &file;
        }
        return nullptr;
    }

    AZ::IO::HandleType GetNextHandle()
    {
        return m_nextHandle++;
    }


    AZ::IO::Result DestroyPath(const char* ) override
    {
        return AZ::IO::ResultCode::Error;        
    }

    bool ResolvePath(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize) const override
    {
        using namespace testing::internal;

        if (path == nullptr)
        {
            return false;
        }

        FilePath filename = FilePath(path);
        if (filename.IsAbsolutePath())
        {
            azstrncpy(resolvedPath, resolvedPathSize, path, strlen(path) + 1);
        }
        else
        {
            FilePath prefix = FilePath::GetCurrentDir();
            FilePath result = FilePath::ConcatPaths(prefix, filename);
            azstrncpy(resolvedPath, resolvedPathSize, result.c_str(), result.string().length() + 1);
        }

        for (AZ::u64 i = 0; i < resolvedPathSize && resolvedPath[i] != '\0'; i++)
        {
            if (resolvedPath[i] == '\\')
            {
                resolvedPath[i] = '/';
            }
        }

        return true;
    }

    bool ResolvePath(AZ::IO::FixedMaxPath& resolvedPath, const AZ::IO::PathView& path) const override
    {
        if (ResolvePath(AZ::IO::FixedMaxPathString{ path.Native() }.c_str(), resolvedPath.Native().data(), resolvedPath.Native().capacity() + 1))
        {
            resolvedPath.Native().resize_no_construct(AZStd::char_traits<char>::length(resolvedPath.Native().data()));
            return true;
        }
        return false;
    }

    bool ReplaceAlias(AZ::IO::FixedMaxPath&, const AZ::IO::PathView&) const override
    {
        return false;
    }

    void SetAlias(const char*, const char*) override
    {
    }

    const char* GetAlias(const char*) const override
    {
        return nullptr;
    }

    void SetDeprecatedAlias(AZStd::string_view, AZStd::string_view) override
    {
    }

    void ClearAlias(const char* ) override { }

    AZStd::optional<AZ::u64> ConvertToAlias(char* inOutBuffer, AZ::u64) const override
    {
        return strlen(inOutBuffer);
    }

    bool ConvertToAlias(AZ::IO::FixedMaxPath& convertedPath, const AZ::IO::PathView& path) const override
    {
        convertedPath = path;
        return true;
    }

    bool GetFilename(AZ::IO::HandleType fileHandle, char* filename, AZ::u64 filenameSize) const override
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
        auto fileIt = m_openFiles.find(fileHandle);
        if (fileIt != m_openFiles.end())
        {
            azstrncpy(filename, filenameSize, fileIt->second.Name(), filenameSize);
            return true;
        }
        return false;
    }

    bool IsDirectory(const char*) override { return false; };
    AZ::IO::Result CreatePath(const char*) override { return AZ::IO::ResultCode::Error; };
    AZ::IO::Result Copy(const char*, const char*) override { return AZ::IO::ResultCode::Error; };
    AZ::IO::Result FindFiles(const char*, const char*, AZ::IO::FileIOBase::FindFilesCallbackType ) override { return AZ::IO::ResultCode::Error; };
};

class SetRestoreFileIOBaseRAII
{
public:
    SetRestoreFileIOBaseRAII(AZ::IO::FileIOBase& fileIO)
        : m_prevFileIO(AZ::IO::FileIOBase::GetInstance())
    {
        // FileIOBase will print an error if SetInstance() goes from non-null value to non-null value, but that's an intentional transition here,
        // so make sure to null it out first.
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(&fileIO);
    }

    ~SetRestoreFileIOBaseRAII()
    {
        // FileIOBase will print an error if SetInstance() goes from non-null value to non-null value, but that's an intentional transition here,
        // so make sure to null it out first.
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
    }
private:
    AZ::IO::FileIOBase* m_prevFileIO;
};
}
