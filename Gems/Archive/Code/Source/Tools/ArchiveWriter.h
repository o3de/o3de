/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Archive/ArchiveBaseAPI.h>
#include <Archive/ArchiveWriterAPI.h>

#include <AzCore/Memory/Memory_fwd.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/utility/to_underlying.h>

namespace AZ::IO
{
    class GenericStream;
}

namespace Archive
{
    class ArchiveWriter
    {
        using FileTokenTable = AZStd::unordered_map<ArchiveFileToken, ArchiveFileMetadata>;
        using FilePathTable = AZStd::unordered_map<AZ::IO::Path, ArchiveFileToken>;
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(ArchiveWriter)
            AZ_CLASS_ALLOCATOR_DECL;

        //! Unique pointer which wraps a stream
        //! where Archive data will be written.
        //! The Stream can be owned by the ArchiveWriter depending
        //! on the m_delete value
        struct ArchiveStreamDeleter
        {
            ArchiveStreamDeleter();
            ArchiveStreamDeleter(bool shouldDelete);
            void operator()(AZ::IO::GenericStream* ptr) const;
            bool m_delete{ true };
        };
        using ArchiveStreamPtr = AZStd::unique_ptr<AZ::IO::GenericStream, ArchiveStreamDeleter>;

        ArchiveWriter();
        explicit ArchiveWriter(const ArchiveWriterSettings& writerSettings);
        //! Open an file at the specified file path and takes sole ownership of it
        //! The ArchiveWriter will close the file on Unmount
        explicit ArchiveWriter(AZ::IO::PathView archivePath, const ArchiveWriterSettings& writerSettings = {});
        //! Takes ownership of the open stream and will optionally delete it based on the ArchiveFileDeleter
        explicit ArchiveWriter(ArchiveStreamPtr archiveStream, const ArchiveWriterSettings& writerSettings = {});
        ~ArchiveWriter();

        //! Opens the archive path and returns true if successful
        //! Will unmount any previously mounted archive
        bool MountArchive(AZ::IO::PathView archivePath);
        bool MountArchive(ArchiveStreamPtr archiveStream);

        //! Closes the handle to the archive file
        void UnmountArchive();

        //! Commits Archive current file to state to stream
        [[nodiscard]] bool Commit();

        //! Adds the content from the stream to the relative path
        //! based on the ArchiveWriterFileSettings
        ArchiveAddToFileResult AddFileToArchive(AZ::IO::GenericStream& inputStream,
            const ArchiveWriterFileSettings& fileSettings = {});

        //! Used the span contents to add the file to the archive
        ArchiveAddToFileResult AddFileToArchive(AZStd::span<const AZStd::byte> inputSpan,
            const ArchiveWriterFileSettings& fileSettings = {});

        //! Searches for a relative path within the archive
        //! @param relativePath Relative path within archive to search for
        //! @return A token that identifies the Archive file if it exist
        //! if the with the specified path doesn't exist InvalidArchiveFileToken is returned
        ArchiveFileToken FindFile(AZ::IO::PathView relativePath);
        //! Returns if the archive contains a relative path
        //! @param relativePath Relative path within archive to search for
        //! @returns true if the relative path is contained with the Archive
        //! equivalent to `return FindFile(relativePath) != InvalidArchiveFileToken;`
        bool ContainsFile(AZ::IO::PathView relativePath);

        //! Removes the file from the archive using the ArchiveFileToken
        //! @param filePathToken Relative path within archive to search for
        bool RemoveFileFromArchive(ArchiveFileToken filePathToken);
        //! Removes the file from the archive using a relative path name
        //! @param relativePath Relative path within archive to search for
        bool RemoveFileFromArchive(AZ::IO::PathView relativePath);

        //! Writes the file data about the archive to the supplied generic stream
        //! @param metadataStream with human readable data about files within the archive will be written to the stream
        //! @return true if metadata was successfully written
        bool WriteArchiveMetadata(AZ::IO::GenericStream& metadataStream,
            const ArchiveWriterMetadataSettings& metadataSettings = {});

    private:
        [[maybe_unused]] ArchiveWriterSettings m_archiveWriterSettings;
        //! Stores metadata about files added to the archive
        FileTokenTable m_tokenMap;
        FilePathTable m_pathMap;
        //! GenericStream pointer which stores the open archive
        ArchiveStreamPtr m_archiveStream;

        AZStd::atomic<AZStd::underlying_type_t<ArchiveFileToken>> m_tokenGenerator{
            AZStd::to_underlying(InvalidArchiveFileToken) + 1 };
    };

} // namespace Archive
