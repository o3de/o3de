/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <IAudioInterfacesCommonData.h>


namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! AudioSystemImplementation may use this base class for a middleware-specific audio object.
    //! (e.g. a middleware-specific object ID)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLAudioObjectData
    {
        virtual ~IATLAudioObjectData() = default;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! AudioSystemImplementation may use this base class for an middleware-specific audio listener.
    //! (e.g. a middleware-specific object ID)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLListenerData
    {
        virtual ~IATLListenerData() = default;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! AudioSystemImplementation may use this base class for a middleware-specific audio trigger.
    //! (e.g. a middleware-specific event ID or name, a sound file to be passed to an API function)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLTriggerImplData
    {
        virtual ~IATLTriggerImplData() = default;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! AudioSystemImplementation may use this base class for a middleware-specific audio parameter.
    //! (e.g. a middleware-specific parameter ID or name to be passed to an API function)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLRtpcImplData
    {
        virtual ~IATLRtpcImplData() = default;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! AudioSystemImplementation may use this base class for a middleware-specific audio switch state.
    //! (e.g. a middleware-specific switch ID or switch/state names to be passed to an API function)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLSwitchStateImplData
    {
        virtual ~IATLSwitchStateImplData() = default;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! AudioSystemImplementation may use this base class for a middleware-specific audio environment.
    //! (e.g. a middleware-specific auxiliary bus ID or name to be passed to an API function)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLEnvironmentImplData
    {
        virtual ~IATLEnvironmentImplData() = default;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! AudioSystemImplementation may use this base class for a middleware-specific audio event.
    //! (e.g. a middleware-specific event or playing ID of an active event/sound)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLEventData
    {
        virtual ~IATLEventData() = default;

        TAudioControlID m_triggerId{ INVALID_AUDIO_CONTROL_ID };
        void* m_owner{ nullptr };
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! AudioSystemImplementation may use this base class for a middleware-specific audio file entry.
    //! (e.g. a middleware-specific bank ID if the AudioFileEntry represents a soundbank)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IATLAudioFileEntryData
    {
        virtual ~IATLAudioFileEntryData() = default;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! AudioSystemImplementation may use this base class for a middleware-specific audio source.
    //! (e.g. a middleware-specific source ID, language, collection, and file ID of an external source)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLSourceData
    {
        SAudioSourceInfo m_sourceInfo;

        SATLSourceData() = default;

        SATLSourceData(const SAudioSourceInfo& sourceInfo)
            : m_sourceInfo(sourceInfo)
        {}

        ~SATLSourceData() = default;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! This is used to pass information about a file loaded into memory between the Audio System
    //! and the Audio Engine (i.e. audio middleware implementation)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLAudioFileEntryInfo
    {
        IATLAudioFileEntryData* pImplData = nullptr;    // the implementation-specific data needed for this AudioFileEntry
        const char* sFileName = nullptr;                // file name
        void* pFileData = nullptr;                      // memory location of the file's contents
        size_t nSize = 0;                               // file size
        size_t nMemoryBlockAlignment = 0;               // alignment to be used when allocating memory for this file's contents
        bool bLocalized = false;                        // is the file localized?
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! This is used to pass information about an AudioSystemImplementation's memory usage in its main allocators.
    //! Note: This struct cannot define a constructor, it needs to be a POD!
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioImplMemoryInfo
    {
        size_t nPrimaryPoolSize = 0;             // total size in bytes of the Primary Memory Pool
        size_t nPrimaryPoolUsedSize = 0;         // bytes allocated inside the Primary Memory Pool
        size_t nPrimaryPoolAllocations = 0;      // number of allocations performed in the Primary Memory Pool
        size_t nSecondaryPoolSize = 0;           // total size in bytes of the Secondary Memory Pool
        size_t nSecondaryPoolUsedSize = 0;       // bytes allocated inside the Secondary Memory Pool
        size_t nSecondaryPoolAllocations = 0;    // number of allocations performed in the Secondary Memory Pool
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! This is used to pass information about an audio middleware's detailed memory pool usage.
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct AudioImplMemoryPoolInfo
    {
        char m_poolName[64];            // friendly name of the pool
        AZ::s32 m_poolId = -1;          // -1 is invalid/default
        AZ::u32 m_memoryReserved = 0;   // size of the pool in bytes
        AZ::u32 m_memoryUsed = 0;       // amount of the pool used in bytes
        AZ::u32 m_peakUsed = 0;         // peak used size in bytes
        AZ::u32 m_numAllocs = 0;        // number of alloc calls
        AZ::u32 m_numFrees = 0;         // number of free calls
    };

} // namespace Audio
