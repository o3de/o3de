/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AudioInput/AudioInputMicrophone.h>
#include <AzCore/Casting/numeric_cast.h>
#include <MicrophoneBus.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Audio Input Source : Microphone
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AudioInputMicrophone::AudioInputMicrophone(const SAudioInputConfig& sourceConfig)
    {
        m_config = sourceConfig;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AudioInputMicrophone::~AudioInputMicrophone()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioInputMicrophone::ReadInput([[maybe_unused]] const AudioStreamData& data)
    {
        // ReadInput only used when PUSHing source data in, and would need an internal buffer to store
        // the data temporarily.  For Microphone, the microphone impl has its own internal buffer, so
        // we only need to PULL data in WriteOutput.
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioInputMicrophone::WriteOutput(AkAudioBuffer* akBuffer)
    {
        AZ::u32 numSampleFramesRequested = (akBuffer->MaxFrames() - akBuffer->uValidFrames);

        AkSampleType* channelData[2] = { nullptr, nullptr };

        for (AZ::u32 channel = 0; channel < akBuffer->NumChannels(); ++channel)
        {
            channelData[channel] = akBuffer->GetChannel(channel);
        }

        size_t numSampleFramesCopied = 0;
        MicrophoneRequestBus::BroadcastResult(numSampleFramesCopied, &MicrophoneRequestBus::Events::GetData, reinterpret_cast<void**>(channelData), numSampleFramesRequested, m_config, true);

        akBuffer->uValidFrames += aznumeric_cast<AkUInt16>(numSampleFramesCopied);

        akBuffer->eState = (numSampleFramesCopied > 0) ? AK_DataReady : AK_NoDataReady;
        // handle the AK_NoMoreData condition?
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioInputMicrophone::OnDeactivated()
    {
        m_config.m_numChannels = 0;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool AudioInputMicrophone::IsOk() const
    {
        // Mono and Stereo only
        bool ok = (m_config.m_numChannels == 1 || m_config.m_numChannels == 2);

        // 32-bit float or 16-bit int only
        ok &= (m_config.m_sampleType == AudioInputSampleType::Float && m_config.m_bitsPerSample == 32)
            || (m_config.m_sampleType == AudioInputSampleType::Int && m_config.m_bitsPerSample == 16);
        return ok;
    }

} // namespace Audio
