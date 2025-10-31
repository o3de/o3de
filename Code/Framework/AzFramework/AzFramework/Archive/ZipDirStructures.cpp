/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/PlatformIncl.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzFramework/Archive/Codec.h>
#include <AzFramework/Archive/IArchive.h>
#include <AzFramework/Archive/ZipFileFormat.h>
#include <AzFramework/Archive/ZipDirStructures.h>
#include <time.h>
#include <stdlib.h>
#include <zstd.h>
#include <lz4frame.h>
#include <zlib.h>

namespace AZ::IO::ZipDir::ZipDirStructuresInternal
{
    static void* ZlibAlloc(void* userData, uint32_t item, uint32_t size)
    {
        auto allocator = reinterpret_cast<AZ::IAllocator*>(userData);
        return allocator->Allocate(item * size, alignof(uint8_t), 0, "ZLibAlloc");
    }

    static void ZlibFree(void* userData, void* ptr)
    {
        auto allocator = reinterpret_cast<AZ::IAllocator*>(userData);
        allocator->DeAllocate(ptr);
    }

    static void ZlibInflateElement_Impl(const void* pCompressed, void* pUncompressed, size_t compressedSize, size_t nUnCompressedSize, size_t* pUncompressedSize, int* pReturnCode);

    static void ZlibOverlapInflate(int* pReturnCode, z_stream* pZStream, UncompressLookahead& lookahead, uint8_t* pOutput, size_t nOutputLen, const uint8_t* pInput, size_t nInputLen)
    {
        pZStream->total_in = 0;
        pZStream->avail_in = 0;
        pZStream->avail_out = 0;

        Bytef* pIn = (Bytef*)pInput;
        uInt nIn = aznumeric_cast<uint32_t>(nInputLen);
        uint8_t* pOutputEnd = pOutput + nOutputLen;

        uInt nInCursorZ = lookahead.cachedStartIdx;
        uInt nInCursorW = lookahead.cachedEndIdx;

        do
        {
            // Capture some more input

            {
                uInt nWrZ = nInCursorZ % sizeof(lookahead.buffer);
                uInt nWrW = nInCursorW % sizeof(lookahead.buffer);

                uInt nBytesToCache = nWrZ > nWrW
                    ? nWrZ - nWrW
                    : sizeof(lookahead.buffer) - nWrW;
                nBytesToCache = (AZStd::min)(nBytesToCache, nIn);

                if (nBytesToCache)
                {
                    memcpy(lookahead.buffer + nWrW, pIn, nBytesToCache);
                    pIn += nBytesToCache;
                    nIn -= nBytesToCache;
                    nInCursorW += nBytesToCache;
                }
            }

            if (!pZStream->avail_in)
            {
                // Need more input storage - provide it from the local window
                uInt nBytesInWindow = nInCursorW - nInCursorZ;
                if (nBytesInWindow > 0)
                {
                    uInt nWrZ = nInCursorZ % sizeof(lookahead.buffer);
                    uInt nWrW = nInCursorW % sizeof(lookahead.buffer);
                    pZStream->next_in = (Bytef*)lookahead.buffer + nWrZ;
                    pZStream->avail_in = nWrW <= nWrZ
                        ? sizeof(lookahead.buffer) - nWrZ
                        : nWrW - nWrZ;
                }
                else if (!nIn)
                {
                    break;
                }
            }

            pZStream->total_in = 0;

            // Limit so that output doesn't overflow into next read block
            pZStream->avail_out = (uInt)AZStd::min<uint64_t>(pIn - pZStream->next_out, (pOutputEnd - pZStream->next_out));

            int nAvailIn = pZStream->avail_in;
            int nAvailOut = pZStream->avail_out;

            *pReturnCode = inflate(pZStream, Z_SYNC_FLUSH);

            if (*pReturnCode == Z_BUF_ERROR)
            {
                // As long as we consumed something, keep going. Only fail permanently if we've stalled.
                if (nAvailIn != static_cast<int>(pZStream->avail_in) || nAvailOut != static_cast<int>(pZStream->avail_out))
                {
                    *pReturnCode = Z_OK;
                }
                else if (!nIn)
                {
                    *pReturnCode = Z_OK;
                    break;
                }
            }

            nInCursorZ += pZStream->total_in;
        } while (*pReturnCode == Z_OK);

        lookahead.cachedStartIdx = nInCursorZ;
        lookahead.cachedEndIdx = nInCursorW;
    }


