/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

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

    class FindData;
    struct ArchiveFileIterator
    {
        ArchiveFileIterator() = default;
        ArchiveFileIterator(FindData* findData, AZStd::string_view filename, const FileDesc& fileDesc);

        ArchiveFileIterator operator++();
        ArchiveFileIterator operator++(int);

        explicit operator bool() const;

        inline static constexpr size_t FilenameMaxLength = 256;
        AZStd::fixed_string<FilenameMaxLength> m_filename;
        FileDesc m_fileDesc;
        AZStd::intrusive_ptr<FindData> m_findData{};

    private:
        friend class FindData;
        bool m_lastFetchValid{};
    };

    struct AZStdStringLessCaseInsensitive
    {
        bool operator()(AZStd::string_view left, AZStd::string_view right) const;

        using is_transparent = void;
    };
    class FindData
        : public AZStd::intrusive_base
    {
    public:
        AZ_CLASS_ALLOCATOR(FindData, AZ::SystemAllocator, 0);
        FindData() = default;
        AZ::IO::ArchiveFileIterator Fetch();
        void Scan(IArchive* archive, AZStd::string_view path, bool bAllowUseFS = false, bool bScanZips = true);

    protected:
        void ScanFS(IArchive* archive, AZStd::string_view path);
        void ScanZips(IArchive* archive, AZStd::string_view path);

        using FileStack = AZStd::vector<ArchiveFileIterator>;
        FileStack m_fileStack;
    };
}
