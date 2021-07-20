/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AudioInput/AudioInputStream.h>
#include <Common_wwise.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Audio Streaming Input
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AudioInputStreaming::AudioInputStreaming(const SAudioInputConfig& sourceConfig)
        : m_framesReady(0)
    {
        m_config = sourceConfig;

        size_t bytesPerSample = (m_config.m_bitsPerSample >> 3);
        size_t numSamples = m_config.m_sampleRate * m_config.m_numChannels;      // <-- This gives a 1 second buffer based on the configuration.

        m_config.m_bufferSize = static_cast<AZ::u32>(numSamples * bytesPerSample);

        if (m_config.m_sampleType == AudioInputSampleType::Float && m_config.m_bitsPerSample == 32)
        {
            m_buffer.reset(new RingBuffer<float>(numSamples));
        }
        else if (m_config.m_sampleType == AudioInputSampleType::Int && m_config.m_bitsPerSample == 16)
        {
            m_buffer.reset(new RingBuffer<AZ::s16>(numSamples));
        }
        else
        {
            AZ_Error("AudioInputStreaming", false, "Audio Stream Format Unsupported!  Bits Per Sample = %d, Sample Type = %d",
                m_config.m_bitsPerSample, static_cast<int>(m_config.m_sampleType));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AudioInputStreaming::~AudioInputStreaming()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    size_t AudioInputStreaming::ReadStreamingInput(const AudioStreamData& data)
    {
        size_t numFrames = data.m_sizeBytes / (m_config.m_bitsPerSample >> 3) / m_config.m_numChannels;
        size_t framesAdded = m_buffer->AddData(data.m_data, numFrames, m_config.m_numChannels);
        m_framesReady += framesAdded;
        return framesAdded;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    size_t AudioInputStreaming::ReadStreamingMultiTrackInput(AudioStreamMultiTrackData& data)
    {
        size_t numFrames = data.m_sizeBytes / (m_config.m_bitsPerSample >> 3);
        size_t framesAdded = m_buffer->AddMultiTrackDataInterleaved(data.m_data, numFrames, m_config.m_numChannels);
        m_framesReady += framesAdded;
        return framesAdded;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioInputStreaming::FlushStreamingInput()
    {
        m_buffer->ResetBuffer();
        m_framesReady = 0;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    size_t AudioInputStreaming::GetStreamingInputNumFramesReady() const
    {
        return m_framesReady;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioInputStreaming::ReadInput([[maybe_unused]] const AudioStreamData& data)
    {
        // Intentionally left as an empty implementation.
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioInputStreaming::WriteOutput(AkAudioBuffer* akBuffer)
    {
        AZ::u16 numSampleFramesRequested = (akBuffer->MaxFrames() - akBuffer->uValidFrames);

        AkSampleType* channelData[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

        for (AZ::u32 channel = 0; channel < akBuffer->NumChannels(); ++channel)
        {
            channelData[channel] = akBuffer->GetChannel(channel);
        }

        bool deinterleave = (m_config.m_sampleType == AudioInputSampleType::Float);
        size_t numSampleFramesCopied = m_buffer->ConsumeData(reinterpret_cast<void**>(channelData), numSampleFramesRequested, akBuffer->NumChannels(), deinterleave);
        akBuffer->uValidFrames += aznumeric_cast<AkUInt16>(numSampleFramesCopied);
        m_framesReady -= numSampleFramesCopied;

        akBuffer->eState = (numSampleFramesCopied > 0) ? AK_DataReady : AK_NoDataReady;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool AudioInputStreaming::IsOk() const
    {
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioInputStreaming::OnActivated()
    {
        AZ_Assert(m_config.m_sourceId != INVALID_AUDIO_SOURCE_ID, "AudioInputStreaming - Being activated but no valid Source Id!\n");
        AudioStreamingRequestBus::Handler::BusConnect(m_config.m_sourceId);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioInputStreaming::OnDeactivated()
    {
        AudioStreamingRequestBus::Handler::BusDisconnect();
    }

} // namespace Audio
