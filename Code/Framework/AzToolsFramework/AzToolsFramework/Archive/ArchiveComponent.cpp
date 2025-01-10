/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ArchiveComponent.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/Archive/INestedArchive.h>
#include <AzFramework/Archive/ZipDirStructures.h>
#include <AzFramework/Process/ProcessCommunicator.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzFramework/FileFunc/FileFunc.h>


namespace AzToolsFramework
{
    [[maybe_unused]] constexpr const char s_traceName[] = "ArchiveComponent";
    constexpr AZ::u32 s_compressionMethod = AZ::IO::INestedArchive::METHOD_DEFLATE;
    constexpr AZ::s32 s_compressionLevel = AZ::IO::INestedArchive::LEVEL_NORMAL;
    constexpr CompressionCodec::Codec s_compressionCodec = CompressionCodec::Codec::ZLIB;

    namespace ArchiveUtils
    {
        // Read a file's contents into a provided buffer.
        // Does not add a zero byte at the end of the buffer.
        // returns true if read was successful, false otherwise.
        bool ReadFile(const AZ::IO::Path& filePath, AZ::IO::OpenMode openMode, AZStd::vector<char>& outBuffer)
        {
            auto fileIO = AZ::IO::FileIOBase::GetDirectInstance();
            if (!fileIO)
            {
                return false;
            }

            bool success = false;
            AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
            if (fileIO->Open(filePath.c_str(), openMode, fileHandle))
            {
                AZ::u64 fileSize = 0;
                if (fileIO->Size(fileHandle, fileSize) && fileSize != 0)
                {
                    outBuffer.resize_no_construct(fileSize);

                    AZ::u64 bytesRead = 0;
                    if (fileIO->Read(fileHandle, outBuffer.data(), fileSize, true, &bytesRead))
                    {
                        success = (fileSize == bytesRead);
                    }
                }

                fileIO->Close(fileHandle);
            }

            return success;
        }

        // Reads a text file that contains a list of file paths.
        // Tokenize the file by lines.
        // Calls the lineVisitor function for each line of the file.
        void ProcessFileList(const AZ::IO::Path& filePath, AZStd::function<void(AZStd::string_view line)> lineVisitor)
        {
            AZStd::vector<char> fileBuffer;
            if (ReadFile(filePath, AZ::IO::OpenMode::ModeText | AZ::IO::OpenMode::ModeRead, fileBuffer))
            {
                AZ::StringFunc::TokenizeVisitor(AZStd::string_view{ fileBuffer.data(), fileBuffer.size() }, lineVisitor, "\n");
            }
        }

    } // namespace ArchiveUtils

    void ArchiveComponent::Activate()
    {
        m_fileIO = AZ::IO::FileIOBase::GetDirectInstance();
        if (m_fileIO == nullptr)
        {
            AZ_Error(s_traceName, false, "Failed to create a LocalFileIO instance!");
        }

        m_archive = AZ::Interface<AZ::IO::IArchive>::Get();
        if (m_archive == nullptr)
        {
            AZ_Error(s_traceName, false, "Failed to get IArchive interface!");
        }

        ArchiveCommandsBus::Handler::BusConnect();
    }

    void ArchiveComponent::Deactivate()
    {
        ArchiveCommandsBus::Handler::BusDisconnect();

        m_fileIO = nullptr;
        m_archive = nullptr;
    }

