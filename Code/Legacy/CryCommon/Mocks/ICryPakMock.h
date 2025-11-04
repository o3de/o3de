/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzFramework/Archive/ArchiveVars.h>
#include <AzFramework/Archive/INestedArchive.h>
#include <AzFramework/Archive/IArchive.h>
#include <CryCommon/platform.h>

struct CryPakMock
    : AZ::IO::IArchive
{
    MOCK_METHOD5(AdjustFileName, const char*(AZStd::string_view src, char* dst, size_t dstSize, uint32_t nFlags, bool skipMods));
    MOCK_METHOD1(Init, bool(AZStd::string_view szBasePath));
    MOCK_METHOD0(Release, void());

    MOCK_METHOD4(OpenPack, bool(AZStd::string_view, AZStd::intrusive_ptr<AZ::IO::MemoryBlock>, AZ::IO::FixedMaxPathString*, bool));
    MOCK_METHOD5(OpenPack, bool(AZStd::string_view, AZStd::string_view, AZStd::intrusive_ptr<AZ::IO::MemoryBlock>, AZ::IO::FixedMaxPathString*, bool));
    MOCK_METHOD2(OpenPacks, bool(AZStd::string_view pWildcard, AZStd::vector<AZ::IO::FixedMaxPathString>* pFullPaths));
    MOCK_METHOD3(OpenPacks, bool(AZStd::string_view pBindingRoot, AZStd::string_view pWildcard, AZStd::vector<AZ::IO::FixedMaxPathString>* pFullPaths));
    MOCK_METHOD1(ClosePack, bool(AZStd::string_view pName));
    MOCK_METHOD1(ClosePacks, bool(AZStd::string_view pWildcard));
    MOCK_METHOD1(FindPacks, bool(AZStd::string_view pWildcardIn));
    MOCK_METHOD2(SetPacksAccessible, bool(bool bAccessible, AZStd::string_view pWildcard));
    MOCK_METHOD2(SetPackAccessible, bool(bool bAccessible, AZStd::string_view pName));
    MOCK_METHOD1(SetLocalizationFolder, void(AZStd::string_view sLocalizationFolder));
    MOCK_CONST_METHOD0(GetLocalizationFolder, const char*());
    MOCK_CONST_METHOD0(GetLocalizationRoot, const char*());
    MOCK_METHOD2(FOpen, AZ::IO::HandleType(AZStd::string_view pName, const char* mode));
    MOCK_METHOD2(FGetCachedFileData, void*(AZ::IO::HandleType handle, size_t& nFileSize));
    MOCK_METHOD3(FRead, size_t(void* data, size_t bytesToRead, AZ::IO::HandleType handle));
    MOCK_METHOD3(FWrite, size_t(const void* data, size_t bytesToWrite, AZ::IO::HandleType handle));
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
    MOCK_METHOD2(FindFirst, AZ::IO::ArchiveFileIterator(AZStd::string_view pDir, AZ::IO::FileSearchLocation));
    MOCK_METHOD1(FindNext, AZ::IO::ArchiveFileIterator(AZ::IO::ArchiveFileIterator handle));
    MOCK_METHOD1(FindClose, bool(AZ::IO::ArchiveFileIterator));
    MOCK_METHOD1(GetModificationTime, AZ::IO::IArchive::FileTime(AZ::IO::HandleType f));
    MOCK_METHOD2(IsFileExist, bool(AZStd::string_view sFilename, AZ::IO::FileSearchLocation));
    MOCK_METHOD1(IsFolder, bool(AZStd::string_view sPath));
    MOCK_METHOD1(GetFileSizeOnDisk, AZ::IO::IArchive::SignedFileSize(AZStd::string_view filename));
    MOCK_METHOD4(OpenArchive, AZStd::intrusive_ptr<AZ::IO::INestedArchive> (AZStd::string_view szPath, AZStd::string_view bindRoot, uint32_t nFlags, AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pData));
    MOCK_METHOD1(GetFileArchivePath, AZ::IO::PathView (AZ::IO::HandleType f));
    MOCK_METHOD5(RawCompress, int(const void* pUncompressed, size_t* pDestSize, void* pCompressed, size_t nSrcSize, int nLevel));
    MOCK_METHOD4(RawUncompress, int(void* pUncompressed, size_t* pDestSize, const void* pCompressed, size_t nSrcSize));
    MOCK_METHOD1(RecordFileOpen, void(ERecordFileOpenList eList));
    MOCK_METHOD2(RecordFile, void(AZ::IO::HandleType in, AZStd::string_view szFilename));
    MOCK_METHOD1(GetResourceList, AZ::IO::IResourceList * (ERecordFileOpenList eList));
    MOCK_METHOD2(SetResourceList, void(ERecordFileOpenList eList, AZ::IO::IResourceList * pResourceList));
    MOCK_METHOD0(GetRecordFileOpenList, AZ::IO::IArchive::ERecordFileOpenList());
    MOCK_METHOD1(RegisterFileAccessSink, void(AZ::IO::IArchiveFileAccessSink * pSink));
    MOCK_METHOD1(UnregisterFileAccessSink, void(AZ::IO::IArchiveFileAccessSink * pSink));
    MOCK_METHOD1(DisableRuntimeFileAccess, void(bool status));
    MOCK_METHOD2(DisableRuntimeFileAccess, bool(bool status, AZStd::thread_id threadId));
    MOCK_CONST_METHOD0(GetPakPriority, AZ::IO::FileSearchPriority());
    MOCK_CONST_METHOD1(GetFileOffsetOnMedia, uint64_t(AZStd::string_view szName));
    MOCK_CONST_METHOD1(GetFileMediaType, EStreamSourceMediaType(AZStd::string_view szName));
    MOCK_METHOD0(GetLevelPackOpenEvent, LevelPackOpenEvent*());
    MOCK_METHOD0(GetLevelPackCloseEvent, LevelPackCloseEvent*());

};

