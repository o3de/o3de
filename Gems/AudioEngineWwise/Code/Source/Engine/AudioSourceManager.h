/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformIncl.h> // This include is needed to include WinSock2.h before including Windows.h
                                 // As AK/SoundEngine/Common/AkTypes.h eventually includes Windows.h
#include <IAudioInterfacesCommonData.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzCore/IO/FileIO.h>

#include <AudioAllocators.h>

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>

namespace Audio
{
    /**
     * Base class for Audio Input Source types.
     * Represents an Audio Input Source, which has input/output routines and configuration information.
     */
    class AudioInputSource
    {
    public:
        AUDIO_IMPL_CLASS_ALLOCATOR(AudioInputSource)

        AudioInputSource() = default;
        virtual ~AudioInputSource() = default;

        virtual void ReadInput(const AudioStreamData& data) = 0;
        virtual void WriteOutput(AkAudioBuffer* akBuffer) = 0;

        virtual bool IsOk() const = 0;
        virtual bool IsFormatValid() const;

        virtual void OnActivated() {}
        virtual void OnDeactivated() {}

        void SetFormat(AkAudioFormat& format);
        void SetSourceId(TAudioSourceId sourceId);
        TAudioSourceId GetSourceId() const;

    protected:
        SAudioInputConfig m_config;     ///< Configuration information for the source type.
        AkPlayingID m_playingId = AK_INVALID_PLAYING_ID;    ///< Playing ID of the source.
    };


    /**
     * Manager class for AudioInputSource.
     * Manages lifetime of AudioInputSource objects as they are created, activated, deactivated, and destroyed.
     * The lifetime of an Audio Input Source:
     * CreateSource (loads resources)
     *  ActivateSource (once you obtain a playing Id)
     *  (Running, callbacks being received, also async loading input if enabled)
     *  DeactivateSource (once it's determined to be done playing)
     * DestroySource (unloads resources)
     */
    class AudioSourceManager
    {
    public:
        AudioSourceManager();
        ~AudioSourceManager();

        static AudioSourceManager& Get();
        static void Initialize();
        void Shutdown();

        /**
         * CreateSource a new AudioInputSource.
         * Creates an AudioInputSource, based on the SAudioInputConfig and stores it in an inactive state.
         * @param sourceConfig Configuration of the AudioInputSource.
         * @return True if the source was created successfully, false otherwise.
         */
        bool CreateSource(const SAudioInputConfig& sourceConfig);

        /**
         * Activates an AudioInputSource.
         * Moves a source from the inactive state to an active state by assigning an AkPlayingID.
         * @param sourceId ID of the source (returned by CreateSource).
         * @param playingId A playing ID of the source that is now playing in Wwise.
         */
        void ActivateSource(TAudioSourceId sourceId, AkPlayingID playingId);

        /**
         * Deactivates an AudioInputSource.
         * Moves a source from the active state back to an inactive state, will happen when an end event callback is recieved.
         * @param playingId Playing ID of the source that ended.
         */
        void DeactivateSource(AkPlayingID playingId);

        /**
         * Destroy an AudioInputSource.
         * Destroys an AudioInputSource from the manager when it is no longer needed.
         * @param sourceId Source ID of the object to remove.
         */
        void DestroySource(TAudioSourceId sourceId);

        /**
         * Find the Playing ID of a source.
         * Given a Source ID, check if there are sources in the active state and if so, return their Playing ID.
         * @param sourceId Source ID to look for in the active sources.
         */
        AkPlayingID FindPlayingSource(TAudioSourceId sourceId);

    private:
        /**
         * Wwise Audio Input Plugin "Execute" callback function.
         * This will be called whenever a playing Audio Input Source needs to be fed.
         * @param playingId The Playing ID of the source.
         * @param audioBuffer The buffer to copy samples into.
         */
        static void ExecuteCallback(AkPlayingID playingId, AkAudioBuffer* audioBuffer);

        /**
         * Wwise Audio Input Plugin "GetFormat" callback function.
         * This will be called once whenever a new Audio Input Source is starting playback.
         * @param playingId The Playing ID of the source.
         * @param audioFormat The format structure that should be filled with format information.
         */
        static void GetFormatCallback(AkPlayingID playingId, AkAudioFormat& audioFormat);

        AZStd::mutex m_inputMutex;      ///< Callbacks will come from the Wwise event processing thread.

        template <typename KeyType, typename ValueType>
        using AudioInputMap = AZStd::unordered_map<KeyType, AZStd::unique_ptr<ValueType>, AZStd::hash<KeyType>, AZStd::equal_to<KeyType>, Audio::AudioImplStdAllocator>;

        AudioInputMap<TAudioSourceId, AudioInputSource> m_inactiveAudioInputs;      ///< Sources that haven't started playing yet.
        AudioInputMap<AkPlayingID, AudioInputSource> m_activeAudioInputs;           ///< Sources that are currently playing.
    };
}
