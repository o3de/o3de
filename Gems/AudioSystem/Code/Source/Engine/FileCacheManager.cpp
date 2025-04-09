/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <FileCacheManager.h>

#include <AzCore/Console/ILogger.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/IStreamer.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AudioAllocators.h>
#include <IAudioSystemImplementation.h>
#include <SoundCVars.h>
#include <AudioSystem_Traits_Platform.h>

#if !defined(AUDIO_RELEASE)
    // Debug Draw
    #include <AzFramework/Entity/EntityDebugDisplayBus.h>
#endif // !AUDIO_RELEASE

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioFileFlags : TATLEnumFlagsType
    {
        eAFF_NOTFOUND                       = AUDIO_BIT(0),
        eAFF_CACHED                         = AUDIO_BIT(1),
        eAFF_MEMALLOCFAIL                   = AUDIO_BIT(2),
        eAFF_REMOVABLE                      = AUDIO_BIT(3),
        eAFF_LOADING                        = AUDIO_BIT(4),
        eAFF_USE_COUNTED                    = AUDIO_BIT(5),
        eAFF_NEEDS_RESET_TO_MANUAL_LOADING  = AUDIO_BIT(6),
        eAFF_LOCALIZED                      = AUDIO_BIT(7),
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    CFileCacheManager::CFileCacheManager(TATLPreloadRequestLookup& preloadRequests)
        : m_preloadRequests(preloadRequests)
        , m_currentByteTotal(0)
        , m_maxByteTotal(0)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    CFileCacheManager::~CFileCacheManager()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::Initialize()
    {
        AllocateHeap(static_cast<size_t>(Audio::CVars::s_FileCacheManagerMemorySize), "AudioFileCacheManager");

        AudioFileCacheManagerNotficationBus::Handler::BusConnect();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::Release()
    {
        AudioFileCacheManagerNotficationBus::Handler::BusDisconnect();

        // Should we check here for any lingering files?
        // ATL unloads everything before getting here, but a stop-gap could be safer.
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::Update()
    {
        AZ_PROFILE_FUNCTION(Audio);

        AudioFileCacheManagerNotficationBus::ExecuteQueuedEvents();
        UpdatePreloadRequestsStatus();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::AllocateHeap(const size_t size, [[maybe_unused]] const char* const usage)
    {
        if (size > 0)
        {
            m_maxByteTotal = size << 10;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    TAudioFileEntryID CFileCacheManager::TryAddFileCacheEntry(const AZ::rapidxml::xml_node<char>* fileXmlNode, const EATLDataScope dataScope, bool autoLoad)
    {
        TAudioFileEntryID fileEntryId = INVALID_AUDIO_FILE_ENTRY_ID;
        SATLAudioFileEntryInfo fileEntryInfo;

        EAudioRequestStatus result = EAudioRequestStatus::None;
        AudioSystemImplementationRequestBus::BroadcastResult(result, &AudioSystemImplementationRequestBus::Events::ParseAudioFileEntry, fileXmlNode, &fileEntryInfo);
        if (result == EAudioRequestStatus::Success)
        {
            const char* fileLocation = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(fileLocation, &AudioSystemImplementationRequestBus::Events::GetAudioFileLocation, &fileEntryInfo);
            AZStd::string filePath;
            AZ::StringFunc::AssetDatabasePath::Join(fileLocation, fileEntryInfo.sFileName, filePath);
            auto newAudioFileEntry = azcreate(CATLAudioFileEntry, (filePath.c_str(), fileEntryInfo.pImplData), Audio::AudioSystemAllocator);

            if (newAudioFileEntry)
            {
                newAudioFileEntry->m_memoryBlockAlignment = fileEntryInfo.nMemoryBlockAlignment;

                if (fileEntryInfo.bLocalized)
                {
                    newAudioFileEntry->m_flags.AddFlags(eAFF_LOCALIZED);
                }

                fileEntryId = AudioStringToID<TAudioFileEntryID>(newAudioFileEntry->m_filePath.c_str());
                auto it = m_audioFileEntries.find(fileEntryId);

                if (it == m_audioFileEntries.end())
                {
                    if (!autoLoad)
                    {
                        // Can now be ref-counted and therefore manually unloaded.
                        newAudioFileEntry->m_flags.AddFlags(eAFF_USE_COUNTED);
                    }

                    newAudioFileEntry->m_dataScope = dataScope;
                    AZStd::to_lower(newAudioFileEntry->m_filePath.begin(), newAudioFileEntry->m_filePath.end());

                    auto fileIO = AZ::IO::FileIOBase::GetInstance();
                    if (AZ::u64 fileSize = 0;
                        fileIO->Size(newAudioFileEntry->m_filePath.c_str(), fileSize) && fileSize != 0)
                    {
                        newAudioFileEntry->m_fileSize = fileSize;
                        newAudioFileEntry->m_flags.ClearFlags(eAFF_NOTFOUND);
                    }

                    m_audioFileEntries[fileEntryId] = newAudioFileEntry;
                }
                else
                {
                    if (autoLoad && it->second->m_flags.AreAnyFlagsActive(eAFF_USE_COUNTED))
                    {
                        // This file entry is upgraded from "manual loading" to "auto loading" but needs a reset to "manual loading" again!
                        it->second->m_flags.AddFlags(eAFF_NEEDS_RESET_TO_MANUAL_LOADING);
                        it->second->m_flags.ClearFlags(eAFF_USE_COUNTED);
                        AZLOG_DEBUG("FileCacheManager - Upgraded file entry from 'Manual' to 'Auto' loading: %s", it->second->m_filePath.c_str());
                    }

                    // Entry already exists, free the memory!
                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioFileEntryData, newAudioFileEntry->m_implData);
                    azdestroy(newAudioFileEntry, Audio::AudioSystemAllocator);
                }
            }
        }

        return fileEntryId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::TryRemoveFileCacheEntry(const TAudioFileEntryID audioFileID, const EATLDataScope dataScope)
    {
        bool success = false;
        const TAudioFileEntries::iterator iter(m_audioFileEntries.find(audioFileID));

        if (iter != m_audioFileEntries.end())
        {
            CATLAudioFileEntry* const audioFileEntry = iter->second;

            if (audioFileEntry->m_dataScope == dataScope)
            {
                UncacheFileCacheEntryInternal(audioFileEntry, true, true);
                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioFileEntryData, audioFileEntry->m_implData);
                azdestroy(audioFileEntry, Audio::AudioSystemAllocator);
                m_audioFileEntries.erase(iter);
            }
            else if (dataScope == eADS_LEVEL_SPECIFIC && audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_NEEDS_RESET_TO_MANUAL_LOADING))
            {
                audioFileEntry->m_flags.AddFlags(eAFF_USE_COUNTED);
                audioFileEntry->m_flags.ClearFlags(eAFF_NEEDS_RESET_TO_MANUAL_LOADING);
                AZLOG_DEBUG("FileCacheManager - Downgraded file entry from 'Auto' to 'Manual' loading: %s", audioFileEntry->m_filePath.c_str());
            }
        }

        return success;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::UpdateLocalizedFileCacheEntries()
    {
        for (auto& audioFileEntryPair : m_audioFileEntries)
        {
            CATLAudioFileEntry* const audioFileEntry = audioFileEntryPair.second;

            if (audioFileEntry && audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_LOCALIZED))
            {
                if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_CACHED | eAFF_LOADING))
                {
                    // The file needs to be unloaded first.
                    const AZ::u32 useCount = audioFileEntry->m_useCount;
                    audioFileEntry->m_useCount = 0; // Needed to uncache without an error.
                    UncacheFile(audioFileEntry);

                    UpdateLocalizedFileEntryData(audioFileEntry);

                    TryCacheFileCacheEntryInternal(audioFileEntry, audioFileEntryPair.first, true, true, useCount);
                }
                else
                {
                    // The file is not cached or loading, it is safe to update the corresponding CATLAudioFileEntry data.
                    UpdateLocalizedFileEntryData(audioFileEntry);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CFileCacheManager::TryLoadRequest(const TAudioPreloadRequestID preloadRequestID, const bool loadSynchronously, const bool autoLoadOnly)
    {
        bool fullSuccess = false;
        bool fullFailure = true;

        auto itPreload = m_preloadRequests.find(preloadRequestID);
        if (itPreload != m_preloadRequests.end())
        {
            CATLPreloadRequest* const preloadRequest = itPreload->second;
            if (!preloadRequest->m_cFileEntryIDs.empty() && (!autoLoadOnly || (autoLoadOnly && preloadRequest->m_bAutoLoad)))
            {
                fullSuccess = true;
                for (auto fileId : preloadRequest->m_cFileEntryIDs)
                {
                    auto itFileEntry = m_audioFileEntries.find(fileId);
                    if (itFileEntry != m_audioFileEntries.end())
                    {
                        const bool tempResult = TryCacheFileCacheEntryInternal(itFileEntry->second, fileId, loadSynchronously);
                        fullSuccess = (fullSuccess && tempResult);
                        fullFailure = (fullFailure && !tempResult);
                    }
                }
            }

            if (fullSuccess && preloadRequest->m_allLoaded)
            {
                // Notify to handlers that the preload is already loaded/cached.
                AudioPreloadNotificationBus::Event(preloadRequestID, &AudioPreloadNotificationBus::Events::OnAudioPreloadCached);
            }
        }

        return (
            fullSuccess ? EAudioRequestStatus::Success
                        : (fullFailure ? EAudioRequestStatus::Failure : EAudioRequestStatus::PartialSuccess));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CFileCacheManager::TryUnloadRequest(const TAudioPreloadRequestID preloadRequestID)
    {
        bool fullSuccess = false;
        bool fullFailure = true;

        auto itPreload = m_preloadRequests.find(preloadRequestID);
        if (itPreload != m_preloadRequests.end())
        {
            CATLPreloadRequest* const preloadRequest = itPreload->second;
            if (!preloadRequest->m_cFileEntryIDs.empty())
            {
                fullSuccess = true;
                for (auto fileId : preloadRequest->m_cFileEntryIDs)
                {
                    auto itFileEntry = m_audioFileEntries.find(fileId);
                    if (itFileEntry != m_audioFileEntries.end())
                    {
                        const bool tempResult = UncacheFileCacheEntryInternal(itFileEntry->second, true);
                        fullSuccess = (fullSuccess && tempResult);
                        fullFailure = (fullFailure && !tempResult);
                    }
                }
            }

            if (fullSuccess && !preloadRequest->m_allLoaded)
            {
                // Notify to handlers the the preload is already unloaded.
                AudioPreloadNotificationBus::Event(preloadRequestID, &AudioPreloadNotificationBus::Events::OnAudioPreloadUncached);
            }
        }

        return (
            fullSuccess ? EAudioRequestStatus::Success
                        : (fullFailure ? EAudioRequestStatus::Failure : EAudioRequestStatus::PartialSuccess));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CFileCacheManager::UnloadDataByScope(const EATLDataScope dataScope)
    {
        for (auto it = m_audioFileEntries.begin(); it != m_audioFileEntries.end(); )
        {
            CATLAudioFileEntry* const audioFileEntry = it->second;

            if (audioFileEntry && audioFileEntry->m_dataScope == dataScope)
            {
                if (UncacheFileCacheEntryInternal(audioFileEntry, true, true))
                {
                    it = m_audioFileEntries.erase(it);
                    continue;
                }
            }

            ++it;
        }

        return EAudioRequestStatus::Success;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::UncacheFileCacheEntryInternal(CATLAudioFileEntry* const audioFileEntry, const bool now, const bool ignoreUsedCount /* = false */)
    {
        bool success = false;

        // In any case decrement the used count.
        if (audioFileEntry->m_useCount > 0)
        {
            --audioFileEntry->m_useCount;
        }

        if (audioFileEntry->m_useCount < 1 || ignoreUsedCount)
        {
            // Must be cached to proceed.
            if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_CACHED))
            {
                // Only "use-counted" files can become removable!
                if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_USE_COUNTED))
                {
                    audioFileEntry->m_flags.AddFlags(eAFF_REMOVABLE);
                }

                if (now || ignoreUsedCount)
                {
                    UncacheFile(audioFileEntry);
                }
            }
            else if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_LOADING | eAFF_MEMALLOCFAIL))
            {
                AZLOG_DEBUG("FileCacheManager - Trying to remove a loading or mem-failed entry '%s'", audioFileEntry->m_filePath.c_str());

                // Reset the entry in case it's still loading or was a memory allocation fail.
                UncacheFile(audioFileEntry);
            }

            // The file was either properly uncached, queued for uncache or not cached at all.
            success = true;
        }

        return success;
    }

#if !defined(AUDIO_RELEASE)
    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::DrawDebugInfo(AzFramework::DebugDisplayRequests& debugDisplay, const float posX, const float posY)
    {
        if (CVars::s_debugDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(DebugDraw::Options::FileCacheInfo)))
        {
            const auto frameTime = AZStd::chrono::steady_clock::now();

            const float entryDrawSize = 0.8f;
            const float entryStepSize = 15.0f;
            float positionY = posY + 20.0f;
            float positionX = posX + 20.0f;
            float time = 0.0f;
            float ratio = 0.0f;
            float originalAlpha = 0.7f;

            // The colors.
            AZ::Color white{ 1.0f, 1.0f, 1.0f, originalAlpha };     // file is use-counted
            AZ::Color cyan{ 0.0f, 1.0f, 1.0f, originalAlpha };      // file is global scope
            AZ::Color orange{ 1.0f, 0.5f, 0.0f, originalAlpha };    // header color
            AZ::Color green{ 0.0f, 1.0f, 0.0f, originalAlpha };     // file is removable
            AZ::Color red{ 1.0f, 0.0f, 0.0f, originalAlpha };       // memory allocation failed
            AZ::Color redish{ 0.7f, 0.0f, 0.0f, originalAlpha };    // file not found
            AZ::Color blue{ 0.1f, 0.2f, 0.8f, originalAlpha };      // file is loading
            AZ::Color yellow{ 1.0f, 1.0f, 0.0f, originalAlpha };    // file is level scope
            AZ::Color darkish{ 0.3f, 0.3f, 0.3f, originalAlpha };   // file is not loaded

            const bool displayAll = CVars::s_fcmDrawOptions.GetRawFlags() == 0;
            const bool displayGlobals = CVars::s_fcmDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(FileCacheManagerDebugDraw::Options::Global));
            const bool displayLevels = CVars::s_fcmDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(FileCacheManagerDebugDraw::Options::LevelSpecific));
            const bool displayUseCounted =
                CVars::s_fcmDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(FileCacheManagerDebugDraw::Options::UseCounted));
            const bool displayLoaded = CVars::s_fcmDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(FileCacheManagerDebugDraw::Options::Loaded));

            // The text
            AZStd::string str = AZStd::string::format(
                "File Cache Mgr (%zu of %zu KiB) [Total Entries: %zu]", m_currentByteTotal >> 10, m_maxByteTotal >> 10,
                m_audioFileEntries.size());
            debugDisplay.SetColor(orange);
            debugDisplay.Draw2dTextLabel(posX, positionY, entryDrawSize, str.c_str());
            positionY += entryStepSize;

            for (auto& audioFileEntryPair : m_audioFileEntries)
            {
                AZ::Color& color = white;

                CATLAudioFileEntry* const audioFileEntry = audioFileEntryPair.second;

                bool isGlobal = (audioFileEntry->m_dataScope == eADS_GLOBAL);
                bool isLevel = (audioFileEntry->m_dataScope == eADS_LEVEL_SPECIFIC);
                bool isUseCounted = audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_USE_COUNTED);
                bool isLoaded = audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_CACHED);

                if (displayAll || (displayGlobals && isGlobal) || (displayLevels && isLevel) || (displayUseCounted && isUseCounted) || (displayLoaded && isLoaded))
                {
                    if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_LOADING))
                    {
                        color = blue;
                    }
                    else if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_MEMALLOCFAIL))
                    {
                        color = red;
                    }
                    else if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_REMOVABLE))
                    {
                        color = green;
                    }
                    else if (!isLoaded)
                    {
                        color = darkish;
                    }
                    else if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_NOTFOUND))
                    {
                        color = redish;
                    }
                    else if (isGlobal)
                    {
                        color = cyan;
                    }
                    else if (isLevel)
                    {
                        color = yellow;
                    }
                    //else  isUseCounted

                    using duration_sec = AZStd::chrono::duration<float>;
                    time = AZStd::chrono::duration_cast<duration_sec>(frameTime - audioFileEntry->m_timeCached).count();

                    ratio = time / 5.0f;
                    originalAlpha = color.GetA();
                    color.SetA(originalAlpha * AZ::GetClamp(ratio, 0.2f, 1.0f));

                    bool kiloBytes = false;
                    size_t fileSize = audioFileEntry->m_fileSize;
                    if (fileSize >= 1024)
                    {
                        fileSize >>= 10;
                        kiloBytes = true;
                    }

                    // Format: "relative/path/filename.ext (230 KiB) [2]"
                    str = AZStd::string::format(
                        "%s (%zu %s) [%u]", audioFileEntry->m_filePath.c_str(), fileSize, kiloBytes ? "KiB" : "Bytes",
                        audioFileEntry->m_useCount);
                    debugDisplay.SetColor(color);
                    debugDisplay.Draw2dTextLabel(positionX, positionY, entryDrawSize, str.c_str());

                    color.SetA(originalAlpha);
                    positionY += entryStepSize;
                }
            }
        }
    }
