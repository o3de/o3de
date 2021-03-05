/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
