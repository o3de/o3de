/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/std/string/fixed_string.h>

namespace AZ::IO
{
    struct IArchive;

    struct FileDesc
    {
        enum class Attribute : uint32_t
        {
            ReadOnly = 0x1,
            Subdirectory = 0x10,
            Archive = 0x80000000
        };
        Attribute nAttrib{};
        uint64_t nSize{};
        time_t tAccess{ -1 };
        time_t tCreate{ -1 };
        time_t tWrite{ -1 };

        FileDesc() = default;
        explicit FileDesc(Attribute fileAttribute, uint64_t fileSize = 0, time_t accessTime = -1, time_t creationTime = -1, time_t writeTime = -1);
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::IO::FileDesc::Attribute);

    inline constexpr size_t ArchiveFilenameMaxLength = 256;
    using ArchiveFileString = AZStd::fixed_string<ArchiveFilenameMaxLength>;

    class FindData;
    //! This is not really an iterator, but a handle
    //! that extends ownership of any found filenames from an archive file or the file system
    struct ArchiveFileIterator
    {
        ArchiveFileIterator() = default;
        explicit ArchiveFileIterator(FindData* findData);

        ArchiveFileIterator operator++();
        ArchiveFileIterator operator++(int);

        explicit operator bool() const;

        ArchiveFileString m_filename;
        FileDesc m_fileDesc;

    private:
        friend class FindData;
        friend class Archive;
        AZStd::intrusive_ptr<FindData> m_findData;
        bool m_lastFetchValid{};
    };


    class FindData
        : public AZStd::intrusive_base
    {
    public:
        AZ_CLASS_ALLOCATOR(FindData, AZ::SystemAllocator, 0);
        FindData() = default;
        ArchiveFileIterator Fetch();
        void Scan(IArchive* archive, AZStd::string_view path, bool bAllowUseFS = false, bool bScanZips = true);

    protected:
        void ScanFS(IArchive* archive, AZStd::string_view path);
        // Populates the FileSet with files within the that match the path pattern that is
        // if it refers to a file within a bound archive root or returns the archive root
        // path if the path pattern matches it.
        void ScanZips(IArchive* archive, AZStd::string_view path);

        class ArchiveFile
        {
        public:
            friend class FindData;

            ArchiveFile();
            ArchiveFile(AZStd::string_view filename, const FileDesc& fileDesc);

            size_t GetHash() const;
            bool operator==(const ArchiveFile& rhs) const;

        private:
            ArchiveFileString m_filename;
            FileDesc m_fileDesc;
        };

        struct ArchiveFileHash
        {
            size_t operator()(const ArchiveFile& archiveFile) const;
        };

        using FileSet = AZStd::unordered_set<ArchiveFile, ArchiveFileHash>;
        FileSet m_fileSet;
    };

}
