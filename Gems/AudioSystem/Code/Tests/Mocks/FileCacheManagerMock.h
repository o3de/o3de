/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <FileCacheManager.h>

#include <AzTest/AzTest.h>
#include <IRenderAuxGeom.h>

namespace Audio
{
    class FileCacheManagerMock
        : public CFileCacheManager
    {
    public:
        explicit FileCacheManagerMock(TATLPreloadRequestLookup& preloadLookup)
            : CFileCacheManager(preloadLookup)
        {}

        MOCK_METHOD0(Initialize, void());
        MOCK_METHOD0(Release, void());
        MOCK_METHOD0(Update, void());

        MOCK_METHOD3(TryAddFileCacheEntry, TAudioFileEntryID(const AZ::rapidxml::xml_node<char>*, EATLDataScope, bool));
        MOCK_METHOD2(TryRemoveFileCacheEntry, bool(const TAudioFileEntryID, const EATLDataScope));

        MOCK_METHOD0(UpdateLocalizedFileCacheEntries, void());

        MOCK_METHOD3(DrawDebugInfo, void(IRenderAuxGeom&, const float, const float));

        MOCK_METHOD3(TryLoadRequest, EAudioRequestStatus(const TAudioPreloadRequestID, const bool, const bool));
        MOCK_METHOD1(TryUnloadRequest, EAudioRequestStatus(const TAudioPreloadRequestID));
        MOCK_METHOD1(UnloadDataByScope, EAudioRequestStatus(const EATLDataScope));

    private:

        MOCK_METHOD2(AllocateHeap, void(const size_t, const char* const));
        MOCK_METHOD3(UncacheFileCacheEntryInternal, bool(CATLAudioFileEntry* const, const bool, const bool));
        MOCK_METHOD1(DoesRequestFitInternal, bool(const size_t));
        MOCK_METHOD0(UpdatePreloadRequestStatus, void());
        MOCK_METHOD3(FinishCachingFileInternal, bool(CATLAudioFileEntry* const, AZ::IO::SizeType, AZ::IO::IStreamerTypes::RequestStatus));

        MOCK_METHOD1(FinishAsyncStreamRequest, void(AZ::IO::FileRequestHandle));
        
        MOCK_METHOD1(AllocateMemoryBlockInternal, bool(CATLAudioFileEntry* const));
        MOCK_METHOD1(UncacheFile, void(CATLAudioFileEntry* const));
        MOCK_METHOD0(TryToUncacheFiles, void());
        MOCK_METHOD1(UpdateLocalizedFileEntryData, void(CATLAudioFileEntry* const));
        MOCK_METHOD5(TryCacheFileCacheEntryInternal, bool(CATLAudioFileEntry* const, const TAudioFileEntryID, const bool, const bool, const size_t));
    };

} // namespace Audio
