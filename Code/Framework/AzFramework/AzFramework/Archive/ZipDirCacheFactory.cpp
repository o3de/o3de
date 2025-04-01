/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Console/Console.h>
#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/sort.h>
#include <AzFramework/Archive/Archive.h>
#include <AzFramework/Archive/ZipDirStructures.h>
#include <AzFramework/Archive/ZipDirTree.h>
#include <AzFramework/Archive/ZipDirCache.h>
#include <AzFramework/Archive/ZipDirCacheFactory.h>
#include <AzFramework/Archive/ZipDirList.h>
#include <AzFramework/Archive/ZipFileFormat.h>

#include <zlib.h>

#include <locale>
#include <cinttypes>

namespace AZ::IO::ZipDir
{
    // this sets the window size of the blocks of data read from the end of the file to find the Central Directory Record
    // since normally there are no
    static constexpr size_t CDRSearchWindowSize = 0x100;
    CacheFactory::CacheFactory(InitMethod nInitMethod, uint32_t nFlags)
    {
        m_nCDREndPos = 0;
        m_bBuildFileEntryMap = false; // we only need it for validation/debugging
        m_bBuildFileEntryTree = true; // we need it to actually build the optimized structure of directories
        m_bBuildOptimizedFileEntry = false;
        m_nInitMethod = nInitMethod;
        m_nFlags = nFlags;
        m_nZipFileSize = 0;
        m_encryptedHeaders = ZipFile::HEADERS_NOT_ENCRYPTED;
        m_signedHeaders = ZipFile::HEADERS_NOT_SIGNED;

        if (m_nFlags & FLAGS_READ_INSIDE_PAK)
        {
            m_fileExt.m_fileIOBase = AZ::IO::FileIOBase::GetInstance();
        }
        else
        {
            m_fileExt.m_fileIOBase = AZ::IO::FileIOBase::GetDirectInstance();
        }
    }

    CacheFactory::~CacheFactory()
    {
        Clear();
    }

