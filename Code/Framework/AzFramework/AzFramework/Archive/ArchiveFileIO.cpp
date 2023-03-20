/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/functional.h> // for function<> in the find files callback.
#include <AzFramework/Archive/ArchiveFileIO.h>
#include <AzFramework/Archive/IArchive.h>

#include <cinttypes>

namespace AZ::IO
{
    ArchiveFileIO::ArchiveFileIO(IArchive* archive)
        : m_archive(archive)
    {
    }

    ArchiveFileIO::~ArchiveFileIO()
    {
        // close all files.  This makes it mimic the behavior of base FileIO even though it sits on archive.
        decltype(m_trackedFiles) trackedFiles;
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_operationGuard);
            AZStd::swap(trackedFiles, m_trackedFiles);
        }

        for (const auto&[trackedFileHandle, trackedFile] : trackedFiles)
        {
            AZ_Warning("File IO", false, "File handle still open while ArchiveFileIO being closed: %s", trackedFile.c_str());
            Close(trackedFileHandle);
        }
    }

    void ArchiveFileIO::SetArchive(IArchive* archive)
    {
        m_archive = archive;
    }

    IArchive* ArchiveFileIO::GetArchive() const
    {
        return m_archive;
    }

    IO::Result ArchiveFileIO::Open(const char* filePath, IO::OpenMode openMode, IO::HandleType& fileHandle)
    {
        if (!m_archive)
        {
            if (GetDirectInstance())
            {
                return GetDirectInstance()->Open(filePath, openMode, fileHandle);
            }

            return IO::ResultCode::Error;
        }

        fileHandle = m_archive->FOpen(filePath, IO::GetStringModeFromOpenMode(openMode));
        if (fileHandle == IO::InvalidHandle)
        {
            return IO::ResultCode::Error;
        }

        //track the open file handles
        char resolvedPath[AZ::IO::MaxPathLength];
        bool pathResolved = ResolvePath(filePath, resolvedPath, AZStd::size(resolvedPath));
        AZStd::string_view trackedPath{ pathResolved ? resolvedPath : AZStd::string_view(filePath) };
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_operationGuard);
        m_trackedFiles.emplace(fileHandle, trackedPath);

        return IO::ResultCode::Success;
    }

    IO::Result ArchiveFileIO::Close(IO::HandleType fileHandle)
    {
        if (!m_archive)
        {
            if (GetDirectInstance())
            {
                return GetDirectInstance()->Close(fileHandle);
            }
            return IO::ResultCode::Error; //  we are likely shutting down and archive has already dropped all its handles already.
        }

        if (m_archive->FClose(fileHandle) == 0)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_operationGuard);

            auto remoteFileIter = m_trackedFiles.find(fileHandle);
            if (remoteFileIter != m_trackedFiles.end())
            {
                m_trackedFiles.erase(remoteFileIter);
            }

            return IO::ResultCode::Success;
        }
        return IO::ResultCode::Error;
    }

    IO::Result ArchiveFileIO::Tell(IO::HandleType fileHandle, AZ::u64& offset)
    {
        if (!m_archive)
        {
            if (GetDirectInstance())
            {
                return GetDirectInstance()->Tell(fileHandle, offset);
            }
            return IO::ResultCode::Error;
        }

        offset = m_archive->FTell(fileHandle);
        return IO::ResultCode::Success;
    }

    IO::Result ArchiveFileIO::Seek(IO::HandleType fileHandle, AZ::s64 offset, IO::SeekType type)
    {
        if (!m_archive)
        {
            if (GetDirectInstance())
            {
                return GetDirectInstance()->Seek(fileHandle, offset, type);
            }
            return IO::ResultCode::Error;
        }

        size_t seekResult = m_archive->FSeek(fileHandle, static_cast<uint64_t>(offset), GetFSeekModeFromSeekType(type));
        return seekResult == 0 ? IO::ResultCode::Success : IO::ResultCode::Error;
    }

    IO::Result ArchiveFileIO::Size(IO::HandleType fileHandle, AZ::u64& size)
    {
        if (!m_archive)
        {
            if (GetDirectInstance())
            {
                return GetDirectInstance()->Size(fileHandle, size);
            }
            return IO::ResultCode::Error;
        }

        size = m_archive->FGetSize(fileHandle);
        if (size || m_archive->IsInPak(fileHandle))
        {
            return IO::ResultCode::Success;
        }

        if (GetDirectInstance())
        {
            return GetDirectInstance()->Size(fileHandle, size);
        }

        return IO::ResultCode::Error;
    }

    IO::Result ArchiveFileIO::Size(const char* filePath, AZ::u64& size)
    {
        if (!m_archive)
        {
            if (GetDirectInstance())
            {
                return GetDirectInstance()->Size(filePath, size);
            }
            return IO::ResultCode::Error;
        }

        size = m_archive->FGetSize(filePath, true);
        if (!size)
        {
            return m_archive->IsFileExist(filePath, FileSearchLocation::Any) ? IO::ResultCode::Success : IO::ResultCode::Error;
        }

        return IO::ResultCode::Success;
    }

    IO::Result ArchiveFileIO::Read(IO::HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead, AZ::u64* bytesRead)
    {
        if (!m_archive)
        {
            if (GetDirectInstance())
            {
                return GetDirectInstance()->Read(fileHandle, buffer, size, failOnFewerThanSizeBytesRead, bytesRead);
            }

            return IO::ResultCode::Error;
        }

        size_t result = m_archive->FRead(buffer, size, fileHandle);
        if (bytesRead)
        {
            *bytesRead = static_cast<AZ::u64>(result);
        }

        if (failOnFewerThanSizeBytesRead)
        {
            return result != size ? IO::ResultCode::Error : IO::ResultCode::Success;
        }
        return result == 0 ? IO::ResultCode::Error : IO::ResultCode::Success;
    }

    IO::Result ArchiveFileIO::Write(IO::HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten)
    {
        if (!m_archive)
        {
            if (GetDirectInstance())
            {
                return GetDirectInstance()->Write(fileHandle, buffer, size, bytesWritten);
            }

            return IO::ResultCode::Error;
        }

        size_t result = m_archive->FWrite(buffer, size, fileHandle);
        if (bytesWritten)
        {
            *bytesWritten = static_cast<AZ::u64>(result);
        }

        return result != size ? IO::ResultCode::Error : IO::ResultCode::Success;
    }

    IO::Result ArchiveFileIO::Flush(IO::HandleType fileHandle)
    {
        if (!m_archive)
        {
            if (GetDirectInstance())
            {
                return GetDirectInstance()->Flush(fileHandle);
            }
            return IO::ResultCode::Error;
        }

        return m_archive->FFlush(fileHandle) == 0 ? IO::ResultCode::Success : IO::ResultCode::Error;
    }

    bool ArchiveFileIO::Eof(IO::HandleType fileHandle)
    {
        if (!m_archive)
        {
            if (GetDirectInstance())
            {
                return GetDirectInstance()->Eof(fileHandle);
            }
            return false;
        }

        return (m_archive->FEof(fileHandle) != 0);
    }

    bool ArchiveFileIO::Exists(const char* filePath)
    {
        if (!m_archive)
        {
            if (GetDirectInstance())
            {
                return GetDirectInstance()->Exists(filePath);
            }
            return false;
        }

        return m_archive->IsFileExist(filePath);
    }

    AZ::u64 ArchiveFileIO::ModificationTime(const char* filePath)
    {
        IO::HandleType openFile = IO::InvalidHandle;
        if (!Open(filePath, IO::OpenMode::ModeRead | IO::OpenMode::ModeBinary, openFile))
        {
            return 0;
        }

        AZ::u64 result = ModificationTime(openFile);

        Close(openFile);

        return result;
    }

    AZ::u64 ArchiveFileIO::ModificationTime(IO::HandleType fileHandle)
    {
        if (!m_archive)
        {
            if (GetDirectInstance())
            {
                return GetDirectInstance()->ModificationTime(fileHandle);
            }
            return 0;
        }

        return m_archive->GetModificationTime(fileHandle);
    }

    bool ArchiveFileIO::IsDirectory(const char* filePath)
    {
        if (!m_archive)
        {
            if (GetDirectInstance())
            {
                return GetDirectInstance()->IsDirectory(filePath);
            }
            return false;
        }

        return m_archive->IsFolder(filePath);
    }

    IO::Result ArchiveFileIO::CreatePath(const char* filePath)
    {
        // since you can't create a path inside a pak file
        // we will pass this to the underlying real fileio!
        FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
        if (!realUnderlyingFileIO)
        {
            return IO::ResultCode::Error;
        }

        return realUnderlyingFileIO->CreatePath(filePath);
    }

    IO::Result ArchiveFileIO::DestroyPath(const char* filePath)
    {
        // since you can't destroy a path inside a pak file
        // we will pass this to the underlying real fileio
        FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
        if (!realUnderlyingFileIO)
        {
            return IO::ResultCode::Error;
        }

        return realUnderlyingFileIO->DestroyPath(filePath);
    }

    IO::Result ArchiveFileIO::Remove(const char* filePath)
    {
        if (!m_archive)
        {
            if (GetDirectInstance())
            {
                return GetDirectInstance()->Remove(filePath);
            }
            return IO::ResultCode::Error;
        }

        return (m_archive->RemoveFile(filePath) ? IO::ResultCode::Success : IO::ResultCode::Error);
    }

    IO::Result ArchiveFileIO::Copy(const char* sourceFilePath, const char* destinationFilePath)
    {
        // you can actually copy a file from inside a pak to a destination path if you want to...
        IO::HandleType sourceFile = IO::InvalidHandle;
        IO::HandleType destinationFile = IO::InvalidHandle;
        if (!Open(sourceFilePath, IO::OpenMode::ModeRead | IO::OpenMode::ModeBinary, sourceFile))
        {
            return IO::ResultCode::Error;
        }

        IO::Path destPath(IO::PathView(destinationFilePath).ParentPath());

        CreatePath(destPath.c_str());

        if (!Open(destinationFilePath, IO::OpenMode::ModeWrite | IO::OpenMode::ModeBinary, destinationFile))
        {
            Close(sourceFile);
            return IO::ResultCode::Error;
        }

        // standard buffered copy.
        bool failureEncountered = false;
        AZ::u64 bytesRemaining = 0;

        if (!Size(sourceFilePath, bytesRemaining))
        {
            Close(destinationFile);
            Close(sourceFile);
            return IO::ResultCode::Error;
        }

        while (bytesRemaining > 0)
        {
            size_t bytesThisTime = AZStd::GetMin<size_t>(bytesRemaining, ArchiveFileIoMaxBuffersize);

            if (!Read(sourceFile, m_copyBuffer.data(), bytesThisTime, true))
            {
                failureEncountered = true;
                break;
            }

            AZ::u64 actualBytesWritten = 0;

            if ((!Write(destinationFile, m_copyBuffer.data(), bytesThisTime, &actualBytesWritten)) || (actualBytesWritten == 0))
            {
                failureEncountered = true;
                break;
            }
            bytesRemaining -= actualBytesWritten;
        }

        Close(sourceFile);
        Close(destinationFile);
        return (failureEncountered || (bytesRemaining > 0)) ? IO::ResultCode::Error : IO::ResultCode::Success;
    }


    bool ArchiveFileIO::IsReadOnly(const char* filePath)
    {
        // a tricky one!  files inside a pack are technically readonly...
        IO::HandleType openedHandle = IO::InvalidHandle;

        if (!Open(filePath, IO::OpenMode::ModeRead | IO::OpenMode::ModeBinary, openedHandle))
        {
            return false; // this will also return false if there is no archive, so no need to check it again
        }

        bool inPak = m_archive->IsInPak(openedHandle);
        Close(openedHandle);

        if (inPak)
        {
            return true; // things inside packfiles are read only by default since you cannot modify them while pak is mounted.
        }

        FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
        if (!realUnderlyingFileIO)
        {
            return false;
        }

        return realUnderlyingFileIO->IsReadOnly(filePath);
    }

    IO::Result ArchiveFileIO::Rename(const char* sourceFilePath, const char* destinationFilePath)
    {
        // since you cannot perform this opearation inside a pak file, we do it on the real file
        FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
        if (!realUnderlyingFileIO)
        {
            return IO::ResultCode::Error;
        }

        return realUnderlyingFileIO->Rename(sourceFilePath, destinationFilePath);
    }

    IO::Result ArchiveFileIO::FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback)
    {
        // note that the underlying findFiles takes both path and filter.
        if (!filePath)
        {
            return IO::ResultCode::Error;
        }

        if (!m_archive)
        {
            if (GetDirectInstance())
            {
                return GetDirectInstance()->FindFiles(filePath, filter, callback);
            }
            return IO::ResultCode::Error;
        }

        AZ::IO::FixedMaxPath total = filePath;
        if (total.empty())
        {
            return IO::ResultCode::Error;
        }

        total /= filter;

        AZ::IO::ArchiveFileIterator fileIterator = m_archive->FindFirst(total.c_str());
        if (!fileIterator)
        {
            return IO::ResultCode::Success; // its not an actual fatal error to not find anything.
        }
        for (; fileIterator; fileIterator = m_archive->FindNext(fileIterator))
        {
            total = filePath;
            total /= fileIterator.m_filename;
            if (ConvertToAlias(total, total))
            {
                if (!callback(total.c_str()))
                {
                    break;
                }
            }
        }

        m_archive->FindClose(fileIterator);

        return IO::ResultCode::Success;
    }

    bool ArchiveFileIO::GetFilename(IO::HandleType fileHandle, char* filename, AZ::u64 filenameSize) const
    {
        // because we sit on archive we need to keep track of archive files too.
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_operationGuard);
        const auto fileIt = m_trackedFiles.find(fileHandle);
        if (fileIt != m_trackedFiles.end())
        {
            const AZStd::string_view trackedFileView = fileIt->second.Native();
            if (filenameSize <= trackedFileView.size())
            {
                return false;
            }
            size_t trackedFileViewLength = trackedFileView.copy(filename, trackedFileView.size());
            filename[trackedFileViewLength] = '\0';
            return true;
        }

        if (GetDirectInstance())
        {
            return GetDirectInstance()->GetFilename(fileHandle, filename, filenameSize);
        }
        return false;
    }

    // the rest of these functions just pipe through to the underlying real file IO, since
    // archive doesn't know about any of this.
    void ArchiveFileIO::SetAlias(const char* alias, const char* path)
    {
        FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
        if (!realUnderlyingFileIO)
        {
            return;
        }
        realUnderlyingFileIO->SetAlias(alias, path);
    }

    const char* ArchiveFileIO::GetAlias(const char* alias) const
    {
        FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
        if (!realUnderlyingFileIO)
        {
            return nullptr;
        }
        return realUnderlyingFileIO->GetAlias(alias);
    }

    void ArchiveFileIO::ClearAlias(const char* alias)
    {
        FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
        if (!realUnderlyingFileIO)
        {
            return;
        }
        realUnderlyingFileIO->GetAlias(alias);
    }

    void ArchiveFileIO::SetDeprecatedAlias(AZStd::string_view oldAlias, AZStd::string_view newAlias)
    {
        FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
        if (!realUnderlyingFileIO)
        {
            return;
        }
        realUnderlyingFileIO->SetDeprecatedAlias(oldAlias, newAlias);
    }

    AZStd::optional<AZ::u64> ArchiveFileIO::ConvertToAlias(char* inOutBuffer, AZ::u64 bufferLength) const
    {
        if ((!inOutBuffer) || (bufferLength == 0))
        {
            return 0;
        }
        FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
        if (!realUnderlyingFileIO)
        {
            inOutBuffer[0] = 0;
            return AZStd::nullopt;
        }
        return realUnderlyingFileIO->ConvertToAlias(inOutBuffer, bufferLength);
    }

    bool ArchiveFileIO::ConvertToAlias(AZ::IO::FixedMaxPath& convertedPath, const AZ::IO::PathView& path) const
    {
        FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
        if (!realUnderlyingFileIO)
        {
            return false;
        }

        return realUnderlyingFileIO->ConvertToAlias(convertedPath, path);
    }

    bool ArchiveFileIO::ResolvePath(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize) const
    {
        FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
        if (!realUnderlyingFileIO)
        {
            return false;
        }
        return realUnderlyingFileIO->ResolvePath(path, resolvedPath, resolvedPathSize);
    }

    bool ArchiveFileIO::ResolvePath(AZ::IO::FixedMaxPath& resolvedPath, const AZ::IO::PathView& path) const
    {
        FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
        if (!realUnderlyingFileIO)
        {
            return false;
        }
        return realUnderlyingFileIO->ResolvePath(resolvedPath, path);
    }

    bool ArchiveFileIO::ReplaceAlias(AZ::IO::FixedMaxPath& replacedAliasPath, const AZ::IO::PathView& path) const
    {
        FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
        if (!realUnderlyingFileIO)
        {
            return false;
        }
        return realUnderlyingFileIO->ReplaceAlias(replacedAliasPath, path);
    }
}//namespace AZ:IO
