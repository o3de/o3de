/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileReader.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>

namespace AZ::IO
{
    FileReader::FileReader() = default;

    FileReader::FileReader(AZ::IO::FileIOBase* fileIoBase, const char* filePath)
    {
        Open(fileIoBase, filePath);
    }

    FileReader::~FileReader()
    {
        Close();
    }

    FileReader::FileReader(FileReader&& other)
    {
        AZStd::swap(m_file, other.m_file);
        AZStd::swap(m_fileIoBase, other.m_fileIoBase);
    }

    FileReader& FileReader::operator=(FileReader&& other)
    {
        // Close the current file and take over other file
        Close();
        m_file = AZStd::move(other.m_file);
        m_fileIoBase = AZStd::move(other.m_fileIoBase);
        other.m_file = AZStd::monostate{};
        other.m_fileIoBase = {};

        return *this;
    }

    FileReader FileReader::GetStdin()
    {
        FileReader fileReader;
        fileReader.m_file.emplace<AZ::IO::SystemFile>(AZ::IO::SystemFile::GetStdin());
        return fileReader;
    }

    bool FileReader::Open(AZ::IO::FileIOBase* fileIoBase, const char* filePath)
    {
        // Close file if the FileReader has an instance open
        Close();

        if (fileIoBase != nullptr)
        {
            AZ::IO::HandleType fileHandle;
            if (fileIoBase->Open(filePath, IO::OpenMode::ModeRead, fileHandle))
            {
                m_file = fileHandle;
                m_fileIoBase = fileIoBase;
                return true;
            }
        }
        else
        {
            AZ::IO::SystemFile file;
            if (file.Open(filePath, IO::SystemFile::OpenMode::SF_OPEN_READ_ONLY))
            {
                m_file = AZStd::move(file);
                return true;
            }
        }

        return false;
    }

    bool FileReader::IsOpen() const
    {
        if (auto fileHandle = AZStd::get_if<AZ::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            return *fileHandle != AZ::IO::InvalidHandle;
        }
        else if (auto systemFile = AZStd::get_if<AZ::IO::SystemFile>(&m_file); systemFile != nullptr)
        {
            return systemFile->IsOpen();
        }

        return false;
    }

    void FileReader::Close()
    {
        if (auto fileHandle = AZStd::get_if<AZ::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            if (AZ::IO::FileIOBase* fileIo = m_fileIoBase; fileIo != nullptr)
            {
                fileIo->Close(*fileHandle);
            }
        }

        m_file = AZStd::monostate{};
        m_fileIoBase = {};
    }

    auto FileReader::Length() const -> SizeType
    {
        if (auto fileHandle = AZStd::get_if<AZ::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            if (SizeType fileSize{}; m_fileIoBase->Size(*fileHandle, fileSize))
            {
                return fileSize;
            }
        }
        else if (auto systemFile = AZStd::get_if<AZ::IO::SystemFile>(&m_file); systemFile != nullptr)
        {
            return systemFile->Length();
        }

        return 0;
    }

    auto FileReader::Read(SizeType byteSize, void* buffer) -> SizeType
    {
        if (auto fileHandle = AZStd::get_if<AZ::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            if (SizeType bytesRead{}; m_fileIoBase->Read(*fileHandle, buffer, byteSize, false, &bytesRead))
            {
                return bytesRead;
            }
        }
        else if (auto systemFile = AZStd::get_if<AZ::IO::SystemFile>(&m_file); systemFile != nullptr)
        {
            return systemFile->Read(byteSize, buffer);
        }

        return 0;
    }

    auto FileReader::Tell() const -> SizeType
    {
        if (auto fileHandle = AZStd::get_if<AZ::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            if (SizeType fileOffset{}; m_fileIoBase->Tell(*fileHandle, fileOffset))
            {
                return fileOffset;
            }
        }
        else if (auto systemFile = AZStd::get_if<AZ::IO::SystemFile>(&m_file); systemFile != nullptr)
        {
            return systemFile->Tell();
        }

        return 0;
    }

    bool FileReader::Seek(AZ::s64 offset, SeekType type)
    {
        if (auto fileHandle = AZStd::get_if<AZ::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            return m_fileIoBase->Seek(*fileHandle, offset, type);
        }
        else if (auto systemFile = AZStd::get_if<AZ::IO::SystemFile>(&m_file); systemFile != nullptr)
        {
            systemFile->Seek(offset, static_cast<AZ::IO::SystemFile::SeekMode>(type));
            return true;
        }

        return false;
    }

    bool FileReader::Eof() const
    {
        if (auto fileHandle = AZStd::get_if<AZ::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            return m_fileIoBase->Eof(*fileHandle);
        }
        else if (auto systemFile = AZStd::get_if<AZ::IO::SystemFile>(&m_file); systemFile != nullptr)
        {
            return systemFile->Eof();
        }

        return false;
    }

    bool FileReader::GetFilePath(AZ::IO::FixedMaxPath& filePath) const
    {
        if (auto fileHandle = AZStd::get_if<AZ::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            AZ::IO::FixedMaxPathString& pathStringRef = filePath.Native();
            if (m_fileIoBase->GetFilename(*fileHandle, pathStringRef.data(), pathStringRef.capacity()))
            {
                pathStringRef.resize_no_construct(AZStd::char_traits<char>::length(pathStringRef.data()));
                return true;
            }
        }
        else if (auto systemFile = AZStd::get_if<AZ::IO::SystemFile>(&m_file); systemFile != nullptr)
        {
            filePath = systemFile->Name();
            return true;
        }

        return false;
    }
}