    CachePtr CacheFactory::New(const char* szFileName)
    {
        m_szFilename = szFileName;

        CachePtr pCache{ aznew Cache{} };
        // opens the given zip file and connects to it. Creates a new file if no such file exists
        // if successful, returns true.
        if (!(m_nFlags & FLAGS_DONT_MEMORIZE_ZIP_PATH))
        {
            pCache->m_strFilePath = szFileName;
        }

        if (m_nFlags & FLAGS_DONT_COMPACT)
        {
            pCache->m_nFlags |= Cache::FLAGS_DONT_COMPACT;
        }

        // first, try to open the file for reading or reading/writing
        if (m_nFlags & FLAGS_READ_ONLY)
        {
            AZ::IO::FileIOBase::GetDirectInstance()->Open(szFileName, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, m_fileExt.m_fileHandle);
            pCache->m_nFlags |= Cache::FLAGS_CDR_DIRTY | Cache::FLAGS_READ_ONLY;

            if (m_fileExt.m_fileHandle == AZ::IO::InvalidHandle)
            {
                AZ_Warning("Archive", false, R"(ZD_ERROR_IO_FAILED: Could not open file "%s" in binary mode for reading)", szFileName);
                return {};
            }
            if (!ReadCache(*pCache))
            {
                AZ_Warning("Archive", false, R"(ZD_ERROR_IO_FAILED: Could not read the CDR of the pack file "%s".)", pCache->m_strFilePath.c_str());
                return {};
            }
        }
        else
        {
            m_fileExt.m_fileHandle = AZ::IO::InvalidHandle;
            if (!(m_nFlags & FLAGS_CREATE_NEW))
            {
                AZ::IO::FileIOBase::GetDirectInstance()->Open(szFileName, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeUpdate | AZ::IO::OpenMode::ModeBinary, m_fileExt.m_fileHandle);
            }

            bool bOpenForWriting = true;

            if (m_fileExt.m_fileHandle != AZ::IO::InvalidHandle)
            {
                Seek(0, SEEK_END);
                size_t nFileSize = (size_t)Tell();
                Seek(0, SEEK_SET);

                AZ_Warning("Archive", nFileSize != 0, R"(ZD_ERROR_IO_FAILED: File "%s" with size 0 will not be open for reading)", szFileName);
                if (nFileSize)
                {
                    if (!ReadCache(*pCache))
                    {
                        AZ_Warning("Archive", false, R"(ZD_ERROR_IO_FAILED: Could not open file "%s" in binary mode for reading)", szFileName);
                        return {};
                    }
                    bOpenForWriting = false;
                }
            }

            if (bOpenForWriting)
            {
                if (m_fileExt.m_fileHandle != AZ::IO::InvalidHandle)
                {
                    AZ::IO::FileIOBase::GetDirectInstance()->Close(m_fileExt.m_fileHandle);
                    m_fileExt.m_fileHandle = AZ::IO::InvalidHandle;
                }

                if (AZ::IO::FileIOBase::GetDirectInstance()->Open(szFileName, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeUpdate | AZ::IO::OpenMode::ModeBinary, m_fileExt.m_fileHandle))
                {
                    // there's no such file, but we'll create one. We'll need to write out the CDR here
                    pCache->m_lCDROffset = 0;
                    pCache->m_nFlags |= Cache::FLAGS_CDR_DIRTY;
                }
            }

            if (m_fileExt.m_fileHandle == AZ::IO::InvalidHandle)
            {
                AZ_Warning("Archive", false, R"(ZD_ERROR_IO_FAILED: Could not open file "%s" in binary mode for appending (read/write))", szFileName);
                return {};
            }
        }


        // give the cache the file handle:
        pCache->m_fileHandle = m_fileExt.m_fileHandle;
        // the factory doesn't own it after that
        m_fileExt.m_fileHandle = AZ::IO::InvalidHandle;

        return pCache;
    }

    bool CacheFactory::ReadCache(Cache& rwCache)
    {
        m_bBuildFileEntryTree = true;
        if (!Prepare())
        {
            return false;
        }

        // since it's open for R/W, we need to know exactly how much space
        // we have for each file to use the gaps efficiently
        FileEntryList Adjuster(&m_treeFileEntries, m_CDREnd.lCDROffset);
        Adjuster.RefreshEOFOffsets();

        m_treeFileEntries.Swap(rwCache.m_treeDir);
        m_CDR_buffer.swap(rwCache.m_CDR_buffer);   // CDR Buffer contain actually the string pool for the tree directory.

        // very important: we need this offset to be able to add to the zip file
        rwCache.m_lCDROffset = m_CDREnd.lCDROffset;

        rwCache.m_encryptedHeaders = m_encryptedHeaders;
        rwCache.m_signedHeaders = m_signedHeaders;
        rwCache.m_headerSignature = m_headerSignature;
        rwCache.m_headerEncryption = m_headerEncryption;
        rwCache.m_headerExtended = m_headerExtended;

        return true;
    }