    static int ZlibInflateIndependentWriteCombined(z_stream* pZS)
    {
        Bytef outputLocal[16384];

        Bytef* pOutRemote = pZS->next_out;
        uInt nOutRemoteLen = pZS->avail_out;

        int err = Z_OK;

        while ((err == Z_OK) && (nOutRemoteLen > 0) && (pZS->avail_in > 0))
        {
            pZS->next_out = outputLocal;
            pZS->avail_out = AZStd::min(static_cast<uInt>(sizeof(outputLocal)), nOutRemoteLen);

            err = inflate(pZS, Z_SYNC_FLUSH);

            int nEmitted = (int)(pZS->next_out - outputLocal);
            memcpy(pOutRemote, outputLocal, nEmitted);
            pOutRemote += nEmitted;
            nOutRemoteLen -= nEmitted;

            if ((err == Z_BUF_ERROR) && (nEmitted > 0))
            {
                err = Z_OK;
            }
        }

        pZS->next_out = pOutRemote;
        pZS->avail_out = nOutRemoteLen;

        return err;
    }

    //Works on an initialized z_stream, is responsible for calling ...init and ...end
    void ZlibInflateElementPartial_Impl(
        int* pReturnCode, z_stream* pZStream, UncompressLookahead* pLookahead,
        uint8_t* pOutput, size_t nOutputLen, bool bOutputWriteOnly,
        const uint8_t* pInput, size_t nInputLen, size_t* pTotalOut)
    {
        z_stream localStream;
        bool bUsingLocal = false;

        if (!pZStream)
        {
            memset(&localStream, 0, sizeof(localStream));
            pZStream = &localStream;
            bUsingLocal = true;
        }

        if (pZStream->total_out == 0)
        {
            *pReturnCode = inflateInit2(pZStream, -MAX_WBITS);

            if (*pReturnCode != Z_OK)
            {
                return;
            }
        }

        pZStream->next_out = pOutput;
        pZStream->avail_out = aznumeric_cast<uint32_t>(nOutputLen);

        // If src/dst overlap (in place decompress), then inflate in chunks, copying src locally to ensure
        // pointers don't foul each other.
        if ((pInput + nInputLen) <= pOutput || pInput >= (pOutput + nOutputLen))
        {
            pZStream->next_in = (Bytef*)pInput;
            pZStream->avail_in = aznumeric_cast<uint32_t>(nInputLen);

            if (!bOutputWriteOnly)
            {
                *pReturnCode = inflate(pZStream, Z_SYNC_FLUSH);
            }
            else
            {
                *pReturnCode = ZlibInflateIndependentWriteCombined(pZStream);
            }
        }
        else if (pLookahead)
        {
            ZlibOverlapInflate(pReturnCode, pZStream, *pLookahead, pOutput, nOutputLen, pInput, nInputLen);
        }
        else
        {
            UncompressLookahead lookahead;
            ZlibOverlapInflate(pReturnCode, pZStream, lookahead, pOutput, nOutputLen, pInput, nInputLen);
        }

        if (pTotalOut)
        {
            *pTotalOut = pZStream->total_out;
        }

        //error during inflate
        if (*pReturnCode != Z_STREAM_END && *pReturnCode != Z_OK)
        {
            inflateEnd(pZStream);
            return;
        }

        //check if we have finished the read
        if (*pReturnCode == Z_STREAM_END)
        {
            inflateEnd(pZStream);
        }
        else if (bUsingLocal)
        {
            *pReturnCode = Z_VERSION_ERROR;
        }
    }

    // function to do the zlib decompression, on SPU, we use ZlibInflateElement, which is defined in zlib_spu (which in case only forward to edge zlib decompression)
    void ZlibInflateElement_Impl(const void* pCompressed, void* pUncompressed, size_t compressedSize, size_t nUnCompressedSize, size_t* pUncompressedSize, int* pReturnCode)
    {
        int err;
        z_stream stream;

        Bytef* pIn = const_cast<Bytef*>(static_cast<const Bytef*>(pCompressed));
        uInt nIn = static_cast<uInt>(compressedSize);

        stream.next_out = static_cast<Bytef*>(pUncompressed);
        stream.avail_out = static_cast<uInt>(nUnCompressedSize);

        stream.zalloc = &ZlibAlloc;
        stream.zfree = &ZlibFree;
        stream.opaque = &AZ::AllocatorInstance<AZ::OSAllocator>::Get();

        err = inflateInit2(&stream, -MAX_WBITS);
        if (err != Z_OK)
        {
            *pReturnCode = err;
            return;
        }

        // If src/dst overlap (in place decompress), then inflate in chunks, copying src locally to ensure
        // pointers don't foul each other.
        if ((pIn + nIn) <= stream.next_out || pIn >= (stream.next_out + stream.avail_out))
        {
            stream.next_in = pIn;
            stream.avail_in = nIn;

            // for some strange reason, passing Z_FINISH doesn't work -
            // it seems the stream isn't finished for some files and
            // inflate returns an error due to stream-end-not-reached (though expected) problem
            err = inflate(&stream, Z_SYNC_FLUSH);
        }
        else
        {
            UncompressLookahead lookahead;
            ZlibOverlapInflate(&err, &stream, lookahead, (uint8_t*)pUncompressed, nUnCompressedSize, (uint8_t*)pCompressed, compressedSize);
        }

        if (err != Z_STREAM_END && err != Z_OK)
        {
            inflateEnd(&stream);
            *pReturnCode = err == Z_OK ? Z_BUF_ERROR : err;
            return;
        }

        *pUncompressedSize = stream.total_out;

        err = inflateEnd(&stream);
        *pReturnCode = err;
    }
}

