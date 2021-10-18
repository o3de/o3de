/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/sort.h>
#include <AzFramework/Archive/ZipFileFormat.h>
#include <AzFramework/Archive/ZipDirStructures.h>
#include <AzFramework/Archive/ZipDirList.h>
#include <AzFramework/Archive/ZipDirTree.h>

namespace AZ::IO::ZipDir
{
    void FileDataRecordDeleter::operator()(const AZStd::intrusive_refcount<AZStd::atomic_uint, FileDataRecordDeleter>* ptr) const
    {
        auto fileDataRecordAddress = const_cast<FileDataRecord*>(static_cast<const FileDataRecord*>(ptr));
        if (m_allocator)
        {
            fileDataRecordAddress->~FileDataRecord();
            m_allocator->DeAllocate(fileDataRecordAddress);
        }
        else
        {
            delete fileDataRecordAddress;
        }
    }

    FileDataRecord::FileDataRecord()
        : AZStd::intrusive_refcount<AZStd::atomic_uint, FileDataRecordDeleter>{
            AZ::AllocatorInstance<AZ::OSAllocator>::IsReady() ?
            FileDataRecordDeleter{&AZ::AllocatorInstance<AZ::OSAllocator>::Get() }
            : FileDataRecordDeleter{} }
    {
    }

    auto FileDataRecord::New(const FileRecord& rThat, AZ::IAllocatorAllocate* allocator) -> AZStd::intrusive_ptr<FileDataRecord>
    {
        auto fileDataRecordAlloc = reinterpret_cast<FileDataRecord*>(allocator->Allocate(
            sizeof(FileDataRecord) + rThat.pFileEntryBase->desc.lSizeCompressed,
            alignof(FileDataRecord),
            0,
            "FileDataRecord::New"));


        if (fileDataRecordAlloc)
        {
            new (fileDataRecordAlloc) FileDataRecord{};
            *static_cast<FileRecord*>(fileDataRecordAlloc) = rThat;
        }
        return fileDataRecordAlloc;
    }
    FileRecordList::FileRecordList(FileEntryTree* pTree)
    {
        clear();
        reserve(pTree->NumFilesTotal());
        AddAllFiles(pTree, {});
    }

    //recursively adds the files from this directory and subdirectories
    // the strRoot contains the trailing slash
    void FileRecordList::AddAllFiles(FileEntryTree* pTree, AZStd::string_view strRoot)
    {
        for (FileEntryTree::SubdirMap::iterator it = pTree->GetDirBegin(); it != pTree->GetDirEnd(); ++it)
        {
            AddAllFiles(it->second.get(), (AZ::IO::Path(strRoot, AZ::IO::PosixPathSeparator) / it->first).Native());
        }

        for (FileEntryTree::FileMap::iterator it = pTree->GetFileBegin(); it != pTree->GetFileEnd(); ++it)
        {
            FileRecord rec;
            rec.pFileEntryBase = pTree->GetFileEntry(it);
            rec.strPath = (AZ::IO::Path(strRoot, AZ::IO::PosixPathSeparator) / it->first).Native();
            push_back(rec);
        }
    }

    // sorts the files by the physical offset in the zip file
    void FileRecordList::SortByFileOffset()
    {
        AZStd::sort(begin(), end(), FileRecordFileOffsetOrder());
    }

    // returns the size of CDR in the zip file
    FileRecordList::ZipStats FileRecordList::GetStats() const
    {
        ZipStats Stats;
        Stats.nSizeCDR = sizeof(ZipFile::CDREnd);
        Stats.nSizeCompactData = 0;
        // for each file, we'll need to store only its CDR header and the name
        for (const_iterator it = begin(); it != end(); ++it)
        {
            Stats.nSizeCDR += sizeof(ZipFile::CDRFileHeader) + it->strPath.length();
            Stats.nSizeCompactData += sizeof(ZipFile::LocalFileHeader) + it->strPath.length() + it->pFileEntryBase->desc.lSizeCompressed;
        }

        return Stats;
    }

