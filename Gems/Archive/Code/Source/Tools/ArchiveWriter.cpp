/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ArchiveWriter.h"

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/OpenMode.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/conversions.h>

#include <Compression/CompressionInterfaceAPI.h>

namespace Archive
{
    // Implement TypeInfo, Rtti and Allocator support
    AZ_TYPE_INFO_WITH_NAME_IMPL(ArchiveWriter, "ArchiveWriter", "{DACA9F90-C8D5-41CB-8400-F0B39BFC4A28}");
    AZ_CLASS_ALLOCATOR_IMPL(ArchiveWriter, AZ::SystemAllocator);

    // ArchiveStreamDeleter for the GenericStream unique_ptr
    ArchiveWriter::ArchiveStreamDeleter::ArchiveStreamDeleter() = default;
    ArchiveWriter::ArchiveStreamDeleter::ArchiveStreamDeleter(bool shouldDelete)
        : m_delete(shouldDelete)
    {}

    void ArchiveWriter::ArchiveStreamDeleter::operator()(AZ::IO::GenericStream* ptr) const
    {
        if (m_delete && ptr)
        {
            delete ptr;
        }
    }

    ArchiveWriter::ArchiveWriter() = default;

    ArchiveWriter::~ArchiveWriter()
    {
        UnmountArchive();
    }

    ArchiveWriter::ArchiveWriter(const ArchiveWriterSettings& writerSettings)
        : m_archiveWriterSettings(writerSettings)
    {}

    ArchiveWriter::ArchiveWriter(AZ::IO::PathView archivePath, const ArchiveWriterSettings& writerSettings)
        : m_archiveWriterSettings(writerSettings)
    {
        MountArchive(archivePath);
    }

    ArchiveWriter::ArchiveWriter(ArchiveStreamPtr archiveStream, const ArchiveWriterSettings& writerSettings)
        : m_archiveWriterSettings(writerSettings)
    {
        MountArchive(AZStd::move(archiveStream));
    }

    bool ArchiveWriter::MountArchive(AZ::IO::PathView archivePath)
    {
        UnmountArchive();
        AZ::IO::FixedMaxPath mountPath{ archivePath };
        constexpr AZ::IO::OpenMode openMode =
            AZ::IO::OpenMode::ModeCreatePath
            | AZ::IO::OpenMode::ModeWrite;

        m_archiveStream.reset(aznew AZ::IO::SystemFileStream(mountPath.c_str(), openMode));

        return m_archiveStream->IsOpen();
    }

    bool ArchiveWriter::MountArchive(ArchiveStreamPtr archiveStream)
    {
        UnmountArchive();
        m_archiveStream = AZStd::move(archiveStream);

        return m_archiveStream != nullptr && m_archiveStream->IsOpen();
    }

    void ArchiveWriter::UnmountArchive()
    {
        if (m_archiveStream->IsOpen())
        {
            m_archiveStream->Close();
        }
    }

    bool ArchiveWriter::Commit()
    {
        return true;
    }

    ArchiveAddToFileResult ArchiveWriter::AddFileToArchive(AZ::IO::GenericStream& inputStream,
        const ArchiveWriterFileSettings& fileSettings)
    {
        AZStd::vector<AZStd::byte> fileData;
        fileData.resize_no_construct(inputStream.GetLength());
        AZ::IO::SizeType bytesRead = inputStream.Read(fileData.size(), fileData.data());
        // Unable to read entire stream data into memory
        if (bytesRead != fileData.size())
        {
            ArchiveAddToFileResult errorResult;
            errorResult.m_relativeFilePath = fileSettings.m_relativeFilePath;
            errorResult.m_compressionAlgorithm = fileSettings.m_compressionAlgorithm;
            errorResult.m_resultOutcome = AZStd::unexpected(
                ResultString::format(R"(Unable to successfully read all bytes(%zu) from input stream.)"
                    " %llu bytes were read.",
                    fileData.size(), bytesRead));
            return errorResult;
        }

        return AddFileToArchive(fileData, fileSettings);
    }