namespace AZ::IO::ZipDir
{

    //////////////////////////////////////////////////////////////////////////
    void CZipFile::LoadToMemory(AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pData)
    {
        if (!m_pInMemoryData)
        {
            m_nCursor = 0;

            if (pData)
            {
                m_pInMemoryData = AZStd::move(pData);

                m_nSize = m_pInMemoryData->m_size;
            }
            else
            {
                AZ::IO::HandleType realFileHandle = m_fileHandle;

                AZ::u64 fileSize = 0;
                if (!m_fileIOBase->Size(realFileHandle, fileSize))
                {
                    // Error
                    m_nSize = 0;
                    return;
                }
                const size_t nFileSize = static_cast<size_t>(fileSize);

                m_pInMemoryData = aznew AZ::IO::MemoryBlock{};

                m_pInMemoryData->m_address.reset(reinterpret_cast<uint8_t*>(azmalloc(nFileSize, alignof(uint8_t))));
                m_pInMemoryData->m_size = nFileSize;


                m_nSize = nFileSize;

                if (!m_fileIOBase->Seek(realFileHandle, 0, AZ::IO::SeekType::SeekFromStart))
                {
                    // Error
                    m_nSize = 0;
                    return;
                }
                if (!m_fileIOBase->Read(realFileHandle, m_pInMemoryData->m_address.get(), nFileSize, true))
                {
                    // Error
                    m_nSize = 0;
                    return;
                }

                return;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void CZipFile::UnloadFromMemory()
    {
        m_pInMemoryData.reset();
    }

    //////////////////////////////////////////////////////////////////////////
    void CZipFile::Close(bool bUnloadFromMem)
    {
        if (m_fileHandle != AZ::IO::InvalidHandle)
        {
            m_fileIOBase->Close(m_fileHandle);
            m_fileHandle = AZ::IO::InvalidHandle;
        }

#ifdef SUPPORT_UNBUFFERED_IO
        if (m_unbufferedFile.IsOpen())
        {
            m_unbufferedFile.Close();
        }

        if (m_pReadTarget)
        {
            _aligned_free(m_pReadTarget);
            m_pReadTarget = nullptr;
        }
#endif

        if (bUnloadFromMem)
        {
            UnloadFromMemory();
        }

        if (!m_pInMemoryData)
        {
            m_nCursor = 0;
            m_nSize = 0;
        }
    }

#ifdef SUPPORT_UNBUFFERED_IO
    bool CZipFile::OpenUnbuffered(const char* filename)
    {
        if (!EvaluateSectorSize(filename))
        {
            return false;
        }

        // defining file attributes for opening files using constants to avoid the need to include windows headers
        constexpr int FileFlagNoBuffering = 0x20000000;
        constexpr int FileAttributeNormal = 0x00000080;
        if (m_unbufferedFile.Open(filename, AZ::IO::SystemFile::OpenMode::SF_OPEN_READ_ONLY, FileFlagNoBuffering | FileAttributeNormal))
        {
            m_nSize = aznumeric_cast<int64_t>(m_unbufferedFile.Length());
            return true;
        }

        return false;
    }

    bool CZipFile::EvaluateSectorSize(const char* filename)
    {
        AZ::IO::FixedMaxPath volume;

        if (AZ::IO::PathView(filename).IsRelative())
        {
            AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(volume, filename);
        }
        else
        {
            volume = filename;
        }

        AZ::IO::FixedMaxPath drive = volume.RootName();
        if (drive.empty())
        {
            return false;
        }

        DWORD bytesPerSector;
        if (GetDiskFreeSpaceA(drive.c_str(),nullptr, &bytesPerSector, nullptr, nullptr))
        {
            m_nSectorSize = bytesPerSector;
            AZ::IAllocator& allocator = AZ::AllocatorInstance<AZ::OSAllocator>::Get();
            if (m_pReadTarget)
            {
                allocator.DeAllocate(m_pReadTarget);
            }
            m_pReadTarget = allocator.Allocate(128 * 1024, m_nSectorSize);
            return true;
        }

        return false;
    }
#endif

    CZipFile::CZipFile() : m_fileHandle(AZ::IO::InvalidHandle)
#ifdef SUPPORT_UNBUFFERED_IO
        , m_unbufferedFile()
        , m_nSectorSize(0)
        , m_pReadTarget(nullptr)
#endif
        , m_nSize(0)
        , m_nCursor(0)
        , m_szFilename(nullptr)
        , m_pInMemoryData(nullptr)
    {

    }

    void CZipFile::Swap(CZipFile& other)
    {
        AZStd::swap(m_fileHandle, other.m_fileHandle);
#ifdef SUPPORT_UNBUFFERED_IO
        AZStd::swap(m_unbufferedFile, other.m_unbufferedFile);
        AZStd::swap(m_nSectorSize, other.m_nSectorSize);
        AZStd::swap(m_pReadTarget, other.m_pReadTarget);
#endif
        AZStd::swap(m_nSize, other.m_nSize);
        AZStd::swap(m_nCursor, other.m_nCursor);
        AZStd::swap(m_szFilename, other.m_szFilename);
        AZStd::swap(m_pInMemoryData, other.m_pInMemoryData);
        m_fileIOBase = other.m_fileIOBase;
    }

    //////////////////////////////////////////////////////////////////////////
    FileEntryBase::FileEntryBase(const ZipFile::CDRFileHeader& header, const SExtraZipFileData& extra)
    {
        desc = header.desc;
        nFileHeaderOffset = header.lLocalHeaderOffset;

        nMethod = header.nMethod;
        nNameOffset = 0; // we don't know yet
        nLastModTime = header.nLastModTime;
        nLastModDate = header.nLastModDate;
        nNTFS_LastModifyTime = extra.nLastModifyTime;

        // make an estimation (at least this offset should be there), but we don't actually know yet
        nFileDataOffset = header.lLocalHeaderOffset + sizeof(ZipFile::LocalFileHeader) + header.nFileNameLength + header.nExtraFieldLength;
        nEOFOffset = nFileDataOffset + header.desc.lSizeCompressed;
    }

    // Uncompresses raw (without wrapping) data that is compressed with method 8 (deflated) in the Zip file
    // returns one of the Z_* errors (Z_OK upon success)
    // This function just mimics the standard uncompress (with modification taken from unzReadCurrentFile)
    // with 2 differences: there are no 16-bit checks, and
    // it initializes the inflation to start without waiting for compression method byte, as this is the
    // way it's stored into zip file
    int ZipRawUncompress(void* pUncompressed, size_t* pDestSize, const void* pCompressed, size_t nSrcSize)
    {
        int nReturnCode = Z_OK;

        //check first 4 bytes to see what compression codec was used
        if (nSrcSize >= 4)
        {
            if (CompressionCodec::TestForZSTDMagic(pCompressed))
            {
                size_t result = ZSTD_decompress(pUncompressed, *pDestSize, pCompressed, nSrcSize);

                if (ZSTD_isError(result))
                {
                    AZ_Error("ZipDirStructures", false, "Error decompressing using zstd: %s", ZSTD_getErrorName(result));
                    nReturnCode = Z_BUF_ERROR;
                }
                else
                {
                    *pDestSize = result;
                }
                return nReturnCode;
            }
            else if (CompressionCodec::TestForLZ4Magic(pCompressed))
            {
                size_t result;
                LZ4F_decompressionContext_t dctx;
                result = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
                if (LZ4F_isError(result))
                {
                    AZ_Error("ZipDirStructures", false, "Error creating lz4 decompression context: %s", LZ4F_getErrorName(result));
                    return Z_BUF_ERROR;
                }

                size_t dstSize = *pDestSize;
                size_t srcSize = nSrcSize;
                result = LZ4F_decompress(dctx, pUncompressed, &dstSize, pCompressed, &srcSize, nullptr);
                if (LZ4F_isError(result))
                {
                    AZ_Error("ZipDirStructures", false, "Error decompressing using lz4: %s", LZ4F_getErrorName(result));
                    nReturnCode = Z_BUF_ERROR;
                }
                else
                {
                    *pDestSize = dstSize;
                }

                size_t freeCode = LZ4F_freeDecompressionContext(dctx);
                if (LZ4F_isError(freeCode))
                {
                    //We are not changing the return code in this case, but it is good to record that releasing the
                    //decompression context failed.
                    AZ_Error("ZipDirStructures", false, "Error releasing lz4 decompression context: %s", LZ4F_getErrorName(freeCode));
                }

                return nReturnCode;
            }
        }

        //fallback to zlib
        ZipDirStructuresInternal::ZlibInflateElement_Impl(pCompressed, pUncompressed, nSrcSize, *pDestSize, pDestSize, &nReturnCode);

        return nReturnCode;
    }

    // compresses the raw data into raw data. The buffer for compressed data itself with the heap passed. Uses method 8 (deflate)
    // returns one of the Z_* errors (Z_OK upon success)
    int ZipRawCompress(const void* pUncompressed, size_t* pDestSize, void* pCompressed, size_t nSrcSize, int nLevel)
    {
        z_stream stream;
        int err;

        stream.next_in = const_cast<Bytef*>(static_cast<const Bytef*>(pUncompressed));
        stream.next_out = reinterpret_cast<Bytef*>(pCompressed);


        stream.avail_in = static_cast<uInt>(nSrcSize);
        stream.avail_out = static_cast<uInt>(*pDestSize);

        stream.zalloc = &ZipDirStructuresInternal::ZlibAlloc;
        stream.zfree = &ZipDirStructuresInternal::ZlibFree;
        stream.opaque = &AZ::AllocatorInstance<AZ::OSAllocator>::Get();

        err = deflateInit2(&stream, nLevel, Z_DEFLATED, -MAX_WBITS, 9, Z_DEFAULT_STRATEGY);
        if (err != Z_OK)
        {
            return err;
        }

        err = deflate(&stream, Z_FINISH);
        if (err != Z_STREAM_END)
        {
            deflateEnd(&stream);
            return err == Z_OK ? Z_BUF_ERROR : err;
        }
        *pDestSize = stream.total_out;

        err = deflateEnd(&stream);
        return err;
    }

    int ZipRawCompressZSTD(const void* pUncompressed, size_t* pDestSize, void* pCompressed, size_t nSrcSize, [[maybe_unused]] int nLevel)
    {
        size_t result = ZSTD_compress(pCompressed, *pDestSize, pUncompressed, nSrcSize, 1);

        int err = Z_OK;

        if (ZSTD_isError(result))
        {
            AZ_Error("Error compressing using zstd:%s", false, ZSTD_getErrorName(result));
            err = Z_BUF_ERROR;
        }
        else
        {
            *pDestSize = static_cast<size_t>(result);
        }
        return err;
    }

    int ZipRawCompressLZ4(const void* pUncompressed, size_t* pDestSize, void* pCompressed, size_t nSrcSize, [[maybe_unused]] int nLevel)
    {
        int returnCode = Z_OK;
        const size_t compressedBufferMaxSize = aznumeric_caster(*pDestSize);
        size_t lz4_code = LZ4F_compressFrame(pCompressed, compressedBufferMaxSize, pUncompressed, aznumeric_caster(nSrcSize), nullptr);

        if (LZ4F_isError(lz4_code))
        {
            returnCode = Z_BUF_ERROR;
        }
        else
        {
            *pDestSize = aznumeric_caster(lz4_code);
        }

        return returnCode;
    }


    // finds the subdirectory entry by the name, using the names from the name pool
    // assumes: all directories are sorted in alphabetical order.
    // case-sensitive (must be lower-case if case-insensitive search in Win32 is performed)
    DirEntry* DirHeader::FindSubdirEntry(AZStd::string_view szName)
    {
        if (this->numDirs)
        {
            const char* pNamePool = GetNamePool();
            DirEntrySortPred pred(pNamePool);
            DirEntry* pBegin = GetSubdirEntry(0);
            DirEntry* pEnd = pBegin + this->numDirs;
            DirEntry* pEntry = AZStd::lower_bound(pBegin, pEnd, szName, pred);
#if AZ_TRAIT_LEGACY_CRYPAK_UNIX_LIKE_FILE_SYSTEM
            AZ::IO::PathView searchPath(szName, AZ::IO::WindowsPathSeparator);
            AZ::IO::PathView entryPath(pEntry->GetName(pNamePool), AZ::IO::WindowsPathSeparator);
            if (pEntry != pEnd && searchPath == entryPath)
#else
            if (pEntry != pEnd && szName == pEntry->GetName(pNamePool))
#endif
            {
                return pEntry;
            }
        }
        return {};
    }

    // finds the file entry by the name, using the names from the name pool
    // assumes: all directories are sorted in alphabetical order.
    // case-sensitive (must be lower-case if case-insensitive search in Win32 is performed)
    FileEntry* DirHeader::FindFileEntry(AZStd::string_view szName)
    {
        if (this->numFiles)
        {
            const char* pNamePool = GetNamePool();
            DirEntrySortPred pred(pNamePool);
            FileEntry* pBegin = GetFileEntry(0);
            FileEntry* pEnd = pBegin + this->numFiles;
            FileEntry* pEntry = AZStd::lower_bound(pBegin, pEnd, szName, pred);
#if AZ_TRAIT_LEGACY_CRYPAK_UNIX_LIKE_FILE_SYSTEM
            AZ::IO::PathView searchPath(szName, AZ::IO::WindowsPathSeparator);
            AZ::IO::PathView entryPath(pEntry->GetName(pNamePool), AZ::IO::WindowsPathSeparator);
            if (pEntry != pEnd && searchPath == entryPath)
#else
            if (pEntry != pEnd && szName == pEntry->GetName(pNamePool))
#endif
            {
                return pEntry;
            }
        }
        return {};
    }


    // tries to refresh the file entry from the given file (reads from there if needed)
    // returns the error code if the operation was impossible to complete
    ErrorEnum Refresh(CZipFile* f, FileEntryBase* pFileEntry)
    {
        if (pFileEntry->nFileDataOffset != pFileEntry->INVALID_DATA_OFFSET)
        {
            return ZD_ERROR_SUCCESS;
        }

        if (FSeek(f, pFileEntry->nFileHeaderOffset, SEEK_SET))
        {
            return ZD_ERROR_IO_FAILED;
        }

        // No validation in release or profile!

        // read the local file header and the name (for validation) into the buffer
        ZipFile::LocalFileHeader fileHeader;
        //if (1 != fread (&fileHeader, sizeof(fileHeader), 1, f))
        if (1 != FRead(f, &fileHeader, sizeof(fileHeader), 1))
        {
            return ZD_ERROR_IO_FAILED;
        }

        if (fileHeader.desc != pFileEntry->desc
            || fileHeader.nMethod != pFileEntry->nMethod)
        {
            AZ_Fatal("Pak", "ERROR: File header doesn't match previously cached file entry record (%s) \n fileheader desc=(%d,%d,%d), method=%d, \n fileentry desc=(%d,%d,%d),method=%d",
                "Unknown" /*f->_tmpfname*/,
                fileHeader.desc.lCRC32, fileHeader.desc.lSizeCompressed, fileHeader.desc.lSizeUncompressed,
                fileHeader.nMethod,
                pFileEntry->desc.lCRC32, pFileEntry->desc.lSizeCompressed, pFileEntry->desc.lSizeUncompressed,
                pFileEntry->nMethod);

            return ZD_ERROR_IO_FAILED;
            //CryFatalError(szErrDesc);
            //THROW_ZIPDIR_ERROR(ZD_ERROR_VALIDATION_FAILED,szErrDesc);
        }
        pFileEntry->nFileDataOffset = pFileEntry->nFileHeaderOffset + sizeof(ZipFile::LocalFileHeader) + fileHeader.nFileNameLength + fileHeader.nExtraFieldLength;
        pFileEntry->nEOFOffset = pFileEntry->nFileDataOffset + pFileEntry->desc.lSizeCompressed;


        return ZD_ERROR_SUCCESS;
    }


    // writes into the file local header (NOT including the name, only the header structure)
    // the file must be opened both for reading and writing
    ErrorEnum UpdateLocalHeader(AZ::IO::HandleType fileHandle, FileEntryBase* pFileEntry)
    {

        if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(fileHandle, pFileEntry->nFileHeaderOffset, AZ::IO::SeekType::SeekFromStart))
        {
            return ZD_ERROR_IO_FAILED;
        }

        ZipFile::LocalFileHeader header;
        if (!AZ::IO::FileIOBase::GetDirectInstance()->Read(fileHandle, &header, sizeof(header), true))
        {
            return ZD_ERROR_IO_FAILED;
        }

        AZ_Assert(header.lSignature == header.SIGNATURE, "header signature %u does not match signature %u", header.lSignature, header.SIGNATURE);

        header.desc.lCRC32 = pFileEntry->desc.lCRC32;
        header.desc.lSizeCompressed = pFileEntry->desc.lSizeCompressed;
        header.desc.lSizeUncompressed = pFileEntry->desc.lSizeUncompressed;
        header.nMethod = pFileEntry->nMethod;
        header.nFlags &= ~ZipFile::GPF_ENCRYPTED; // we don't support encrypted files

        if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(fileHandle, pFileEntry->nFileHeaderOffset, AZ::IO::SeekType::SeekFromStart))
        {
            return ZD_ERROR_IO_FAILED;
        }

        if (!AZ::IO::FileIOBase::GetDirectInstance()->Write(fileHandle, &header, sizeof(header)))
        {
            return ZD_ERROR_IO_FAILED;
        }

        return ZD_ERROR_SUCCESS;
    }


    // writes into the file local header - without Extra data
    // puts the new offset to the file data to the file entry
    // in case of error can put INVALID_DATA_OFFSET into the data offset field of file entry
    ErrorEnum WriteLocalHeader(AZ::IO::HandleType fileHandle, FileEntryBase* pFileEntry, AZStd::string_view szRelativePath)
    {
        size_t nFileNameLength = szRelativePath.size();
        size_t nHeaderSize = sizeof(ZipFile::LocalFileHeader) + nFileNameLength;

        pFileEntry->nFileDataOffset = aznumeric_cast<uint32_t>(pFileEntry->nFileHeaderOffset + nHeaderSize);
        pFileEntry->nEOFOffset = pFileEntry->nFileDataOffset + pFileEntry->desc.lSizeCompressed;

        if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(fileHandle, pFileEntry->nFileHeaderOffset, AZ::IO::SeekType::SeekFromStart))
        {
            return ZD_ERROR_IO_FAILED;
        }

        ZipFile::LocalFileHeader header;
        header.lSignature = ZipFile::LocalFileHeader::SIGNATURE;
        header.nVersionNeeded = 10;
        header.nFlags = 0;
        header.nMethod = pFileEntry->nMethod;
        header.nLastModDate = pFileEntry->nLastModDate;
        header.nLastModTime = pFileEntry->nLastModTime;
        header.desc = pFileEntry->desc;
        header.nFileNameLength = aznumeric_cast<uint16_t>(nFileNameLength);
        header.nExtraFieldLength = 0;

        if (!AZ::IO::FileIOBase::GetDirectInstance()->Write(fileHandle, &header, sizeof(header)))
        {
            return ZD_ERROR_IO_FAILED;
        }

        if (nFileNameLength > 0)
        {
            if (!AZ::IO::FileIOBase::GetDirectInstance()->Write(fileHandle, szRelativePath.data(), nFileNameLength))
            {
                return ZD_ERROR_IO_FAILED;
            }
        }

        return ZD_ERROR_SUCCESS;
    }