    void ArchiveComponent::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArchiveComponent, AZ::Component>()
                ->Version(2)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("AssetBuilder") }))
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ArchiveComponent>(
                    "Archive", "Handles creation and extraction of zip archives.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ;
            }
        }
    }

    std::future<bool> ArchiveComponent::CreateArchive(
        const AZStd::string& archivePath,
        const AZStd::string& dirToArchive)
    {
        if (!CheckParamsForCreate(archivePath, dirToArchive))
        {
            std::promise<bool> p;
            p.set_value(false);
            return p.get_future();
        }

        auto FnCreateArchive = [this](AZStd::string archivePath, AZStd::string dirToArchive) -> bool
        {
            auto archive = m_archive->OpenArchive(archivePath, {}, AZ::IO::INestedArchive::FLAGS_CREATE_NEW);
            if (!archive)
            {
                AZ_Error(s_traceName, false, "Failed to create archive file '%s'", archivePath.c_str());
                return false;
            }

            auto foundFiles = AzFramework::FileFunc::FindFilesInPath(dirToArchive, "*", true);
            if (!foundFiles.IsSuccess())
            {
                AZ_Error(s_traceName, false, "Failed to find file listing under directory '%d'", dirToArchive.c_str());
                return false;
            }

            bool success = true;
            AZStd::vector<char> fileBuffer;
            const AZ::IO::FixedMaxPath workingPath{ dirToArchive };

            for (const auto& fileName : foundFiles.GetValue())
            {
                bool thisSuccess = false;

                AZ::IO::FixedMaxPath relativePath = AZ::IO::FixedMaxPath{ fileName }.LexicallyRelative(workingPath);
                AZ::IO::FixedMaxPath fullPath = (workingPath / relativePath);

                if (ArchiveUtils::ReadFile(static_cast<AZ::IO::PathView>(fullPath), AZ::IO::OpenMode::ModeRead, fileBuffer))
                {
                    int result = archive->UpdateFile(
                        relativePath.Native(), fileBuffer.data(), fileBuffer.size(), s_compressionMethod,
                        s_compressionLevel, s_compressionCodec);

                    thisSuccess = (result == AZ::IO::ZipDir::ZD_ERROR_SUCCESS);
                    AZ_Error(
                        s_traceName, thisSuccess, "Error %d encountered while adding '%s' to archive '%.*s'", result, fileName.c_str(),
                        AZ_STRING_ARG(archive->GetFullPath().Native()));
                }
                else
                {
                    AZ_Error(
                        s_traceName, false, "Error encountered while reading '%s' to add to archive '%.*s'", fileName.c_str(),
                        AZ_STRING_ARG(archive->GetFullPath().Native()));
                }

                success = (success && thisSuccess);
            }

            archive.reset();
            return success;
        };

        return std::async(std::launch::async, FnCreateArchive, archivePath, dirToArchive);
    }


    std::future<bool> ArchiveComponent::ExtractArchive(
        const AZStd::string& archivePath,
        const AZStd::string& destinationPath)
    {
        if (!CheckParamsForExtract(archivePath, destinationPath))
        {
            std::promise<bool> p;
            p.set_value(false);
            return p.get_future();
        }

        auto FnExtractArchive = [this, archivePath, destinationPath]() -> bool
        {
            auto archive = m_archive->OpenArchive(archivePath, {}, AZ::IO::INestedArchive::FLAGS_READ_ONLY);
            if (!archive)
            {
                AZ_Error(s_traceName, false, "Failed to open archive file '%s'", archivePath.c_str());
                return false;
            }

            AZStd::vector<AZ::IO::Path> filesInArchive;
            if (int result = archive->ListAllFiles(filesInArchive); result != AZ::IO::ZipDir::ZD_ERROR_SUCCESS)
            {
                AZ_Error(s_traceName, false, "Failed to get list of files in archive '%s'", archivePath.c_str());
                return false;
            }

            AZStd::vector<AZ::u8> fileBuffer;
            AZ::IO::Path destination{ destinationPath };
            AZ::u64 fileSize = 0;
            AZ::u64 numFilesWritten = 0;
            AZ::u64 bytesWritten = 0;
            AZ::IO::INestedArchive::Handle srcHandle{};
            AZ::IO::HandleType dstHandle = AZ::IO::InvalidHandle;
            constexpr AZ::IO::OpenMode openMode =
                (AZ::IO::OpenMode::ModeCreatePath | AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeUpdate);

            for (const auto& filePath : filesInArchive)
            {
                srcHandle = archive->FindFile(filePath.Native());
                AZ_Assert(srcHandle != nullptr, "File '%s' does not exist inside archive '%s'", filePath.c_str(), archivePath.c_str());

                fileSize = (srcHandle != nullptr) ? archive->GetFileSize(srcHandle) : 0;
                fileBuffer.resize_no_construct(fileSize);
                if (auto result = archive->ReadFile(srcHandle, fileBuffer.data()); result != AZ::IO::ZipDir::ZD_ERROR_SUCCESS)
                {
                    AZ_Error(
                        s_traceName, false, "Failed to read file '%s' in archive '%s' with error %d", filePath.c_str(), archivePath.c_str(),
                        result);
                    continue;
                }

                AZ::IO::Path destinationFile = destination / filePath;
                if (!m_fileIO->Open(destinationFile.c_str(), openMode, dstHandle))
                {
                    AZ_Error(s_traceName, false, "Failed to open '%s' for writing", destinationFile.c_str());
                    continue;
                }

                if (!m_fileIO->Write(dstHandle, fileBuffer.data(), fileSize, &bytesWritten))
                {
                    AZ_Error(s_traceName, false, "Failed to write destination file '%s'", destinationFile.c_str());
                }
                else if (bytesWritten == fileSize)
                {
                    ++numFilesWritten;
                }

                m_fileIO->Close(dstHandle);
            }

            return (numFilesWritten == filesInArchive.size());
        };

        return std::async(std::launch::async, FnExtractArchive);
    }


    std::future<bool> ArchiveComponent::ExtractFile(
        const AZStd::string& archivePath,
        const AZStd::string& fileInArchive,
        const AZStd::string& destinationPath)
    {
        if (!CheckParamsForExtract(archivePath, destinationPath))
        {
            std::promise<bool> p;
            p.set_value(false);
            return p.get_future();
        }

        auto FnExtractFile = [this, archivePath, fileInArchive, destinationPath]() -> bool
        {
            auto archive = m_archive->OpenArchive(archivePath, {}, AZ::IO::INestedArchive::FLAGS_READ_ONLY);
            if (!archive)
            {
                AZ_Error(s_traceName, false, "Failed to open archive file '%s'", archivePath.c_str());
                return false;
            }

            AZ::IO::INestedArchive::Handle fileHandle = archive->FindFile(fileInArchive);
            if (!fileHandle)
            {
                AZ_Error(s_traceName, false, "File '%s' does not exist inside archive '%s'", fileInArchive.c_str(), archivePath.c_str());
                return false;
            }

            AZ::u64 fileSize = archive->GetFileSize(fileHandle);
            AZStd::vector<AZ::u8> fileBuffer;
            fileBuffer.resize_no_construct(fileSize);

            if (auto result = archive->ReadFile(fileHandle, fileBuffer.data()); result != AZ::IO::ZipDir::ZD_ERROR_SUCCESS)
            {
                AZ_Error(
                    s_traceName, false, "Failed to read file '%s' in archive '%s' with error %d", fileInArchive.c_str(),
                    archivePath.c_str(), result);
                return false;
            }

            AZ::IO::HandleType destFileHandle = AZ::IO::InvalidHandle;
            AZ::IO::Path destinationFile{ destinationPath };
            destinationFile /= fileInArchive;
            AZ::IO::OpenMode openMode = (AZ::IO::OpenMode::ModeCreatePath | AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeUpdate);
            if (!m_fileIO->Open(destinationFile.c_str(), openMode, destFileHandle))
            {
                AZ_Error(s_traceName, false, "Failed to open destination file '%s' for writing", destinationFile.c_str());
                return false;
            }

            AZ::u64 bytesWritten = 0;
            if (!m_fileIO->Write(destFileHandle, fileBuffer.data(), fileSize, &bytesWritten))
            {
                AZ_Error(s_traceName, false, "Failed to write destination file '%s'", destinationFile.c_str());
            }

            m_fileIO->Close(destFileHandle);
            return (bytesWritten == fileSize);
        };

        // Async task...
        return std::async(std::launch::async, FnExtractFile);
    }


    bool ArchiveComponent::ListFilesInArchive(const AZStd::string& archivePath, AZStd::vector<AZStd::string>& outFileEntries)
    {
        if (!m_fileIO || !m_archive)
        {
            return false;
        }

        if (!m_fileIO->Exists(archivePath.c_str()))
        {
            AZ_Error(s_traceName, false, "Archive '%s' does not exist!", archivePath.c_str());
            return false;
        }

        auto archive = m_archive->OpenArchive(archivePath, {}, AZ::IO::INestedArchive::FLAGS_READ_ONLY);
        if (!archive)
        {
            AZ_Error(s_traceName, false, "Failed to open archive file '%s'", archivePath.c_str());
            return false;
        }

        AZStd::vector<AZ::IO::Path> fileEntries;
        int result = archive->ListAllFiles(fileEntries);
        outFileEntries.clear();
        for (const auto& path : fileEntries)
        {
            outFileEntries.emplace_back(path.String());
        }
        return (result == AZ::IO::ZipDir::ZD_ERROR_SUCCESS);
    }


    std::future<bool> ArchiveComponent::AddFileToArchive(
        const AZStd::string& archivePath,
        const AZStd::string& workingDirectory,
        const AZStd::string& fileToAdd)
    {
        if (!CheckParamsForAdd(workingDirectory, fileToAdd))
        {
            std::promise<bool> p;
            p.set_value(false);
            return p.get_future();
        }

        auto FnAddFileToArchive = [this, archivePath, workingDirectory, fileToAdd]() -> bool
        {
            auto archive = m_archive->OpenArchive(archivePath);
            if (!archive)
            {
                AZ_Error(s_traceName, false, "Failed to open archive file '%s'", archivePath.c_str());
                return false;
            }

            AZ::IO::Path workingPath{ workingDirectory };
            AZ::IO::Path fullPath = workingPath / fileToAdd;
            AZ::IO::Path relativePath = fullPath.LexicallyRelative(workingPath);

            AZStd::vector<char> fileBuffer;
            bool success = false;
            if (ArchiveUtils::ReadFile(fullPath, AZ::IO::OpenMode::ModeRead, fileBuffer))
            {
                int result = archive->UpdateFile(
                    relativePath.c_str(), fileBuffer.data(), fileBuffer.size(), s_compressionMethod,
                    s_compressionLevel, s_compressionCodec);

                success = (result == AZ::IO::ZipDir::ZD_ERROR_SUCCESS);
                AZ_Error(
                    s_traceName, success, "Error %d encountered while adding '%s' to archive '%.*s'", result, fileToAdd.c_str(),
                    AZ_STRING_ARG(archive->GetFullPath().Native()));
            }
            else
            {
                AZ_Error(
                    s_traceName, false, "Error encountered while reading '%s' to add to archive '%.*s'", fileToAdd.c_str(),
                    AZ_STRING_ARG(archive->GetFullPath().Native()));
            }

            archive.reset();
            return success;
        };

        return std::async(std::launch::async, FnAddFileToArchive); 
    }


    std::future<bool> ArchiveComponent::AddFilesToArchive(
        const AZStd::string& archivePath,
        const AZStd::string& workingDirectory,
        const AZStd::string& listFilePath)
    {
        if (!CheckParamsForAdd(workingDirectory, listFilePath))
        {
            std::promise<bool> p;
            p.set_value(false);
            return p.get_future();
        }

        auto FnAddFilesToArchive = [this, archivePath, workingDirectory, listFilePath]() -> bool
        {
            auto archive = m_archive->OpenArchive(archivePath);
            if (!archive)
            {
                AZ_Error(s_traceName, false, "Failed to open archive file '%s'", archivePath.c_str());
                return false;
            }

            bool success = true; // starts true and turns false when any error is encountered.
            AZ::IO::Path basePath{ workingDirectory };

            auto PerLineCallback = [&success, &basePath, &archive](AZStd::string_view filePathLine) -> void
            {
                AZStd::vector<char> fileBuffer;
                AZ::IO::Path fullPath = (basePath / filePathLine);
                if (ArchiveUtils::ReadFile(fullPath, AZ::IO::OpenMode::ModeRead, fileBuffer))
                {
                    int result = archive->UpdateFile(
                        filePathLine, fileBuffer.data(), fileBuffer.size(), s_compressionMethod,
                        s_compressionLevel, s_compressionCodec);

                    bool thisSuccess = (result == AZ::IO::ZipDir::ZD_ERROR_SUCCESS);
                    success = (success && thisSuccess);
                    AZ_Error(
                        s_traceName, thisSuccess, "Error %d encountered while adding '%.*s' to archive '%.*s'", result,
                        AZ_STRING_ARG(filePathLine), AZ_STRING_ARG(archive->GetFullPath().Native()));
                }
                else
                {
                    AZ_Error(
                        s_traceName, false, "Error encountered while reading '%.*s' to add to archive '%.*s'", AZ_STRING_ARG(filePathLine),
                        AZ_STRING_ARG(archive->GetFullPath().Native()));
                }
            };

            ArchiveUtils::ProcessFileList(listFilePath, PerLineCallback);

            archive.reset();
            return success;
        };

        return std::async(std::launch::async, FnAddFilesToArchive); 
    }


    bool ArchiveComponent::CheckParamsForAdd(const AZStd::string& directory, const AZStd::string& file)
    {
        if (!m_fileIO || !m_archive)
        {
            return false;
        }

        if (!m_fileIO->IsDirectory(directory.c_str()))
        {
            AZ_Error(
                s_traceName, false, "Working directory '%s' is not a directory or doesn't exist!", directory.c_str());
            return false;
        }

        if (!file.empty())
        {
            auto filePath = AZ::IO::Path{ directory } / file;
            if (!m_fileIO->Exists(filePath.c_str()) || m_fileIO->IsDirectory(filePath.c_str()))
            {
                AZ_Error(s_traceName, false, "File list '%s' is a directory or doesn't exist!", filePath.c_str());
                return false;
            }
        }

        return true;
    }

    bool ArchiveComponent::CheckParamsForExtract(const AZStd::string& archive, const AZStd::string& directory)
    {
        if (!m_fileIO || !m_archive)
        {
            return false;
        }

        if (!m_fileIO->Exists(archive.c_str()))
        {
            AZ_Error(s_traceName, false, "Archive '%s' does not exist!", archive.c_str());
            return false;
        }

        if (!m_fileIO->Exists(directory.c_str()))
        {
            if (!m_fileIO->CreatePath(directory.c_str()))
            {
                AZ_Error(s_traceName, false, "Failed to create destination directory '%s'", directory.c_str());
                return false;
            }
        }

        return true;
    }

    bool ArchiveComponent::CheckParamsForCreate(const AZStd::string& archive, const AZStd::string& directory)
    {
        if (!m_fileIO || !m_archive)
        {
            return false;
        }

        if (m_fileIO->Exists(archive.c_str()))
        {
            AZ_Error(s_traceName, false, "Archive file '%s' already exists, cannot create a new archive there!");
            return false;
        }

        if (!m_fileIO->IsDirectory(directory.c_str()))
        {
            AZ_Error(s_traceName, false, "Directory '%s' is not a directory or doesn't exist!", directory.c_str());
            return false;
        }

        return true;
    }

} // namespace AzToolsFramework
