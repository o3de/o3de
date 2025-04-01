/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ::IO::ZipDir
{
    // this is the array of file entries that's convenient to use to construct CDR
    struct FileRecord
    {
        AZStd::string strPath; // relative path to the file inside zip
        FileEntryBase* pFileEntryBase{}; // the file entry itself
    };

    struct FileDataRecord
        : public FileRecord
        , public AZStd::intrusive_base
    {
        AZ_CLASS_ALLOCATOR(FileDataRecord, AZ::SystemAllocator);
        FileDataRecord(const FileRecord& rThat);

        void* GetData() {return m_data.get(); }
    private:
        struct DataDeleter
        {
            void operator()(void* ptr) const
            {
                azfree(ptr);
            }
        };
        AZStd::unique_ptr<AZStd::byte[], DataDeleter> m_data;
    };

    using FileDataRecordPtr = AZStd::intrusive_ptr<FileDataRecord>;

    // this is used for construction of CDR
    class FileRecordList
        : public AZStd::vector<FileRecord>
    {
    public:
        FileRecordList(class FileEntryTree* pTree);

        struct ZipStats
        {
            // the size of the CDR in the file
            size_t nSizeCDR;
            // the size of the file data part (local file descriptors and file datas)
            // if it's compacted
            size_t nSizeCompactData;
        };

        // sorts the files by the physical offset in the zip file
        void SortByFileOffset ();

        // returns the size of CDR in the zip file
        ZipStats GetStats() const;

        // puts the CDR into the given block of mem
        size_t MakeZipCDR(uint32_t lCDROffset, void* p) const;

        void Backup(AZStd::vector<FileEntryBase>& arrFiles) const;
        void Restore(const AZStd::vector<FileEntryBase>& arrFiles);

    protected:
        //recursively adds the files from this directory and subdirectories
        // the strRoot contains the trailing slash
        void AddAllFiles(FileEntryTree* pTree, AZStd::string_view strRoot);
    };

    struct FileRecordFileOffsetOrder
    {
        bool operator()(const FileRecord& left, const FileRecord& right)
        {
            return left.pFileEntryBase->nFileHeaderOffset < right.pFileEntryBase->nFileHeaderOffset;
        }
    };


    struct FileEntryFileOffsetOrder
    {
        bool operator()(FileEntry* pLeft, FileEntry* pRight) const
        {
            return pLeft->nFileHeaderOffset < pRight->nFileHeaderOffset;
        }
    };

    // this is used for refreshing EOFOffsets
    class FileEntryList
        : public AZStd::set<FileEntry*, FileEntryFileOffsetOrder>
    {
    public:
        FileEntryList(FileEntryTree* pTree, uint32_t lCDROffset);
        // updates each file entry's info about the next file entry
        void RefreshEOFOffsets();
    protected:
        void Add(FileEntryTree* pTree);
        uint32_t m_lCDROffset;
    };
}