    // conversion routines for the date/time fields used in Zip
    uint16_t DOSDate(tm* t)
    {
        return static_cast<uint16_t>(
            ((t->tm_year - 80) << 9)
            | (t->tm_mon << 5)
            | t->tm_mday);
    }

    uint16_t DOSTime(tm* t)
    {
        return static_cast<uint16_t>(
            ((t->tm_hour) << 11)
            | ((t->tm_min) << 5)
            | ((t->tm_sec) >> 1));
    }

    // sets the current time to modification time
    // calculates CRC32 for the new data
    void FileEntry::OnNewFileData(const void* pUncompressed, uint64_t nSize, uint64_t nCompressedSize, uint32_t nCompressionMethod, bool bContinuous)
    {
        time_t nTime;
        time(&nTime);
        tm t;
        azlocaltime(&nTime, &t);

        // While local time converts the month to a 0 to 11 interval...
        // ...the pack file expects months from 1 to 12...
        // Therefore, for correct date, we have to do t->tm_mon+=1;
        t.tm_mon += 1;

        this->nLastModTime = DOSTime(&t);
        this->nLastModDate = DOSDate(&t);

        if (!bContinuous)
        {
            this->desc.lSizeCompressed = aznumeric_cast<uint32_t>(nCompressedSize);
            this->desc.lSizeUncompressed = aznumeric_cast<uint32_t>(nSize);
        }

        // we'll need CRC32 of the file to pack it
        this->desc.lCRC32 = AZ::Crc32(pUncompressed, nSize);

        this->nMethod = static_cast<uint16_t>(nCompressionMethod);
    }

