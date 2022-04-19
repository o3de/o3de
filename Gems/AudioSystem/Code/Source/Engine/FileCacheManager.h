/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/XML/rapidxml.h>

#include <IAudioInterfacesCommonData.h>
#include <ATLEntities.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    class AudioFileCacheManagerNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioFileCacheManagerNotifications() = default;

        ///////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits - Single Bus Address, Single Handler, Mutex, Queued
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const bool EnableEventQueue = true;
        using MutexType = AZStd::recursive_mutex;
        ///////////////////////////////////////////////////////////////////////////////////////////

        virtual void FinishAsyncStreamRequest(AZ::IO::FileRequestHandle request) = 0;
    };

    using AudioFileCacheManagerNotficationBus = AZ::EBus<AudioFileCacheManagerNotifications>;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    class CFileCacheManager
        : public AudioFileCacheManagerNotficationBus::Handler
    {
    public:
        explicit CFileCacheManager(TATLPreloadRequestLookup& preloadRequests);
        ~CFileCacheManager() override;

        CFileCacheManager(const CFileCacheManager&) = delete;            // Copy protection
        CFileCacheManager& operator=(const CFileCacheManager&) = delete; // Copy protection

        // Public methods
        void Initialize();
        void Release();
        void Update();

        virtual TAudioFileEntryID TryAddFileCacheEntry(const AZ::rapidxml::xml_node<char>* fileXmlNode, EATLDataScope dataScope, bool autoLoad);  // 'virtual' is needed for unit tests/mocking
        bool TryRemoveFileCacheEntry(const TAudioFileEntryID audioFileID, const EATLDataScope dataScope);
        void UpdateLocalizedFileCacheEntries();

        EAudioRequestStatus TryLoadRequest(const TAudioPreloadRequestID preloadRequestID, const bool loadSynchronously, const bool autoLoadOnly);
        EAudioRequestStatus TryUnloadRequest(const TAudioPreloadRequestID preloadRequestID);
        EAudioRequestStatus UnloadDataByScope(const EATLDataScope dataScope);

    #if !defined(AUDIO_RELEASE)
        void DrawDebugInfo(AzFramework::DebugDisplayRequests& debugDisplay, const float posX, const float posY);
    #endif // !AUDIO_RELEASE

    private:
        // Internal type definitions.
        using TAudioFileEntries = ATLMapLookupType<TAudioFileEntryID, CATLAudioFileEntry*>;

        // Internal methods
        void AllocateHeap(const size_t size, const char* const usage);
        bool UncacheFileCacheEntryInternal(CATLAudioFileEntry* const audioFileEntry, const bool now, const bool ignoreUsedCount = false);
        bool DoesRequestFitInternal(const size_t requestSize);
        void UpdatePreloadRequestsStatus();
        bool FinishCachingFileInternal(CATLAudioFileEntry* const audioFileEntry, AZ::IO::SizeType sizeBytes,
            AZ::IO::IStreamerTypes::RequestStatus requestState);

        ///////////////////////////////////////////////////////////////////////////////////////////
        // AudioFileCacheManagerNotficationBus
        void FinishAsyncStreamRequest(AZ::IO::FileRequestHandle request) override;
        ///////////////////////////////////////////////////////////////////////////////////////////

        bool AllocateMemoryBlockInternal(CATLAudioFileEntry* const audioFileEntry);
        void UncacheFile(CATLAudioFileEntry* const audioFileEntry);
        void TryToUncacheFiles();
        void UpdateLocalizedFileEntryData(CATLAudioFileEntry* const audioFileEntry);
        bool TryCacheFileCacheEntryInternal(
            CATLAudioFileEntry* const audioFileEntry,
            const TAudioFileEntryID fileID,
            const bool loadSynchronously,
            const bool overrideUseCount = false,
            const AZ::u32 useCount = 0);

        // Internal members
        TATLPreloadRequestLookup& m_preloadRequests;
        TAudioFileEntries m_audioFileEntries;

        size_t m_currentByteTotal;
        size_t m_maxByteTotal;
    };
} // namespace Audio