#endif // !AUDIO_RELEASE

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::DoesRequestFitInternal(const size_t requestSize)
    {
        // Make sure these unsigned values don't flip around.
        AZ_Assert(m_currentByteTotal <= m_maxByteTotal, "FileCacheManager DoesRequestFitInternal - Unsigned wraparound detected!");
        bool success = false;

        if (requestSize <= (m_maxByteTotal - m_currentByteTotal))
        {
            // Here the requested size is available without the need of first cleaning up.
            success = true;
        }
        else
        {
            // Determine how much memory would get freed if all eAFF_REMOVABLE files get thrown out.
            // We however skip files that are already queued for unload. The request will get queued up in that case.
            size_t possibleMemoryGain = 0;

            // Check the single file entries for removability.
            for (auto& audioFileEntryPair : m_audioFileEntries)
            {
                CATLAudioFileEntry* const audioFileEntry = audioFileEntryPair.second;

                if (audioFileEntry && audioFileEntry->m_flags.AreAllFlagsActive(eAFF_CACHED | eAFF_REMOVABLE))
                {
                    possibleMemoryGain += audioFileEntry->m_fileSize;
                }
            }

            const size_t maxAvailableSize = (m_maxByteTotal - (m_currentByteTotal - possibleMemoryGain));

            if (requestSize <= maxAvailableSize)
            {
                // Here we need to cleanup first before allowing the new request to be allocated.
                TryToUncacheFiles();

                // We should only indicate success if there's actually really enough room for the new entry!
                success = (m_maxByteTotal - m_currentByteTotal) >= requestSize;
            }
        }

        return success;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::FinishAsyncStreamRequest(AZ::IO::FileRequestHandle request)
    {
        auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
        AZ_Assert(streamer, "FileCacheManager - AZ::IO::Streamer is not available.")

        // Find the file entry that matches the request handle...
        auto fileEntryIter = AZStd::find_if(m_audioFileEntries.begin(), m_audioFileEntries.end(),
            [&request] (const AZStd::pair<TAudioFileEntryID, CATLAudioFileEntry*>& data) -> bool
            {
                return (data.second->m_asyncStreamRequest == request);
            }
        );

        // If found, we finish processing the async file load request...
        if (fileEntryIter != m_audioFileEntries.end())
        {
            void* buffer{};
            AZ::u64 numBytesRead{};
            [[maybe_unused]] bool result = streamer->GetReadRequestResult(request, buffer, numBytesRead);
            AZ_Assert(result, "FileCacheManager - Unable to retrieve read information from the file request. "
                "This can happen if the callback was assigned to a request that didn't read.");

            CATLAudioFileEntry* audioFileEntry = fileEntryIter->second;
            AZ_Assert(audioFileEntry, "FileCacheManager - Audio file entry is null!");
            AZ_Assert(buffer == audioFileEntry->m_memoryBlock, "FileCacheManager - The memory buffer doesn't match the file entry memory block!");
            FinishCachingFileInternal(audioFileEntry, numBytesRead, streamer->GetRequestStatus(request));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::FinishCachingFileInternal(CATLAudioFileEntry* const audioFileEntry, [[maybe_unused]] AZ::IO::SizeType bytesRead,
        AZ::IO::IStreamerTypes::RequestStatus requestState)
    {
        AZ_PROFILE_FUNCTION(Audio);

        bool success = false;
        audioFileEntry->m_asyncStreamRequest.reset();

        switch (requestState)
        {
            case AZ::IO::IStreamerTypes::RequestStatus::Completed:
            {
                AZ_Assert(
                    bytesRead == audioFileEntry->m_fileSize,
                    "FileCacheManager - Sync Streamed Read completed, but bytes read does not match file size!");

                if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_LOADING))
                {
                    audioFileEntry->m_flags.AddFlags(eAFF_CACHED);
                    audioFileEntry->m_flags.ClearFlags(eAFF_LOADING);

#if !defined(AUDIO_RELEASE)
                    audioFileEntry->m_timeCached = AZStd::chrono::steady_clock::now();
#endif // !AUDIO_RELEASE

                    SATLAudioFileEntryInfo fileEntryInfo;
                    fileEntryInfo.nMemoryBlockAlignment = audioFileEntry->m_memoryBlockAlignment;
                    fileEntryInfo.pFileData = audioFileEntry->m_memoryBlock;
                    fileEntryInfo.nSize = audioFileEntry->m_fileSize;
                    fileEntryInfo.pImplData = audioFileEntry->m_implData;
                    AZ::IO::PathView filePath{ audioFileEntry->m_filePath };
                    fileEntryInfo.sFileName = filePath.Filename().Native().data();

                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::RegisterInMemoryFile, &fileEntryInfo);
                    success = true;

                    AZLOG_DEBUG("FileCacheManager - File Cached: '%s'", fileEntryInfo.sFileName);
                }
                break;
            }
            case AZ::IO::IStreamerTypes::RequestStatus::Failed:
            {
                AZLOG_ERROR("FileCacheManager - Async file stream '%s' failed during operation!", audioFileEntry->m_filePath.c_str());
                UncacheFileCacheEntryInternal(audioFileEntry, true, true);
                break;
            }
            case AZ::IO::IStreamerTypes::RequestStatus::Canceled:
            {
                AZLOG_DEBUG("FileCacheManager - Async file stream '%s' was canceled by user!", audioFileEntry->m_filePath.c_str());
                UncacheFileCacheEntryInternal(audioFileEntry, true, true);
                break;
            }
            default:
            {
                break;
            }
        }

        return success;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::UpdatePreloadRequestsStatus()
    {
        // Run through the list of preload requests and their fileEntryIDs.
        // Check the fileEntries for the CACHED flags and accumulate the 'allLoaded' and 'anyLoaded' status of each preload request.
        // If the result is different than what is stored on the preload request, update it and send a notification of
        // either cached or uncached.
        for (auto& preloadPair : m_preloadRequests)
        {
            CATLPreloadRequest* preloadRequest = preloadPair.second;
            bool wasLoaded = preloadRequest->m_allLoaded;
            bool allLoaded = !preloadRequest->m_cFileEntryIDs.empty();
            bool anyLoaded = false;
            for (auto fileId : preloadRequest->m_cFileEntryIDs)
            {
                bool cached = false;
                auto iter = m_audioFileEntries.find(fileId);
                if (iter != m_audioFileEntries.end())
                {
                    cached = iter->second->m_flags.AreAnyFlagsActive(eAFF_CACHED);
                }
                allLoaded = (allLoaded && cached);
                anyLoaded = (anyLoaded || cached);
            }

            if (allLoaded != wasLoaded && allLoaded)
            {
                // Loaded now...
                preloadRequest->m_allLoaded = allLoaded;
                AudioPreloadNotificationBus::Event(preloadPair.first, &AudioPreloadNotificationBus::Events::OnAudioPreloadCached);
            }

            if (anyLoaded != wasLoaded && !anyLoaded)
            {
                // Unloaded now...
                preloadRequest->m_allLoaded = anyLoaded;
                AudioPreloadNotificationBus::Event(preloadPair.first, &AudioPreloadNotificationBus::Events::OnAudioPreloadUncached);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::AllocateMemoryBlockInternal(CATLAudioFileEntry* const audioFileEntry)
    {
        AZ_PROFILE_FUNCTION(Audio);

        // Must not have valid memory yet.
        AZ_Assert(!audioFileEntry->m_memoryBlock, "FileCacheManager AllocateMemoryBlockInternal - Memory appears to be set already!");

        audioFileEntry->m_memoryBlock = AZ::AllocatorInstance<AudioBankAllocator>::Get().Allocate(
            audioFileEntry->m_fileSize,
            audioFileEntry->m_memoryBlockAlignment,
            0,
            audioFileEntry->m_filePath.c_str(),
            __FILE__, __LINE__);

        if (!audioFileEntry->m_memoryBlock)
        {
            // Memory block is either full or too fragmented, let's try to throw everything out that can be removed and allocate again.
            TryToUncacheFiles();

            // And try again
            audioFileEntry->m_memoryBlock = AZ::AllocatorInstance<AudioBankAllocator>::Get().Allocate(
                audioFileEntry->m_fileSize,
                audioFileEntry->m_memoryBlockAlignment,
                0,
                audioFileEntry->m_filePath.c_str(),
                __FILE__, __LINE__);
        }

        return (audioFileEntry->m_memoryBlock != nullptr);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::UncacheFile(CATLAudioFileEntry* const audioFileEntry)
    {
        if (audioFileEntry->m_asyncStreamRequest)
        {
            auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
            AZ::IO::FileRequestPtr request = streamer->Cancel(audioFileEntry->m_asyncStreamRequest);

            AZStd::binary_semaphore wait;
            streamer->SetRequestCompleteCallback(
                request,
                [&wait](AZ::IO::FileRequestHandle)
                {
                    wait.release();
                });

            streamer->QueueRequest(request);
            wait.acquire();

            audioFileEntry->m_asyncStreamRequest.reset();
        }

        if (audioFileEntry->m_memoryBlock)
        {
            SATLAudioFileEntryInfo fileEntryInfo;
            fileEntryInfo.nMemoryBlockAlignment = audioFileEntry->m_memoryBlockAlignment;
            fileEntryInfo.pFileData = audioFileEntry->m_memoryBlock;
            fileEntryInfo.nSize = audioFileEntry->m_fileSize;
            fileEntryInfo.pImplData = audioFileEntry->m_implData;
            AZ::IO::PathView filePath{ audioFileEntry->m_filePath };
            fileEntryInfo.sFileName = filePath.Filename().Native().data();

            EAudioRequestStatus result = EAudioRequestStatus::None;
            AudioSystemImplementationRequestBus::BroadcastResult(result, &AudioSystemImplementationRequestBus::Events::UnregisterInMemoryFile, &fileEntryInfo);
            if (result == EAudioRequestStatus::Success)
            {
                AZLOG_DEBUG("FileCacheManager - File Uncached: '%s'", fileEntryInfo.sFileName);
            }
            else
            {
                AZLOG_NOTICE("FileCacheManager - Unable to uncache file '%s'", fileEntryInfo.sFileName);
                return;
            }
        }

        AZ::AllocatorInstance<AudioBankAllocator>::Get().DeAllocate(
            audioFileEntry->m_memoryBlock,
            audioFileEntry->m_fileSize,
            audioFileEntry->m_memoryBlockAlignment
        );
        audioFileEntry->m_flags.ClearFlags(eAFF_CACHED | eAFF_REMOVABLE);
        m_currentByteTotal -= audioFileEntry->m_fileSize;
        AZ_Warning("FileCacheManager", audioFileEntry->m_useCount == 0, "Use-count of file '%s' is non-zero while uncaching it! Use Count: %d", audioFileEntry->m_filePath.c_str(), audioFileEntry->m_useCount);
        audioFileEntry->m_useCount = 0;

    #if !defined(AUDIO_RELEASE)
        audioFileEntry->m_timeCached = AZStd::chrono::steady_clock::time_point();
    #endif // !AUDIO_RELEASE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::TryToUncacheFiles()
    {
        for (auto& audioFileEntryPair : m_audioFileEntries)
        {
            CATLAudioFileEntry* const audioFileEntry = audioFileEntryPair.second;

            if (audioFileEntry && audioFileEntry->m_flags.AreAllFlagsActive(eAFF_CACHED | eAFF_REMOVABLE))
            {
                UncacheFileCacheEntryInternal(audioFileEntry, true);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::UpdateLocalizedFileEntryData(CATLAudioFileEntry* const audioFileEntry)
    {
        static SATLAudioFileEntryInfo fileEntryInfo;
        fileEntryInfo.bLocalized = true;
        fileEntryInfo.nSize = 0;
        fileEntryInfo.pFileData = nullptr;
        fileEntryInfo.nMemoryBlockAlignment = 0;

        AZ::IO::FixedMaxPath filePath{ audioFileEntry->m_filePath };
        AZStd::string_view fileName{ filePath.Filename().Native() };
        fileEntryInfo.pImplData = audioFileEntry->m_implData;
        fileEntryInfo.sFileName = fileName.data();

        const char* fileLocation = nullptr;
        AudioSystemImplementationRequestBus::BroadcastResult(fileLocation, &AudioSystemImplementationRequestBus::Events::GetAudioFileLocation, &fileEntryInfo);
        if (fileLocation && fileLocation[0] != '\0')
        {
            audioFileEntry->m_filePath.assign(fileLocation);
            audioFileEntry->m_filePath.append(fileName.data(), fileName.size());
        }
        else
        {
            AZ_WarningOnce("FileCacheManager", fileLocation != nullptr, "GetAudioFileLocation returned null when getting a localized file path!  Path will not be changed.");
        }
        AZStd::to_lower(audioFileEntry->m_filePath.begin(), audioFileEntry->m_filePath.end());

        AZ::u64 fileSize = 0;
        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        fileIO->Size(audioFileEntry->m_filePath.c_str(), fileSize);
        audioFileEntry->m_fileSize = fileSize;

        AZ_Assert(audioFileEntry->m_fileSize != 0, "FileCacheManager - UpdateLocalizedFileEntryData expected file size to be greater than zero!");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::TryCacheFileCacheEntryInternal(
        CATLAudioFileEntry* const audioFileEntry,
        [[maybe_unused]] const TAudioFileEntryID fileEntryId,
        [[maybe_unused]] const bool loadSynchronously,
        const bool overrideUseCount /* = false */,
        const AZ::u32 useCount /* = 0 */)
    {
        AZ_PROFILE_FUNCTION(Audio);

        bool success = false;

        if (!audioFileEntry->m_filePath.empty() && !audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_CACHED | eAFF_LOADING))
        {
            if (DoesRequestFitInternal(audioFileEntry->m_fileSize) && AllocateMemoryBlockInternal(audioFileEntry))
            {
                auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
                AZ_Assert(streamer, "FileCacheManager - Streamer should be ready!");

                audioFileEntry->m_flags.AddFlags(eAFF_LOADING);

                if (loadSynchronously)
                {
                    AZ::IO::FileRequestPtr request = streamer->Read(
                        audioFileEntry->m_filePath.c_str(),
                        audioFileEntry->m_memoryBlock,
                        audioFileEntry->m_fileSize,
                        audioFileEntry->m_fileSize,
                        AZ::IO::IStreamerTypes::s_deadlineNow,
                        AZ::IO::IStreamerTypes::s_priorityHigh);

                    AZStd::binary_semaphore wait;
                    streamer->SetRequestCompleteCallback(
                        request,
                        [&wait](AZ::IO::FileRequestHandle)
                        {
                            wait.release();
                        });

                    streamer->QueueRequest(request);
                    wait.acquire();

                    AZ::IO::IStreamerTypes::RequestStatus status = streamer->GetRequestStatus(request);
                    if (FinishCachingFileInternal(audioFileEntry, audioFileEntry->m_fileSize, status))
                    {
                        m_currentByteTotal += audioFileEntry->m_fileSize;
                        success = true;
                    }
                }
                else
                {
                    if (!audioFileEntry->m_asyncStreamRequest)
                    {
                        audioFileEntry->m_asyncStreamRequest = streamer->CreateRequest();
                    }

                    streamer->Read(
                        audioFileEntry->m_asyncStreamRequest,
                        audioFileEntry->m_filePath.c_str(),
                        audioFileEntry->m_memoryBlock,
                        audioFileEntry->m_fileSize,
                        audioFileEntry->m_fileSize,
                        AZ::IO::IStreamerTypes::s_noDeadline,
                        AZ::IO::IStreamerTypes::s_priorityHigh);

                    streamer->SetRequestCompleteCallback(
                        audioFileEntry->m_asyncStreamRequest,
                        [](AZ::IO::FileRequestHandle request)
                        {
                            AZ_PROFILE_FUNCTION(Audio);
                            AudioFileCacheManagerNotficationBus::QueueBroadcast(
                                &AudioFileCacheManagerNotficationBus::Events::FinishAsyncStreamRequest,
                                request);
                        });

                    streamer->QueueRequest(audioFileEntry->m_asyncStreamRequest);

                    // Increase total size even though async request is processing...
                    m_currentByteTotal += audioFileEntry->m_fileSize;
                    success = true;
                }
            }
            else
            {
                // Cannot have a valid memory block!
                AZ_Assert(audioFileEntry->m_memoryBlock == nullptr,
                    "FileCacheManager - Memory block should be null after memory allocation failure!");

                // This unfortunately is a total memory allocation fail.
                audioFileEntry->m_flags.AddFlags(eAFF_MEMALLOCFAIL);

                // The user should be made aware of it.
                AZLOG_ERROR(
                    "FileCacheManager - Could not cache '%s' - out of memory or fragmented memory!", audioFileEntry->m_filePath.c_str());
            }
        }
        else if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_CACHED | eAFF_LOADING))
        {
            AZLOG_DEBUG(
                "FileCacheManager - Skipping '%s' - it's either already loaded or currently loading!", audioFileEntry->m_filePath.c_str());
            success = true;
        }
        else if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_NOTFOUND))
        {
            AZLOG_WARN(
                "FileCacheManager - Could not cache '%s' - file was not found at that location!", audioFileEntry->m_filePath.c_str());
        }

        // Increment the used count on manually-loaded files.
        if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_USE_COUNTED) && audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_CACHED | eAFF_LOADING))
        {
            if (overrideUseCount)
            {
                audioFileEntry->m_useCount = useCount;
            }
            else
            {
                ++audioFileEntry->m_useCount;
            }

            // Make sure to handle the eAFCS_REMOVABLE flag according to the m_useCount count.
            if (audioFileEntry->m_useCount != 0)
            {
                audioFileEntry->m_flags.ClearFlags(eAFF_REMOVABLE);
            }
            else
            {
                audioFileEntry->m_flags.AddFlags(eAFF_REMOVABLE);
            }
        }

        return success;
    }

} // namespace Audio
