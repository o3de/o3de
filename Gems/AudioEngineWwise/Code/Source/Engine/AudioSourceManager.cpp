/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AudioSourceManager.h>
#include <AudioInput/AudioInputFile.h>
#include <AudioInput/AudioInputMicrophone.h>
#include <AudioInput/AudioInputStream.h>

#include <AzCore/std/parallel/lock.h>

#include <AK/AkWwiseSDKVersion.h>
#include <AK/Plugin/AkAudioInputPlugin.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Audio Input Source
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool AudioInputSource::IsFormatValid() const
    {
        // Audio Input Source has restrictions on the formats that are supported:
        // 16-bit Integer samples, interleaved samples
        // 32-bit Float samples, non-interleaved samples
        // The Parser doesn't care about such restrictions and is only responsible for
        // reading the header information and validating it.

        bool valid = true;

        if (m_config.m_sampleType == AudioInputSampleType::Int && m_config.m_bitsPerSample != 16)
        {
            valid = false;
        }

        if (m_config.m_sampleType == AudioInputSampleType::Float && m_config.m_bitsPerSample != 32)
        {
            valid = false;
        }

        if (m_config.m_sampleType == AudioInputSampleType::Unsupported)
        {
            valid = false;
        }

        if (!valid)
        {
            AZ_TracePrintf("AudioInputFile", "The file format is NOT supported!  Only 16-bit integer or 32-bit float sample types are allowed!\n"
                "Current Format: (%s / %d)\n", m_config.m_sampleType == AudioInputSampleType::Int ? "Int"
                : (m_config.m_sampleType == AudioInputSampleType::Float ? "Float" : "Unknown"),
                m_config.m_bitsPerSample);
        }

        return valid;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioInputSource::SetFormat(AkAudioFormat& format)
    {
        AkUInt32 speakerConfig = 0;
        switch (m_config.m_numChannels)
        {
            case 1:
            {
                speakerConfig = AK_SPEAKER_SETUP_MONO;
                break;
            }
            case 2:
            {
                speakerConfig = AK_SPEAKER_SETUP_STEREO;
                break;
            }
            case 6:
            {
                speakerConfig = AK_SPEAKER_SETUP_5POINT1;
                break;
            }
            default:
            {
                // TODO: Test more channels
                return;
            }
        }

        AkUInt32 sampleType = 0;
        AkUInt32 sampleInterleaveType = 0;
        switch (m_config.m_bitsPerSample)
        {
            case 16:
            {
                sampleType = AK_INT;
                sampleInterleaveType = AK_INTERLEAVED;
                break;
            }
            case 32:
            {
                sampleType = AK_FLOAT;
                sampleInterleaveType = AK_NONINTERLEAVED;
                break;
            }
            default:
            {
                // Anything else and Audio Input Source doesn't support it.
                // But we've already checked the format when parsing the header, so we shouldn't get here.
                break;
            }
        }

        AkChannelConfig akChannelConfig(m_config.m_numChannels, speakerConfig);

        format.SetAll(
            m_config.m_sampleRate,
            akChannelConfig,
            m_config.m_bitsPerSample,
            m_config.m_numChannels * m_config.m_bitsPerSample >> 3, // shift converts bits->bytes, this is the frame size
            sampleType,
            sampleInterleaveType
        );
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioInputSource::SetSourceId(TAudioSourceId sourceId)
    {
        m_config.m_sourceId = sourceId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    TAudioSourceId AudioInputSource::GetSourceId() const
    {
        return m_config.m_sourceId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Audio Input Source Manager
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AudioSourceManager::AudioSourceManager()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AudioSourceManager::~AudioSourceManager()
    {
        Shutdown();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // static
    AudioSourceManager& AudioSourceManager::Get()
    {
        static AudioSourceManager s_manager;
        return s_manager;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // static
    void AudioSourceManager::Initialize()
    {
        // Wwise Api call to setup the callbacks used by Audio Input Sources.
        SetAudioInputCallbacks(AudioSourceManager::ExecuteCallback, AudioSourceManager::GetFormatCallback);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioSourceManager::Shutdown()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_inputMutex);

        m_activeAudioInputs.clear();
        m_inactiveAudioInputs.clear();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool AudioSourceManager::CreateSource(const SAudioInputConfig& sourceConfig)
    {
        AZStd::unique_ptr<AudioInputSource> ptr = nullptr;
        switch (sourceConfig.m_sourceType)
        {
            case AudioInputSourceType::PcmFile:
            case AudioInputSourceType::WavFile:
            //case AudioInputSourceType::OggFile:
            //case AudioInputSourceType::OpusFile:
            {
                if (!sourceConfig.m_sourceFilename.empty())
                {
                    ptr.reset(aznew AudioInputFile(sourceConfig));
                }
                break;
            }
            case AudioInputSourceType::Microphone:
            {
                ptr.reset(aznew AudioInputMicrophone(sourceConfig));
                break;
            }
            case AudioInputSourceType::ExternalStream:
            {
                ptr.reset(aznew AudioInputStreaming(sourceConfig));
                break;
            }
            case AudioInputSourceType::Synthesis:       // Will need to allow setting a user-defined Generate callback.
            default:
            {
                AZ_TracePrintf("AudioSourceManager", "AudioSourceManager::CreateSource - The type of AudioInputSource requested is not supported yet!\n");
                return INVALID_AUDIO_SOURCE_ID;
            }
        }

        if (!ptr || !ptr->IsOk())
        {   // this check could change in the future as we add asynch loading.
            return false;
        }

        AZStd::lock_guard<AZStd::mutex> lock(m_inputMutex);

        ptr->SetSourceId(sourceConfig.m_sourceId);
        m_inactiveAudioInputs.emplace(sourceConfig.m_sourceId, AZStd::move(ptr));

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioSourceManager::ActivateSource(TAudioSourceId sourceId, AkPlayingID playingId)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_inputMutex);

        if (m_inactiveAudioInputs.find(sourceId) != m_inactiveAudioInputs.end())
        {
            if (m_activeAudioInputs.find(playingId) == m_activeAudioInputs.end())
            {
                m_inactiveAudioInputs[sourceId]->SetSourceId(sourceId);
                m_activeAudioInputs[playingId] = AZStd::move(m_inactiveAudioInputs[sourceId]);
                m_inactiveAudioInputs.erase(sourceId);

                m_activeAudioInputs[playingId]->OnActivated();
            }
            else
            {
                AZ_TracePrintf("AudioSourceManager", "AudioSourceManager::ActivateSource - Active source with playing Id %u already exists!\n", playingId);
            }
        }
        else
        {
            AZ_TracePrintf("AudioSourceManager", "AudioSourceManager::ActivateSource - Source with Id %u not found!\n", sourceId);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioSourceManager::DeactivateSource(AkPlayingID playingId)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_inputMutex);

        if (m_activeAudioInputs.find(playingId) != m_activeAudioInputs.end())
        {
            TAudioSourceId sourceId = m_activeAudioInputs[playingId]->GetSourceId();
            if (m_inactiveAudioInputs.find(sourceId) == m_inactiveAudioInputs.end())
            {
                m_inactiveAudioInputs[sourceId] = AZStd::move(m_activeAudioInputs[playingId]);
                m_activeAudioInputs.erase(playingId);

                // Signal to the audio input source that it was deactivated!  It might unload it's resources.
                m_inactiveAudioInputs[sourceId]->OnDeactivated();

                if (!m_inactiveAudioInputs[sourceId]->IsOk())
                {
                    m_inactiveAudioInputs.erase(sourceId);
                }
            }
            else
            {
                AZ_TracePrintf("AudioSourceManager", "AudioSourceManager::DeactivateSource - Source with Id %u was already inactive!\n", sourceId);
            }
        }
        else
        {
            AZ_TracePrintf("AudioSourceManager", "AudioSourceManager::DeactivateSource - Active source with playing Id %u not found!\n", playingId);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void AudioSourceManager::DestroySource(TAudioSourceId sourceId)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_inputMutex);

        if (m_inactiveAudioInputs.find(sourceId) != m_inactiveAudioInputs.end())
        {
            m_inactiveAudioInputs.erase(sourceId);
        }
        else
        {
            AZ_TracePrintf("AudioSourceManager", "AudioSourceManager::DestroySource - No source with Id %u was found!\nDid you call DeactivateSource first on the playingId??\n", sourceId);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AkPlayingID AudioSourceManager::FindPlayingSource(TAudioSourceId sourceId)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_inputMutex);

        for (auto& inputPair : m_activeAudioInputs)
        {
            if (inputPair.second->GetSourceId() == sourceId)
            {
                return inputPair.first;
            }
        }

        return AK_INVALID_PLAYING_ID;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // static
    void AudioSourceManager::ExecuteCallback(AkPlayingID playingId, AkAudioBuffer* akBuffer)
    {
        if (!akBuffer->HasData())
        {
            akBuffer->eState = AK_Fail;
            akBuffer->uValidFrames = 0;
            return;
        }

        if (akBuffer->eState == AK_NoDataNeeded)
        {
            akBuffer->eState = AK_NoDataReady;
            akBuffer->uValidFrames = 0;
            return;
        }

        AZStd::lock_guard<AZStd::mutex> lock(Get().m_inputMutex);

        auto inputIter = Get().m_activeAudioInputs.find(playingId);
        if (inputIter != Get().m_activeAudioInputs.end())
        {
            auto& audioInput = inputIter->second;
            if (audioInput)
            {
                // this will set the uValidFrames and eState for us.
                audioInput->WriteOutput(akBuffer);
            }
        }
        else
        {
            // signal that the audio input playback should end.
            akBuffer->eState = AK_NoMoreData;
            akBuffer->uValidFrames = 0;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // static
    void AudioSourceManager::GetFormatCallback(AkPlayingID playingId, AkAudioFormat& audioFormat)
    {
        AZStd::lock_guard<AZStd::mutex> lock(Get().m_inputMutex);

        auto inputIter = Get().m_activeAudioInputs.find(playingId);
        if (inputIter != Get().m_activeAudioInputs.end())
        {
            // Set the AkAudioFormat from the AudioInputSource's SAudioInputConfig
            auto& audioInput = inputIter->second;
            if (audioInput)
            {
                audioInput->SetFormat(audioFormat);
            }
        }
    }

} // namespace Audio
