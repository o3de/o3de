/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/Archive/INestedArchive.h>
#include <AzFramework/Archive/IArchive.h>


struct CryPakMock
    : AZ::IO::IArchive
{
    MOCK_METHOD5(AdjustFileName, const char*(AZStd::string_view src, char* dst, size_t dstSize, uint32_t nFlags, bool skipMods));
    MOCK_METHOD1(Init, bool(AZStd::string_view szBasePath));
    MOCK_METHOD0(Release, void());
    MOCK_CONST_METHOD1(IsInstalledToHDD, bool(AZStd::string_view acFilePath));

    MOCK_METHOD5(OpenPack, bool(AZStd::string_view, uint32_t, AZStd::intrusive_ptr<AZ::IO::MemoryBlock>, AZStd::fixed_string<AZ::IO::IArchive::MaxPath>*, bool));
    MOCK_METHOD6(OpenPack, bool(AZStd::string_view, AZStd::string_view, uint32_t, AZStd::intrusive_ptr<AZ::IO::MemoryBlock>, AZStd::fixed_string<AZ::IO::IArchive::MaxPath>*, bool));
    MOCK_METHOD3(OpenPacks, bool(AZStd::string_view pWildcard, uint32_t nFlags, AZStd::vector<AZStd::fixed_string<AZ::IO::IArchive::MaxPath>>* pFullPaths));
    MOCK_METHOD4(OpenPacks, bool(AZStd::string_view pBindingRoot, AZStd::string_view pWildcard, uint32_t nFlags, AZStd::vector<AZStd::fixed_string<AZ::IO::IArchive::MaxPath>>* pFullPaths));
    MOCK_METHOD2(ClosePack, bool(AZStd::string_view pName, uint32_t nFlags));
    MOCK_METHOD2(ClosePacks, bool(AZStd::string_view pWildcard, uint32_t nFlags));
    MOCK_METHOD1(FindPacks, bool(AZStd::string_view pWildcardIn));
    MOCK_METHOD3(SetPacksAccessible, bool(bool bAccessible, AZStd::string_view pWildcard, uint32_t nFlags));
    MOCK_METHOD3(SetPackAccessible, bool(bool bAccessible, AZStd::string_view pName, uint32_t nFlags));
    MOCK_METHOD3(LoadPakToMemory, bool(AZStd::string_view pName, EInMemoryArchiveLocation eLoadToMemory, AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pMemoryBlock));
    MOCK_METHOD2(LoadPaksToMemory, void(int nMaxPakSize, bool bLoadToMemory));
    MOCK_METHOD1(GetMod, const char*(int index));
    MOCK_METHOD1(ParseAliases, void(AZStd::string_view szCommandLine));
    MOCK_METHOD3(SetAlias, void(AZStd::string_view szName, AZStd::string_view szAlias, bool bAdd));
    MOCK_METHOD2(GetAlias, const char*(AZStd::string_view szName, bool bReturnSame));
    MOCK_METHOD0(Lock, void());
    MOCK_METHOD0(Unlock, void());
    MOCK_METHOD1(SetLocalizationFolder, void(AZStd::string_view sLocalizationFolder));
    MOCK_CONST_METHOD0(GetLocalizationFolder, const char*());
    MOCK_CONST_METHOD0(GetLocalizationRoot, const char*());
    MOCK_METHOD3(FOpen, AZ::IO::HandleType(AZStd::string_view pName, const char* mode, uint32_t nFlags));
    MOCK_METHOD4(FReadRaw, size_t(void* data, size_t length, size_t elems, AZ::IO::HandleType handle));
    MOCK_METHOD3(FReadRawAll, size_t(void* data, size_t nFileSize, AZ::IO::HandleType handle));
    MOCK_METHOD2(FGetCachedFileData, void*(AZ::IO::HandleType handle, size_t & nFileSize));
    MOCK_METHOD4(FWrite, size_t(const void* data, size_t length, size_t elems, AZ::IO::HandleType handle));
    MOCK_METHOD3(FGets, char*(char*, int, AZ::IO::HandleType));
    MOCK_METHOD1(Getc, int(AZ::IO::HandleType));
    MOCK_METHOD1(FGetSize, size_t(AZ::IO::HandleType f));
    MOCK_METHOD2(FGetSize, size_t(AZStd::string_view pName, bool bAllowUseFileSystem));
    MOCK_METHOD1(IsInPak, bool(AZ::IO::HandleType handle));
    MOCK_METHOD1(RemoveFile, bool(AZStd::string_view pName));
    MOCK_METHOD1(RemoveDir, bool(AZStd::string_view pName));
    MOCK_METHOD1(IsAbsPath, bool(AZStd::string_view pPath));
    MOCK_METHOD3(FSeek, size_t(AZ::IO::HandleType handle, uint64_t seek, int mode));
    MOCK_METHOD1(FTell, uint64_t(AZ::IO::HandleType handle));
    MOCK_METHOD1(FClose, int(AZ::IO::HandleType handle));
    MOCK_METHOD1(FEof, int(AZ::IO::HandleType handle));
    MOCK_METHOD1(FFlush, int(AZ::IO::HandleType handle));
    MOCK_METHOD1(PoolMalloc, void*(size_t size));
    MOCK_METHOD1(PoolFree, void(void* p));
    MOCK_METHOD3(PoolAllocMemoryBlock, AZStd::intrusive_ptr<AZ::IO::MemoryBlock> (size_t nSize, const char* sUsage, size_t nAlign));
    MOCK_METHOD2(FindFirst, AZ::IO::ArchiveFileIterator(AZStd::string_view pDir, AZ::IO::IArchive::EFileSearchType));
    MOCK_METHOD1(FindNext, AZ::IO::ArchiveFileIterator(AZ::IO::ArchiveFileIterator handle));
    MOCK_METHOD1(FindClose, bool(AZ::IO::ArchiveFileIterator));
    MOCK_METHOD1(GetModificationTime, AZ::IO::IArchive::FileTime(AZ::IO::HandleType f));
    MOCK_METHOD2(IsFileExist, bool(AZStd::string_view sFilename, EFileSearchLocation));
    MOCK_METHOD1(IsFolder, bool(AZStd::string_view sPath));
    MOCK_METHOD1(GetFileSizeOnDisk, AZ::IO::IArchive::SignedFileSize(AZStd::string_view filename));
    MOCK_METHOD2(MakeDir, bool(AZStd::string_view szPath, bool bGamePathMapping));
    MOCK_METHOD4(OpenArchive, AZStd::intrusive_ptr<AZ::IO::INestedArchive> (AZStd::string_view szPath, AZStd::string_view bindRoot, uint32_t nFlags, AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pData));
    MOCK_METHOD1(GetFileArchivePath, const char* (AZ::IO::HandleType f));
    MOCK_METHOD5(RawCompress, int(const void* pUncompressed, size_t* pDestSize, void* pCompressed, size_t nSrcSize, int nLevel));
    MOCK_METHOD4(RawUncompress, int(void* pUncompressed, size_t* pDestSize, const void* pCompressed, size_t nSrcSize));
    MOCK_METHOD1(RecordFileOpen, void(ERecordFileOpenList eList));
    MOCK_METHOD2(RecordFile, void(AZ::IO::HandleType in, AZStd::string_view szFilename));
    MOCK_METHOD1(GetResourceList, AZ::IO::IResourceList * (ERecordFileOpenList eList));
    MOCK_METHOD2(SetResourceList, void(ERecordFileOpenList eList, AZ::IO::IResourceList * pResourceList));
    MOCK_METHOD0(GetRecordFileOpenList, AZ::IO::IArchive::ERecordFileOpenList());
    MOCK_METHOD2(ComputeCRC, uint32_t(AZStd::string_view szPath, uint32_t nFileOpenFlags));
    MOCK_METHOD4(ComputeMD5, bool(AZStd::string_view szPath, uint8_t* md5, uint32_t nFileOpenFlags, bool useDirectAccess));
    MOCK_METHOD1(RegisterFileAccessSink, void(AZ::IO::IArchiveFileAccessSink * pSink));
    MOCK_METHOD1(UnregisterFileAccessSink, void(AZ::IO::IArchiveFileAccessSink * pSink));
    MOCK_METHOD1(DisableRuntimeFileAccess, void(bool status));
    MOCK_METHOD2(DisableRuntimeFileAccess, bool(bool status, AZStd::thread_id threadId));
    MOCK_METHOD2(CheckFileAccessDisabled, bool(AZStd::string_view name, const char* mode));
    MOCK_METHOD1(SetRenderThreadId, void(AZStd::thread_id renderThreadId));
    MOCK_CONST_METHOD0(GetPakPriority, AZ::IO::ArchiveLocationPriority());
    MOCK_CONST_METHOD1(GetFileOffsetOnMedia, uint64_t(AZStd::string_view szName));
    MOCK_CONST_METHOD1(GetFileMediaType, EStreamSourceMediaType(AZStd::string_view szName));
    MOCK_METHOD0(GetLevelPackOpenEvent, auto()->LevelPackOpenEvent*);
    MOCK_METHOD0(GetLevelPackCloseEvent, auto()->LevelPackCloseEvent*);

    // Implementations required for variadic functions
    virtual int FPrintf([[maybe_unused]] AZ::IO::HandleType handle, [[maybe_unused]] const char* format, ...) PRINTF_PARAMS(3, 4)
    {
        return 0;
    }
};