    uint64_t FileEntry::GetModificationTime()
    {
        int year = (nLastModDate >> 9) + 1980;
        int month = ((nLastModDate >> 5) & 0xF);
        int day = (nLastModDate & 0x1F);
        int hour = (nLastModTime >> 11);
        int minute = (nLastModTime >> 5) & 0x3F;
        int second = (nLastModTime << 1) & 0x3F;

        constexpr int YearLengths[2] = { 365, 366 };
        constexpr int MonthLengths[2][12] =
        {
            { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
            { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
        };

        auto IsLeapYear = [](int Year)
        {
            return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0) ? 1 : 0;
        };

        // Add seconds overflow above a minute to the minute value
        minute += second / 60;
        second = second % 60;
        // Add minute overflow above an hour to the hour value
        hour += minute / 60;
        minute = minute % 60;
        // Add hour overflow above a day to the day value
        day += hour / 24;
        hour = hour % 24;
        // Add days overflow above the length of the current month to a new month 
        while (day > MonthLengths[IsLeapYear(year)][(month % 12) - 1])
        {
            day -= MonthLengths[IsLeapYear(year)][(month % 12) - 1];
            ++month;
        }
        // Add months overflow above the a year to the year value
        year += month / 12;
        month = month % 12;

        uint64_t filetime{};
        constexpr int EpochYear = 1601;
        for (int currYear = EpochYear; currYear < year; ++currYear)
        {
            filetime += YearLengths[IsLeapYear(currYear)];
        }
        for (int currMonth = 1; currMonth < month; ++currMonth)
        {
            filetime += MonthLengths[IsLeapYear(year)][currMonth - 1];
        }
        filetime += day - 1;
        // Convert days to second
        filetime *= 24 * 60 * 60;
        filetime += hour * (60 * 60) + minute * 60 + second;

        AZStd::chrono::seconds filetimeSeconds(filetime);

        // Returns time in units of second * 10^-7
        using ModTimeUnits = AZStd::chrono::duration<AZStd::sys_time_t, AZStd::ratio<10000000>>;
        return AZStd::chrono::duration_cast<ModTimeUnits>(filetimeSeconds).count();
    }

    const char* DOSTimeCStr(uint16_t nTime)
    {
        static char szBuf[16];
        azsnprintf(szBuf, AZ_ARRAY_SIZE(szBuf), "%02d:%02d.%02d", (nTime >> 11), ((nTime & ((1 << 11) - 1)) >> 5), ((nTime & ((1 << 5) - 1)) << 1));
        return szBuf;
    }

    const char* DOSDateCStr(uint16_t nTime)
    {
        static char szBuf[32];
        azsnprintf(szBuf, AZ_ARRAY_SIZE(szBuf), "%02d.%02d.%04d", (nTime & 0x1F), (nTime >> 5) & 0xF, (nTime >> 9) + 1980);
        return szBuf;
    }

    const char* Error::getError()
    {
        switch (this->nError)
        {
#define DECLARE_ERROR(x) case ZD_ERROR_##x: \
    return #x;
            DECLARE_ERROR(SUCCESS);
            DECLARE_ERROR(IO_FAILED);
            DECLARE_ERROR(UNEXPECTED);
            DECLARE_ERROR(UNSUPPORTED);
            DECLARE_ERROR(INVALID_SIGNATURE);
            DECLARE_ERROR(ZIP_FILE_IS_CORRUPT);
            DECLARE_ERROR(DATA_IS_CORRUPT);
            DECLARE_ERROR(NO_CDR);
            DECLARE_ERROR(CDR_IS_CORRUPT);
            DECLARE_ERROR(NO_MEMORY);
            DECLARE_ERROR(VALIDATION_FAILED);
            DECLARE_ERROR(CRC32_CHECK);
            DECLARE_ERROR(ZLIB_FAILED);
            DECLARE_ERROR(ZLIB_CORRUPTED_DATA);
            DECLARE_ERROR(ZLIB_NO_MEMORY);
            DECLARE_ERROR(CORRUPTED_DATA);
            DECLARE_ERROR(INVALID_CALL);
            DECLARE_ERROR(NOT_IMPLEMENTED);
            DECLARE_ERROR(FILE_NOT_FOUND);
            DECLARE_ERROR(DIR_NOT_FOUND);
            DECLARE_ERROR(NAME_TOO_LONG);
            DECLARE_ERROR(INVALID_PATH);
            DECLARE_ERROR(FILE_ALREADY_EXISTS);
#undef DECLARE_ERROR
        default:
            return "Unknown ZD_ERROR code";
        }
    }

    //////////////////////////////////////////////////////////////////////////

    int64_t FSeek(CZipFile* file, int64_t origin, int command)
    {
        if (file->IsInMemory())
        {
            int64_t retCode = -1;
            int64_t newPos;
            switch (command)
            {
            case SEEK_SET:
                newPos = origin;
                if (newPos >= 0 && newPos <= file->m_nSize)
                {
                    file->m_nCursor = newPos;
                    retCode = 0;
                }
                break;
            case SEEK_CUR:
                newPos = origin + file->m_nCursor;
                if (newPos >= 0 && newPos <= file->m_nSize)
                {
                    file->m_nCursor = newPos;
                    retCode = 0;
                }
                break;
            case SEEK_END:
                newPos = file->m_nSize - origin;
                if (newPos >= 0 && newPos <= file->m_nSize)
                {
                    file->m_nCursor = newPos;
                    retCode = 0;
                }
                break;
            default:
                // Not valid disk operation!
                AZ_Assert(false, "Invalid seek mode %d supplied to FSeek", command);
            }
            return retCode;
        }
        else
        {
            if (AZ::IO::FileIOBase::GetDirectInstance()->Seek(file->m_fileHandle, origin, AZ::IO::GetSeekTypeFromFSeekMode(command)))
            {
                return 0;
            }
            return 1;
        }
    }

    int64_t FRead(CZipFile* file, void* data, size_t elementSize, size_t count)
    {
        if (file->IsInMemory())
        {
            int64_t nRead = count * elementSize;
            int64_t nCanBeRead = file->m_nSize - file->m_nCursor;
            if (nRead > nCanBeRead)
            {
                nRead = nCanBeRead;
            }

            if (file->m_nCursor + nRead <= aznumeric_cast<int64_t>(file->m_pInMemoryData->m_size))
            {
                memcpy(data, reinterpret_cast<uint8_t*>(&file->m_pInMemoryData->m_address[file->m_nCursor]), nRead);
            }
            file->m_nCursor += nRead;
            return nRead / elementSize;
        }
        else
        {
            AZ::IO::HandleType fileHandle = file->m_fileHandle;
            AZ::u64 bytesRead = 0;
            AZ::IO::FileIOBase::GetDirectInstance()->Read(fileHandle, data, elementSize * count, false, &bytesRead);
            return bytesRead / elementSize;
        }
    }

    int64_t FTell(CZipFile* file)
    {
        if (file->IsInMemory())
        {
            return file->m_nCursor;
        }
        else
        {
            AZ::u64 tellResult = 0;
            AZ::IO::FileIOBase::GetDirectInstance()->Tell(file->m_fileHandle, tellResult);
            return tellResult;
        }
    }

    int FEof(CZipFile* zipFile)
    {
        if (zipFile->IsInMemory())
        {
            return zipFile->m_nCursor == zipFile->m_nSize;
        }
        else
        {
            return AZ::IO::FileIOBase::GetDirectInstance()->Eof(zipFile->m_fileHandle);
        }
    }
}