    ArchiveAddToFileResult ArchiveWriter::AddFileToArchive(AZStd::span<const AZStd::byte> inputSpan,
        const ArchiveWriterFileSettings& fileSettings)
    {
        // Update the file case based on the ArchiveFilePathCase enum
        AZ::IO::Path filePath = fileSettings.m_relativeFilePath;
        switch (fileSettings.m_fileCase)
        {
        case ArchiveFilePathCase::Lowercase:
            AZStd::to_lower(filePath.Native());
            break;
        case ArchiveFilePathCase::Uppercase:
            AZStd::to_upper(filePath.Native());
            break;
        }

        // Check if a file being added is already in the archive
        // If it is return an ArchiveAddToFileResult with an invalid file token
        if (fileSettings.m_fileMode == ArchiveWriterFileMode::AddNew
            && ContainsFile(filePath))
        {
            ArchiveAddToFileResult errorResult;
            errorResult.m_relativeFilePath = fileSettings.m_relativeFilePath;
            errorResult.m_compressionAlgorithm = fileSettings.m_compressionAlgorithm;
            errorResult.m_resultOutcome = AZStd::unexpected(
                ResultString::format(R"(The file with relative path "%s" already exist in the archive.)"
                    " The FileMode::AddNew option was specified.",
                    filePath.c_str()));
            return errorResult;
        }

        ArchiveAddToFileResult result;
        result.m_relativeFilePath = fileSettings.m_relativeFilePath;

        // Storage buffer used to store the file data if it s compressed
        // It's lifetime must outlive the writeDataSpan
        AZStd::vector<AZStd::byte> compressionBuffer;

        // Span which points to the data that will be written to the archive stream
        auto writeDataSpan = inputSpan;

        if (fileSettings.m_compressionAlgorithm != Compression::Invalid
            && fileSettings.m_compressionAlgorithm != Compression::Uncompressed)
        {
            if (auto compressionRegistrar = Compression::CompressionRegistrar::Get();
                compressionRegistrar != nullptr)
            {
                if (Compression::ICompressionInterface* compressionInterface =
                    compressionRegistrar->FindCompressionInterface(fileSettings.m_compressionAlgorithm);
                    compressionInterface != nullptr)
                {
                    // Determine the maximum size of the compression buffer
                    const size_t compressedSizeBound = compressionInterface->CompressBound(inputSpan.size());
                    compressionBuffer.resize_no_construct(compressedSizeBound);

                    // Run the input data through the compressor
                    if (Compression::CompressionResultData compressionResultData = compressionInterface->CompressBlock(compressionBuffer, inputSpan,
                        fileSettings.m_compressionOptions != nullptr
                        ? *fileSettings.m_compressionOptions
                        : Compression::CompressionOptions{});
                        compressionResultData)
                    {
                        // Update the writeDataSpan to point to the compressed buffer span
                        writeDataSpan = compressionResultData.m_compressedBuffer;
                        result.m_compressionAlgorithm = fileSettings.m_compressionAlgorithm;
                    }
                }
            }
        }

        // Generate a file token
        auto archiveFileToken(static_cast<ArchiveFileToken>(m_tokenGenerator++));
        auto emplaceIt = m_pathMap.emplace(AZStd::move(filePath), archiveFileToken);
        // Retrieve a reference to the path map AZ::IO::Path
        // This reference will be used to initialize the ArchiveFileMetadata PathView member
        const AZ::IO::Path& pathReference = emplaceIt.first->first;

        // Archive File Metadata
        ArchiveFileMetadata fileMetadata;
        fileMetadata.m_uncompressedSize = inputSpan.size();
        fileMetadata.m_compressedSize = writeDataSpan.size();
        fileMetadata.m_compressionAlgorithm = result.m_compressionAlgorithm;
        fileMetadata.m_filePath = pathReference;
        fileMetadata.m_offset = 0;
        m_tokenMap.emplace(archiveFileToken, AZStd::move(fileMetadata));

        result.m_filePathToken = archiveFileToken;

        return result;
    }

    ArchiveFileToken ArchiveWriter::FindFile(AZ::IO::PathView relativePath)
    {
        auto foundIt = m_pathMap.find(relativePath);
        return foundIt != m_pathMap.end() ? foundIt->second : InvalidArchiveFileToken;
    }

    bool ArchiveWriter::ContainsFile(AZ::IO::PathView relativePath)
    {
        return m_pathMap.contains(relativePath);
    }

    bool ArchiveWriter::RemoveFileFromArchive(ArchiveFileToken filePathToken)
    {
        if (auto tokenIt = m_tokenMap.find(filePathToken);
            tokenIt != m_tokenMap.end())
        {
            const ArchiveFileMetadata& fileMetadata = tokenIt->second;

            // Remove the file path -> file token entry
            size_t erasedPathCount = m_pathMap.erase(fileMetadata.m_filePath);

            // Now erase the file token -> file metadata entry
            m_tokenMap.erase(tokenIt);

            return erasedPathCount != 0;
        }

        return false;
    }

    bool ArchiveWriter::RemoveFileFromArchive(AZ::IO::PathView relativePath)
    {
        if (auto pathIt = m_pathMap.find(relativePath);
            pathIt != m_pathMap.end())
        {
            ArchiveFileToken fileToken = pathIt->second;

            // Remove the file token -> file metadata entry
            size_t erasedTokenCount = m_tokenMap.erase(fileToken);

            // Now the file path key can be removed from the path map
            m_pathMap.erase(pathIt);

            return erasedTokenCount != 0;
        }

        return false;
    }

    bool ArchiveWriter::WriteArchiveMetadata([[maybe_unused]] AZ::IO::GenericStream& metadataStream,
        [[maybe_unused]] const ArchiveWriterMetadataSettings& metadataSettings)
    {
        return false;
    }

} // namespace Archive
