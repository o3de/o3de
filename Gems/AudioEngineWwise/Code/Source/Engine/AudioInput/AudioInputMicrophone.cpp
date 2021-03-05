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