    // reads everything and prepares the maps
    bool CacheFactory::Prepare()
    {
        if (!FindCDREnd())
        {
            return false;
        }

        //Earlier pak file encryption techniques stored the encryption type in the disk number of the CDREnd.
        //This works, but can't be used by the more recent techniques that require signed paks to be readable by 7-Zip during dev.
        ZipFile::EHeaderEncryptionType headerEnc = (ZipFile::EHeaderEncryptionType)((m_CDREnd.nDisk & 0xC000) >> 14);
        if (headerEnc == ZipFile::HEADERS_ENCRYPTED_TEA || headerEnc == ZipFile::HEADERS_ENCRYPTED_STREAMCIPHER)
        {
            m_encryptedHeaders = headerEnc;
        }
        m_CDREnd.nDisk = m_CDREnd.nDisk & 0x3fff;

        //Pak may be encrypted with CryCustom technique and/or signed. Being signed is compatible (in principle) with the earlier encryption methods.
        //The information for this exists in some custom headers at the end of the archive (in the comment section)
        if (m_CDREnd.nCommentLength >= sizeof(m_headerExtended))
        {
            Seek(m_CDREnd.lCDROffset + m_CDREnd.lCDRSize + sizeof(ZipFile::CDREnd));
            Read(&m_headerExtended, sizeof(m_headerExtended));
            if (m_headerExtended.nHeaderSize != sizeof(m_headerExtended))
            {
                // Extended Header is not valid
                AZ_Warning("Archive", false, "ZD_ERROR_DATA_IS_CORRUPT: Bad extended header");
                return false;
            }
            //We have the header, so read the encryption and signing techniques
            m_signedHeaders = (ZipFile::EHeaderSignatureType)m_headerExtended.nSigning;

            //Prepare for a quick sanity check on the size of the comment field now that we know what it should contain
            //Also check that the techniques are supported
            uint16_t expectedCommentLength = sizeof(m_headerExtended);

            if (m_headerExtended.nEncryption != ZipFile::HEADERS_NOT_ENCRYPTED && m_encryptedHeaders != ZipFile::HEADERS_NOT_ENCRYPTED)
            {
                //Encryption technique has been specified in both the disk number (old technique) and the custom header (new technique).
                AZ_Warning("Archive", false, "ZD_ERROR_DATA_IS_CORRUPT: Unexpected encryption technique in header");
                return false;
            }
            else
            {
                //The encryption technique has been specified only in the custom header
                m_encryptedHeaders = (ZipFile::EHeaderEncryptionType)m_headerExtended.nEncryption;
                switch (m_encryptedHeaders)
                {
                case ZipFile::HEADERS_NOT_ENCRYPTED:
                    break;
                case ZipFile::HEADERS_ENCRYPTED_STREAMCIPHER_KEYTABLE:
                    expectedCommentLength += sizeof(ZipFile::CryCustomEncryptionHeader);
                    break;
                default:
                    // Unexpected technique
                    AZ_Warning("Archive", false, "ZD_ERROR_DATA_IS_CORRUPT: Bad encryption technique in header");
                    return false;
                }
            }

            //Add the signature header to the expected size
            switch (m_signedHeaders)
            {
            case ZipFile::HEADERS_NOT_SIGNED:
                break;
            case ZipFile::HEADERS_CDR_SIGNED:
                expectedCommentLength += sizeof(ZipFile::CrySignedCDRHeader);
                break;
            default:
                // Unexpected technique
                AZ_Warning("Archive", false, "ZD_ERROR_DATA_IS_CORRUPT: Bad signing technique in header");
                return false;
            }

            if (m_CDREnd.nCommentLength == expectedCommentLength)
            {
                if (m_signedHeaders == ZipFile::HEADERS_CDR_SIGNED)
                {
                    Read(&m_headerSignature, sizeof(m_headerSignature));
                    if (m_headerSignature.nHeaderSize != sizeof(m_headerSignature))
                    {
                        AZ_Warning("Archive", false, "ZD_ERROR_DATA_IS_CORRUPT: Bad signature header");
                        return false;
                    }
                }
            }
            else
            {
                // Unexpected technique
                AZ_Warning("Archive", false, "ZD_ERROR_DATA_IS_CORRUPT: Comment field is the wrong length");
                return false;
            }
        }


        // we don't support multivolume archives
        if (m_CDREnd.nDisk != 0
            || m_CDREnd.nCDRStartDisk != 0
            || m_CDREnd.numEntriesOnDisk != m_CDREnd.numEntriesTotal)
        {
            AZ_Warning("Archive", false, "ZD_ERROR_UNSUPPORTED: Multivolume archive detected.Current version of ZipDir does not support multivolume archives");
            return false;
        }

        // if the central directory offset or size are out of range,
        // the CDREnd record is probably corrupt
        if (m_CDREnd.lCDROffset > m_nCDREndPos
            || m_CDREnd.lCDRSize > m_nCDREndPos
            || m_CDREnd.lCDROffset + m_CDREnd.lCDRSize > m_nCDREndPos)
        {
            AZ_Warning("Archive", false, "ZD_ERROR_DATA_IS_CORRUPT: The central directory offset or size are out of range, the pak is probably corrupt, try to repare or delete the file");
            return false;
        }

        BuildFileEntryMap();

        return true;
    }

