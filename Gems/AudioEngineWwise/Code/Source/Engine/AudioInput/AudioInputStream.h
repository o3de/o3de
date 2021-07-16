/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "AudioSourceManager.h"
#include <IAudioSystem.h>
#include <AudioRingBuffer.h>

namespace Audio
{
    /**
     * A type of AudioInputSource representing an audio stream.
     * holds a buffer of the raw data and provides methods to read chunks of data at a time
     * to an output (AkAudioBuffer).
     */
    class AudioInputStreaming
        : public AudioInputSource
        , public AudioStreamingRequestBus::Handler
    {
    public:
        AUDIO_IMPL_CLASS_ALLOCATOR(AudioInputStreaming)

        AudioInputStreaming(const SAudioInputConfig& sourceConfig);
        ~AudioInputStreaming() override;

        // AudioInputSource Interface
        void ReadInput(const AudioStreamData& data) override;
        void WriteOutput(AkAudioBuffer* akBuffer) override;
        bool IsOk() const override;

        void OnDeactivated() override;
        void OnActivated() override;

        // AudioStreamingRequestBus::Handler Interface
        size_t ReadStreamingInput(const AudioStreamData& data) override;
        size_t ReadStreamingMultiTrackInput(AudioStreamMultiTrackData& data) override;

        void FlushStreamingInput();
        size_t GetStreamingInputNumFramesReady() const;

    private:
        AZStd::unique_ptr<RingBufferBase> m_buffer = nullptr;
        size_t m_framesReady;
    };

} // namespace Audio
