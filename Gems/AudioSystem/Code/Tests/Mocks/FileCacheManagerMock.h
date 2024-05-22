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

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace AZ::IO
{
    class FileRequestHandle;
}

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

        MOCK_METHOD3(DrawDebugInfo, void(AzFramework::DebugDisplayRequests&, const float, const float));

        MOCK_METHOD3(TryLoadRequest, EAudioRequestStatus(const TAudioPreloadRequestID, const bool, const bool));
        MOCK_METHOD1(TryUnloadRequest, EAudioRequestStatus(const TAudioPreloadRequestID));
        MOCK_METHOD1(UnloadDataByScope, EAudioRequestStatus(const EATLDataScope));
    };

} // namespace Audio