    struct SortFileEntryByNameOffsetPredicate
    {
        bool operator()(const FileEntry& f1, const FileEntry& f2) const
        {
            return f1.nNameOffset < f2.nNameOffset;
        }
    };

    void CacheFactory::Clear()
    {
        m_fileExt.Close();

        m_nCDREndPos = 0;
        memset(&m_CDREnd, 0, sizeof(m_CDREnd));
        m_mapFileEntries.clear();
        m_treeFileEntries.Clear();
        m_encryptedHeaders = ZipFile::HEADERS_NOT_ENCRYPTED;
    }


    //////////////////////////////////////////////////////////////////////////
    // searches for CDREnd record in the given file
    bool CacheFactory::FindCDREnd()
    {
        // this buffer will be used to find the CDR End record
        // the additional bytes are required to store the potential tail of the CDREnd structure
        // when moving the window to the next position in the file

        //We cannot create it on the stack for RSX memory usage as we are not permitted to access it via SPU
        AZStd::vector<char> pReservedBuffer(CDRSearchWindowSize + sizeof(ZipFile::CDREnd) - 1);

        Seek(0, SEEK_END);
        int64_t nFileSize = Tell();

        //There is a 2GB pak file limit
        constexpr size_t pakSizeLimit{ 1U << 31 };
        if (nFileSize > pakSizeLimit)
        {
            AZ_Fatal("Archive", "The file is too large. Can't open a pak file that is greater than 2GB in size. Current size is " PRIi64, nFileSize);
        }

        m_nZipFileSize = aznumeric_cast<size_t>(nFileSize);

        if (nFileSize < sizeof(ZipFile::CDREnd))
        {
            AZ_Warning("Archive", false, "The file is too small(%" PRIi64 "), it needs to contain the CDREnd structure which is %zu bytes. Please check and delete the file. Truncated files are not deleted automatically",
                nFileSize, sizeof(ZipFile::CDREnd));
            return false;
        }

        // this will point to the place where the buffer was loaded
        auto nOldBufPos = aznumeric_cast<uint32_t>(nFileSize);
        // start scanning well before the end of the file to avoid reading beyond the end

        uint32_t nScanPos = nOldBufPos - sizeof(ZipFile::CDREnd);

        m_CDREnd.lSignature = 0; // invalid signature as the flag of not-found CDR End structure
        while (true)
        {
            uint32_t nNewBufPos; // the new buf pos
            char* pWindow = &pReservedBuffer[0]; // the window pointer into which data will be read (takes into account the possible tail-of-CDREnd)
            if (nOldBufPos <= CDRSearchWindowSize)
            {
                // the old buffer position doesn't let us read the full search window size
                // therefore the new buffer pos will be 0 (instead of negative beyond the start of the file)
                // and the window pointer will be closer tot he end of the buffer because the end of the buffer
                // contains the data from the previous iteration (possibly)
                nNewBufPos = 0;
                pWindow = &pReservedBuffer[CDRSearchWindowSize - (nOldBufPos - nNewBufPos)];
            }
            else
            {
                nNewBufPos = nOldBufPos - CDRSearchWindowSize;
                AZ_Assert(nNewBufPos > 0, "The new buffer position must be greater than the last search window");
            }

            // since dealing with 32bit unsigned, check that filesize is bigger than
            // CDREnd plus comment before the following check occurs.
            if (nFileSize > (sizeof(ZipFile::CDREnd) + 0xFFFF))
            {
                // if the new buffer pos is beyond 64k limit for the comment size
                if (nNewBufPos < aznumeric_cast<uint32_t>(nFileSize - sizeof(ZipFile::CDREnd) - 0xFFFF))
                {
                    nNewBufPos = aznumeric_cast<uint32_t>(nFileSize - sizeof(ZipFile::CDREnd) - 0xFFFF);
                }
            }

            // if there's nothing to search
            if (nNewBufPos >= nOldBufPos)
            {
                AZ_Warning("Archive", false, "ZD_ERROR_NO_CDR: Cannot find Central Directory Record in pak."
                    " This is either not a pak file, or a pak file without Central Directory."
                    " It does not mean that the data is permanently lost,"
                    " but it may be severely damaged."
                    " Please repair the file with external tools,"
                    " there may be enough information left to recover the file completely."); // we didn't find anything
                return false;
            }

            // seek to the start of the new window and read it
            Seek(nNewBufPos);
            Read(pWindow, nOldBufPos - nNewBufPos);

            while (nScanPos >= nNewBufPos)
            {
                ZipFile::CDREnd* pEnd = (ZipFile::CDREnd*)(pWindow + nScanPos - nNewBufPos);
                auto formatSignature = pEnd->lSignature;
                if (formatSignature == pEnd->SIGNATURE)
                {
                    auto commentFileLength = pEnd->nCommentLength;
                    if (commentFileLength == nFileSize - nScanPos - sizeof(ZipFile::CDREnd))
                    {
                        // the comment length is exactly what we expected
                        m_CDREnd = *pEnd;
                        m_nCDREndPos = nScanPos;
                        break;
                    }
                    else
                    {
                        AZ_Warning("Archive", false, "ZD_ERROR_DATA_IS_CORRUPT:"
                            " Central Directory Record is followed by a comment of inconsistent length."
                            " This might be a minor misconsistency, please try to repair the file.However,"
                            " it is dangerous to open the file because I will have to guess some structure offsets,"
                            " which can lead to permanent unrecoverable damage of the archive content");
                        return false;
                    }
                }
                if (nScanPos == 0)
                {
                    break;
                }
                --nScanPos;
            }

            if (m_CDREnd.lSignature == m_CDREnd.SIGNATURE)
            {
                return true; // we've found it
            }
            nOldBufPos = nNewBufPos;
            memmove(&pReservedBuffer[CDRSearchWindowSize], pWindow, sizeof(ZipFile::CDREnd) - 1);
        }
        AZ_Assert(false, "ZD_ERROR_UNEXPECTED: The program flow may not have possibly lead here. This error is unexplainable"); // we shouldn't be here

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    // uses the found CDREnd to scan the CDR and probably the Zip file itself
    // builds up the m_mapFileEntries
    bool CacheFactory::BuildFileEntryMap()
    {
        Seek(m_CDREnd.lCDROffset);

        if (m_CDREnd.lCDRSize == 0)
        {
            return true;
        }

        auto& pBuffer = m_CDR_buffer; // Use persistent buffer.

        pBuffer.resize(m_CDREnd.lCDRSize + 16); // Allocate some more because we use this memory as a strings pool.

        if (pBuffer.empty()) // couldn't allocate enough memory for temporary copy of CDR
        {
            AZ_Warning("Archive", false, "ZD_ERROR_NO_MEMORY: Not enough memory to cache Central Directory record for fast initialization. This error may not happen on non-console systems");
            return false;
        }

        if (!ReadHeaderData(&pBuffer[0], m_CDREnd.lCDRSize))
        {
            AZ_Warning("Archive", false, "ZD_ERROR_CORRUPTED_DATA: Archive contains corrupted CDR.");
            return false;
        }

        // now we've read the complete CDR - parse it.
        ZipFile::CDRFileHeader* pFile = (ZipFile::CDRFileHeader*)(&pBuffer[0]);
        const uint8_t* pEndOfData = &pBuffer[0] + m_CDREnd.lCDRSize;
        uint8_t* pFileName;

        while ((pFileName = (uint8_t*)(pFile + 1)) <= pEndOfData)
        {
            // Hacky way to use CDR memory block as a string pool.
            pFile->lSignature = 0; // Force signature to always be 0 (First byte of signature maybe a zero termination of the previous file filename).

            if ((pFile->nVersionNeeded & 0xFF) > 20)
            {
                AZ_Warning("Archive", false, "ZD_ERROR_UNSUPPORTED: Cannot read the archive file (nVersionNeeded > 20).");
                return false;
            }
            //if (pFile->lSignature != pFile->SIGNATURE) // Timur, Dont compare signatures as signatue in memory can be overwritten by the code below
            //break;
            // the end of this file record
            const uint8_t* pEndOfRecord = (pFileName + pFile->nFileNameLength + pFile->nExtraFieldLength + pFile->nFileCommentLength);
            // if the record overlaps with the End Of CDR structure, something is wrong
            if (pEndOfRecord > pEndOfData)
            {
                AZ_Warning("Archive", false, "ZD_ERROR_CDR_IS_CORRUPT: Central Directory record is either corrupt, or truncated, or missing."
                    " Cannot read the archive directory");
                return false;
            }

            //////////////////////////////////////////////////////////////////////////
            // Analyze advanced section.
            //////////////////////////////////////////////////////////////////////////
            SExtraZipFileData extra;
            const uint8_t* pExtraField = (pFileName + pFile->nFileNameLength);
            const uint8_t* pExtraEnd = pExtraField + pFile->nExtraFieldLength;
            while (pExtraField < pExtraEnd)
            {
                const uint8_t* pAttrData = pExtraField + sizeof(ZipFile::ExtraFieldHeader);
                ZipFile::ExtraFieldHeader& hdr = *(ZipFile::ExtraFieldHeader*)pExtraField;
                switch (hdr.headerID)
                {
                case ZipFile::EXTRA_NTFS:
                {
                    memcpy(&extra.nLastModifyTime, pAttrData + sizeof(ZipFile::ExtraNTFSHeader), sizeof(extra.nLastModifyTime));
                }
                break;
                }
                pExtraField += sizeof(ZipFile::ExtraFieldHeader) + hdr.dataSize;
            }

            bool bDirectory = false;
            if (pFile->nFileNameLength > 0 && AZStd::string_view{ AZ_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR }.find_first_of(pFileName[pFile->nFileNameLength - 1]) != AZStd::string_view::npos)
            {
                bDirectory = true;
            }

            if (!bDirectory)
            {
                // Add this file entry.
                char* str = reinterpret_cast<char*>(pFileName);
                str[pFile->nFileNameLength] = 0; // Not standard!, may overwrite signature of the next memory record data in zip.
                AddFileEntry(str, pFile, extra);
            }

            // move to the next file
            pFile = (ZipFile::CDRFileHeader*)pEndOfRecord;
        }

        // finished reading CDR
        return true;
    }


    //////////////////////////////////////////////////////////////////////////
    // give the CDR File Header entry, reads the local file header to validate
    // and determine where the actual file resides
    void CacheFactory::AddFileEntry(char* strFilePath, const ZipFile::CDRFileHeader* pFileHeader, const SExtraZipFileData& extra)
    {
        if (pFileHeader->lLocalHeaderOffset > m_CDREnd.lCDROffset)
        {
            AZ_Warning("Archive", false, "ZD_ERROR_CDR_IS_CORRUPT:"
                " Central Directory contains file descriptors pointing outside the archive file boundaries."
                " The archive file is either truncated or damaged.Please try to repair the file"); // the file offset is beyond the CDR: impossible
            return;
        }

        if ((pFileHeader->nMethod == ZipFile::METHOD_STORE || pFileHeader->nMethod == ZipFile::METHOD_STORE_AND_STREAMCIPHER_KEYTABLE) && pFileHeader->desc.lSizeUncompressed != pFileHeader->desc.lSizeCompressed)
        {
            AZ_Warning("Archive", false, "ZD_ERROR_VALIDATION_FAILED:"
                " File with STORE compression method declares its compressed size not matching its uncompressed size."
                " File descriptor is inconsistent, archive content may be damaged, please try to repair the archive");
            return;
        }

        FileEntryBase fileEntry(*pFileHeader, extra);

        InitDataOffset(fileEntry, pFileHeader);

        if (m_bBuildFileEntryMap)
        {
            m_mapFileEntries.emplace(strFilePath, fileEntry);
        }

        if (m_bBuildFileEntryTree)
        {
            m_treeFileEntries.Add(strFilePath, fileEntry);
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // initializes the actual data offset in the file in the fileEntry structure
    // searches to the local file header, reads it and calculates the actual offset in the file
    void CacheFactory::InitDataOffset(FileEntryBase& fileEntry, const ZipFile::CDRFileHeader* pFileHeader)
    {
        if (m_encryptedHeaders != ZipFile::HEADERS_NOT_ENCRYPTED)
        {
            // use CDR instead of local header
            fileEntry.nFileDataOffset = pFileHeader->lLocalHeaderOffset + sizeof(ZipFile::LocalFileHeader) + pFileHeader->nFileNameLength + pFileHeader->nExtraFieldLength;
        }
        else
        {
            Seek(pFileHeader->lLocalHeaderOffset);

            // Read only the LocalFileHeader w/ no additional bytes ('name' or 'extra' fields)
            AZStd::vector<char> buffer;
            uint32_t bufferLen = sizeof(ZipFile::LocalFileHeader);
            buffer.resize_no_construct(bufferLen);
            Read(buffer.data(), bufferLen);

            const auto* localFileHeader = reinterpret_cast<const ZipFile::LocalFileHeader*>(buffer.data());

            // set the correct file data offset...
            fileEntry.nFileDataOffset = pFileHeader->lLocalHeaderOffset + sizeof(ZipFile::LocalFileHeader) +
                localFileHeader->nFileNameLength + localFileHeader->nExtraFieldLength;

            fileEntry.nEOFOffset = fileEntry.nFileDataOffset + fileEntry.desc.lSizeCompressed;

            if (m_nInitMethod != ZipDir::InitMethod::Default)
            {
                if (m_nInitMethod == ZipDir::InitMethod::FullValidation)
                {
                    // Mark the FileEntry to check CRC when the next read occurs
                    fileEntry.bCheckCRCNextRead = true;
                }

                // Timestamps
                if (pFileHeader->nLastModDate != localFileHeader->nLastModDate
                    || pFileHeader->nLastModTime != localFileHeader->nLastModTime)
                {
                    AZ_Warning("Archive", false, "ZD_ERROR_VALIDATION_FAILED: (%s)\n"
                        " The local file header's modification timestamps don't match that of the global file header in the archive."
                        " The archive timestamps are inconsistent and may be damaged. Check the archive file.", m_szFilename.c_str());
                    // don't return here, it may be ok.
                }

                // Validate data
                if (pFileHeader->desc != localFileHeader->desc  // this checks CRCs and compressed/uncompressed sizes
                    || pFileHeader->nMethod != localFileHeader->nMethod
                    || pFileHeader->nFileNameLength != localFileHeader->nFileNameLength)
                {
                    AZ_Warning("Archive", false, "ZD_ERROR_VALIDATION_FAILED: (%s)\n"
                        " The local file header descriptor doesn't match basic parameters declared in the global file header in the file."
                        " The archive content is inconsistent and may be damaged. Please try to repair the archive.", m_szFilename.c_str());
                    // return here because further checks aren't worse than this.
                    return;
                }

                // Read extra data
                uint32_t extraDataLen = localFileHeader->nFileNameLength + localFileHeader->nExtraFieldLength;
                buffer.resize_no_construct(buffer.size() + extraDataLen);
                Read(buffer.data() + buffer.size(), extraDataLen);

                // Compare local file name with the CDR file name, they should match
                AZStd::string_view zipFileName{ buffer.data() + sizeof(ZipFile::LocalFileHeader), localFileHeader->nFileNameLength };
                AZStd::string_view cdrFileName{ reinterpret_cast<const char*>(pFileHeader + 1), pFileHeader->nFileNameLength };
                if (zipFileName != cdrFileName)
                {
                    AZ_Warning("Archive", false, "ZD_ERROR_VALIDATION_FAILED: (%s)\n"
                        " The file name in the local file header doesn't match the name in the global file header."
                        " The archive content is inconsisten with the directory. Please check the archive.", m_szFilename.c_str());
                }

                // CDR and local "extra field" lengths may be different, should we compare them if they are equal?

                // make sure it's the same file and the fileEntry structure is properly initialized
                AZ_Assert(fileEntry.nFileHeaderOffset == pFileHeader->lLocalHeaderOffset,
                    "The file entry header offset doesn't match the file header local offst (%s)", m_szFilename.c_str());

                if (fileEntry.nFileDataOffset >= m_nCDREndPos)
                {
                    AZ_Warning("Archive", false, "ZD_ERROR_VALIDATION_FAILED: (%s)\n"
                        " The global file header declares the file which crosses the boundaries of the archive."
                        " The archive is either corrupted or truncated, please try to repair it", m_szFilename.c_str());
                }

                // End Validation
            }
        }
    }

    // seeks in the file relative to the starting position
    void CacheFactory::Seek(uint32_t nPos, int nOrigin) // throw
    {
        if (FSeek(&m_fileExt, nPos, nOrigin))
        {
            AZ_Warning("Archive", false, "ZD_ERROR_IO_FAILED: Cannot fseek() to the new position in the file. This is unexpected error and should not happen under any circumstances. Perhaps some network or disk failure error has caused this");
            return;
        }
    }

    int64_t CacheFactory::Tell() // throw
    {
        int64_t nPos = FTell(&m_fileExt);
        if (nPos == -1)
        {
            AZ_Warning("Archive", false, "ZD_ERROR_IO_FAILED: Cannot ftell() position in the archive. This is unexpected error and should not happen under any circumstances. Perhaps some network or disk failure error has caused this");
            return 0;
        }
        return nPos;
    }

    bool CacheFactory::Read(void* pDest, uint32_t nSize) // throw
    {
        if (FRead(&m_fileExt, pDest, nSize, 1) != 1)
        {
            AZ_Warning("Archive", false, "ZD_ERROR_IO_FAILED: Cannot fread() a portion of data from archive");
            return false;
        }
        return true;
    }

    bool CacheFactory::ReadHeaderData(void* pDest, uint32_t nSize) // throw
    {
        if (!Read(pDest, nSize))
        {
            return false;
        }

        switch (m_encryptedHeaders)
        {
        case ZipFile::HEADERS_NOT_ENCRYPTED:
            break;  //Nothing to do here
        default:
            AZ_Warning("Archive", false, "Attempting to load encrypted pak by unsupported method, or unencrypted pak when support is disabled");
            return false;
        }

        switch (m_signedHeaders)
        {
        case ZipFile::HEADERS_CDR_SIGNED:
            AZ_Warning("Archive", false, "[ZipDir] HEADERS_CDR_SIGNED not yet supported");
            break;
        case ZipFile::HEADERS_NOT_SIGNED:
            //Nothing to do here
            break;
        default:
            AZ_Warning("Archive", false, "Unsupported pak signature, or use of unsigned pak when support is disabled.");
            return false;
            break;
        }

        return true;
    }
}
