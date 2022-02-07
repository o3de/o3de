/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// This is the class that can read the directory from Zip file,
// and store it into the directory cache

#pragma once

#include <AzFramework/Archive/IArchive.h>

namespace AZ::IO::ZipDir
{
    class Cache;

    // an instance of this class is temporarily created on stack to initialize the CZipFile instance
    class CacheFactory
    {
    public:
        enum
        {
            // open RW cache in read-only mode
            FLAGS_READ_ONLY = 1,
            // do not compact RW-cached zip upon destruction
            FLAGS_DONT_COMPACT = 1 << 1,
            // if this is set, then the zip paths won't be memorized in the cache objects
            FLAGS_DONT_MEMORIZE_ZIP_PATH = 1 << 2,
            // if this is set, the archive will be created anew (the existing file will be overwritten)
            FLAGS_CREATE_NEW = 1 << 3,

            // if this is set, zip path will be searched inside other zips
            FLAGS_READ_INSIDE_PAK = 1 << 7,
        };

        // initializes the internal structures
        // nFlags can have FLAGS_READ_ONLY flag, in this case the object will be opened only for reading
        CacheFactory(InitMethod nInitMethod, uint32_t nFlags = 0);
        ~CacheFactory();

        // the new function creates a new cache
        CachePtr New(const char* szFileName);

    protected:
        // reads the zip file into the file entry tree.
        bool ReadCache(Cache& rwCache);

        void Clear();

        // reads everything and prepares the maps
        bool Prepare();

        // searches for CDREnd record in the given file
        bool FindCDREnd();// throw(ErrorEnum);

        // uses the found CDREnd to scan the CDR and probably the Zip file itself
        // builds up the m_mapFileEntries
        bool BuildFileEntryMap();// throw (ErrorEnum);

        // give the CDR File Header entry, reads the local file header to validate and determine where
        // the actual file lies
        // This function can actually modify strFilePath variable, make sure you use a copy of the real path.
        void AddFileEntry(char* strFilePath, const ZipFile::CDRFileHeader* pFileHeader, const SExtraZipFileData& extra);// throw (ErrorEnum);

        // initializes the actual data offset in the file in the fileEntry structure
        // searches to the local file header, reads it and calculates the actual offset in the file
        void InitDataOffset(FileEntryBase& fileEntry, const ZipFile::CDRFileHeader* pFileHeader);

        // seeks in the file relative to the starting position
        void Seek(uint32_t nPos, int nOrigin = 0); // throw
        int64_t Tell(); // throw
        bool Read(void* pDest, uint32_t nSize); // throw
        bool ReadHeaderData(void* pDest, uint32_t nSize); // throw


    protected:

        AZStd::string m_szFilename;
        CZipFile m_fileExt;

        InitMethod m_nInitMethod;
        uint32_t m_nFlags;
        ZipFile::CDREnd m_CDREnd;

        size_t m_nZipFileSize;

        uint32_t m_nCDREndPos; // position of the CDR End in the file

        // Map: Relative file path => file entry info
        using FileEntryMap = AZStd::map<AZStd::string, ZipDir::FileEntryBase>;
        FileEntryMap m_mapFileEntries;

        FileEntryTree m_treeFileEntries;

        AZStd::vector<uint8_t> m_CDR_buffer;

        bool m_bBuildFileEntryMap;
        bool m_bBuildFileEntryTree;
        bool m_bBuildOptimizedFileEntry;
        ZipFile::EHeaderEncryptionType m_encryptedHeaders;
        ZipFile::EHeaderSignatureType m_signedHeaders;

        ZipFile::CryCustomEncryptionHeader m_headerEncryption;
        ZipFile::CrySignedCDRHeader m_headerSignature;
        ZipFile::CryCustomExtendedHeader m_headerExtended;
    };

}