    // puts the CDR into the given block of mem
    size_t FileRecordList::MakeZipCDR(uint32_t lCDROffset, void* pBuffer) const
    {
        char* pCur = (char*)pBuffer;
        for (const_iterator it = begin(); it != end(); ++it)
        {
            ZipFile::CDRFileHeader& h = *(ZipFile::CDRFileHeader*)pCur;
            pCur = (char*)(&h + 1);
            h.lSignature = h.SIGNATURE;
            h.nVersionMadeBy = 20;
            h.nVersionNeeded = 20;
            h.nFlags = 0;
            h.nMethod = it->pFileEntryBase->nMethod;
            h.nLastModTime = it->pFileEntryBase->nLastModTime;
            h.nLastModDate = it->pFileEntryBase->nLastModDate;
            h.desc = it->pFileEntryBase->desc;
            h.nFileNameLength = static_cast<uint16_t>(it->strPath.size());
            h.nExtraFieldLength = 0;
            h.nFileCommentLength = 0;
            h.nDiskNumberStart = 0;
            h.nAttrInternal = 0;
            h.lAttrExternal = 0;
            h.lLocalHeaderOffset = it->pFileEntryBase->nFileHeaderOffset;

            memcpy(pCur, it->strPath.c_str(), it->strPath.size());
            pCur += it->strPath.size();
        }

        ZipFile::CDREnd& e = *(ZipFile::CDREnd*)pCur;
        e.lSignature = e.SIGNATURE;
        e.nDisk = 0;
        e.nCDRStartDisk = 0;
        e.numEntriesOnDisk = static_cast<uint16_t>(this->size());
        e.numEntriesTotal = static_cast<uint16_t>(this->size());
        e.lCDRSize = static_cast<uint32_t>(pCur - reinterpret_cast<char*>(pBuffer));
        e.lCDROffset = lCDROffset;
        e.nCommentLength = 0;

        pCur = (char*)(&e + 1);

        return pCur - (char*)pBuffer;
    }


    FileEntryList::FileEntryList(FileEntryTree* pTree, uint32_t lCDROffset)
        : m_lCDROffset(lCDROffset)
    {
        Add(pTree);
    }

    void FileEntryList::Add(FileEntryTree* pTree)
    {
        for (FileEntryTree::SubdirMap::iterator itDir = pTree->GetDirBegin(); itDir != pTree->GetDirEnd(); ++itDir)
        {
            Add(pTree->GetDirEntry(itDir));
        }
        for (FileEntryTree::FileMap::iterator itFile = pTree->GetFileBegin(); itFile != pTree->GetFileEnd(); ++itFile)
        {
            insert(pTree->GetFileEntry(itFile));
        }
    }

    // updates each file entry's info about the next file entry
    void FileEntryList::RefreshEOFOffsets()
    {
        iterator it, itNext = begin();

        if (itNext != end())
        {
            while ((it = itNext, ++itNext) != end())
            {
                // start scan
                (*it)->nEOFOffset = (*itNext)->nFileHeaderOffset;
            }
            // it is the last one..
            (*it)->nEOFOffset = m_lCDROffset;
        }
    }


    void FileRecordList::Backup(AZStd::vector<FileEntryBase>& arrFiles) const
    {
        arrFiles.reserve(size());

        for (const auto& fileEntryBase : *this)
        {
            arrFiles.emplace_back(*fileEntryBase.pFileEntryBase);
        }
    }

    void FileRecordList::Restore(const AZStd::vector<FileEntryBase>& arrFiles)
    {
        if (arrFiles.size() == size())
        {
            for (size_t fileEntryIndex = 0; fileEntryIndex < arrFiles.size(); ++fileEntryIndex)
            {
                AZStd::vector<FileRecord>& fileRecords = *this;
                *fileRecords[fileEntryIndex].pFileEntryBase = arrFiles[fileEntryIndex];
            }
        }
    }

}
