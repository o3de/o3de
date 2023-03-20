/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MicrophoneSystemComponent.h"

#include <audioclient.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AudioRingBuffer.h>
#include <IAudioInterfacesCommonData.h>

#if defined(USE_LIBSAMPLERATE)
    #include <samplerate.h>
#endif // USE_LIBSAMPLERATE

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    class MicrophoneSystemComponentWindows : public MicrophoneSystemComponent::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(MicrophoneSystemComponentWindows, AZ::SystemAllocator);

        ///////////////////////////////////////////////////////////////////////////////////////////
        bool InitializeDevice() override
        {
            AZ_TracePrintf("WindowsMicrophone", "Initializing Microphone device - Windows!!\n");

            // Assert: m_enumerator, m_device, m_audioClient, m_audioCaptureClient are all nullptr!
            AZ_Assert(!m_enumerator && !m_device && !m_audioClient && !m_audioCaptureClient, "InitializeDevice - One or more pointers are not null before init!\n");

            // This Gem initializes very early, before Qt application is booted.
            // Qt calls OleInitialize internally, which initializes COM with Apartment Threaded model.
            // To avoid errors, we initialize COM here with the same model.
            CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

            const CLSID CLSID_MMDeviceEnumerator_UUID = __uuidof(MMDeviceEnumerator);
            const IID IID_IMMDeviceEnumerator_UUID = __uuidof(IMMDeviceEnumerator);
            HRESULT hresult = CoCreateInstance(
                CLSID_MMDeviceEnumerator_UUID, nullptr,
                CLSCTX_ALL, IID_IMMDeviceEnumerator_UUID,
                reinterpret_cast<void**>(&m_enumerator)
            );

            if (FAILED(hresult))
            {
                AZ_Error("WindowsMicrophone", false, "Failed to create an MMDeviceEnumerator!\n");
                return false;
            }

            if (m_enumerator)
            {
                hresult = m_enumerator->GetDefaultAudioEndpoint(EDataFlow::eCapture, ERole::eConsole, &m_device);

                if (!m_device || hresult == ERROR_NOT_FOUND)
                {
                    AZ_Warning("WindowsMicrophone", false, "No Microphone Device found!\n");
                    return false;
                }

                if (FAILED(hresult))
                {
                    AZ_Error("WindowsMicrophone", false, "HRESULT %d received while getting the default endpoint!\n", hresult);
                    return false;
                }

                IPropertyStore* m_deviceProps = nullptr;
                hresult = m_device->OpenPropertyStore(STGM_READ, &m_deviceProps);

                if (FAILED(hresult))
                {
                    AZ_Warning("WindowsMicrophone", false, "Failed to open the enpoint device's properties!\n");
                    // not a full failure
                }

                PROPVARIANT endpointName;
                PropVariantInit(&endpointName);
                hresult = m_deviceProps->GetValue(PKEY_Device_FriendlyName, &endpointName);

                if (FAILED(hresult))
                {
                    AZ_Warning("WindowsMicrophone", false, "Failed to get the endpoint device's friendly name!\n");
                    // not a full failure
                }
                else
                {
                    AZ_TracePrintf("WindowsMicrophone", "Microphone Endpoint Device Initialized: %S\n", endpointName.pwszVal);
                    m_deviceName = endpointName.pwszVal;
                }

                PropVariantClear(&endpointName);
                if (m_deviceProps)
                {
                    m_deviceProps->Release();
                    m_deviceProps = nullptr;
                }
            }

            return true;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        void ShutdownDevice() override
        {
            AZ_TracePrintf("WindowsMicrophone", "Shutting down Microphone device - Windows!\n");

            // Assert: m_audioClient and m_audioCaptureClient are both nullptr!  (i.e. the capture thread is not running)
            AZ_Assert(!m_audioClient && !m_audioCaptureClient, "ShutdownDevice - Audio Client pointers are not null!  You need to call EndSession first!\n");

            if (m_device)
            {
                m_device->Release();
                m_device = nullptr;
            }
            if (m_enumerator)
            {
                m_enumerator->Release();
                m_enumerator = nullptr;
            }

            CoUninitialize();
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        bool StartSession() override
        {
            AZ_TracePrintf("WindowsMicrophone", "Starting Microphone session - Windows!\n");
            AZ_Assert(m_device != nullptr, "Attempting to start a Microphone session while the device is uninitialized - Windows!\n");

            // Get the IAudioClient from the device
            const IID IID_IAudioClient_UUID = __uuidof(IAudioClient);
            HRESULT hresult = m_device->Activate(IID_IAudioClient_UUID, CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&m_audioClient));

            if (FAILED(hresult))
            {
                AZ_Error("WindowsMicrophone", false, "Failed to get an IAudioClient on the device - Windows!\n");
                return false;
            }

            // Get the mix format of the IAudioClient
            hresult = m_audioClient->GetMixFormat(&m_streamFormat);

            if (FAILED(hresult))
            {
                AZ_Error("WindowsMicrophone", false, "Failed to get the mix format from the IAudioClient - Windows!\n");
                return false;
            }

            // Initialize the IAudioClient
            // Note: REFERENCE_TIME = 100 nanoseconds (1e2)
            //  --> 1e9 ns = 1 sec
            //  --> 1e7 (REFTIMES_PER_SEC) * 1e2 (REFERENCE_TIME) ns = 1 sec
            const AZ::u64 REFTIMES_PER_SEC = 10000000;
            REFERENCE_TIME duration = REFTIMES_PER_SEC;
            hresult = m_audioClient->Initialize(
                AUDCLNT_SHAREMODE_SHARED,   // Share Mode
                0,                          // Stream Flags
                duration,                   // Buffer Duration
                0,                          // Periodicity
                m_streamFormat,             // Wave Format Ex
                nullptr                     // Audio Session GUID
            );

            if (FAILED(hresult))
            {
                AZ_Error("WindowsMicrophone", false, "Failed to initialize the IAudioClient - Windows!\n");
                return false;
            }

            // Get size of the allocated buffer
            hresult = m_audioClient->GetBufferSize(&m_bufferFrameCount);

            if (FAILED(hresult))
            {
                AZ_Error("WindowsMicrophone", false, "Failed to get the buffer size of the IAudioClient - Windows!\n");
                return false;
            }

            // Get the IAudioCaptureClient
            const IID IID_IAudioCaptureClient_UUID = __uuidof(IAudioCaptureClient);
            hresult = m_audioClient->GetService(IID_IAudioCaptureClient_UUID, reinterpret_cast<void**>(&m_audioCaptureClient));

            if (FAILED(hresult))
            {
                // Some possible results: (hresult == E_NOINTERFACE || hresult == AUDCLNT_E_NOT_INITIALIZED || hresult == AUDCLNT_E_WRONG_ENDPOINT_TYPE)
                AZ_Error("WindowsMicrophone", false, "Failed to get an IAudioCaptureClient service interface - Windows!\n");
                return false;
            }

            // Set format for internal sink
            SetFormatInternal(m_bufferFrameCount);

            if (!ValidateFormatInternal())
            {
                AZ_Error("WindowsMicrophone", false, "Failed to set a supported format - Windows!\n");
                return false;
            }

            AllocateBuffersInternal();

            m_bufferDuration = static_cast<double>(REFTIMES_PER_SEC) * (m_bufferFrameCount / m_streamFormat->nSamplesPerSec);

            // Start recording!
            hresult = m_audioClient->Start();

            if (FAILED(hresult))
            {
                AZ_Error("WindowsMicrophone", false, "Failed to start Microphone recording - Windows!\n");
                return false;
            }

            // Spawn a new thread with the basic capture loop: [GetNextPacketSize, GetBuffer, CopyData, ReleaseBuffer]
            m_capturing = true;
            AZStd::thread_desc threadDesc;
            threadDesc.m_name = "MicrophoneCapture-WASAPI";
            auto captureFunc = AZStd::bind(&MicrophoneSystemComponentWindows::RunAudioCapture, this);
            m_captureThread = AZStd::thread(threadDesc, captureFunc);

            return true;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        void RunAudioCapture()
        {
            const AZ::u64 REFTIMES_PER_MILLISEC = 10000;
            AZ::u32 numFramesAvailable = 0;
            AZ::u32 packetLength = 0;
            DWORD bufferFlags = 0;

            AZ::u8* data = nullptr;

            while (m_capturing)
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(static_cast<AZ::u64>(m_bufferDuration) / REFTIMES_PER_MILLISEC / 2));

                HRESULT hresult = m_audioCaptureClient->GetNextPacketSize(&packetLength);

                if (FAILED(hresult))
                {
                    AZ_Error("WindowsMicrophone", false, "Failed to GetNextPacketSize, ending thread - Windows!\n");
                    m_capturing = false;
                    continue;
                }

                while (m_capturing && packetLength != 0)
                {
                    bufferFlags = 0;
                    hresult = m_audioCaptureClient->GetBuffer(&data, &numFramesAvailable, &bufferFlags, nullptr, nullptr);

                    if (FAILED(hresult))
                    {
                        AZ_Error("WindowsMicrophone", false, "Failed to GetBuffer, ending thread - Windows!\n");
                        packetLength = 0;
                        m_capturing = false;
                        continue;
                    }

                    if (bufferFlags & AUDCLNT_BUFFERFLAGS_SILENT)
                    {
                        data = nullptr;     // signals internal buffer to write silence
                    }

                    if (!CopyDataInternal(data, numFramesAvailable))
                    {
                        numFramesAvailable = 0;
                    }

                    hresult = m_audioCaptureClient->ReleaseBuffer(numFramesAvailable);

                    if (FAILED(hresult))
                    {
                        AZ_Error("WindowsMicrophone", false, "Failed to ReleaseBuffer, ending thread - Windows!\n");
                        packetLength = 0;
                        m_capturing = false;
                        continue;
                    }

                    hresult = m_audioCaptureClient->GetNextPacketSize(&packetLength);

                    if (FAILED(hresult))
                    {
                        AZ_Error("WindowsMicrophone", false, "Failed to GetNextPacketSize, ending thread - Windows!\n");
                        packetLength = 0;
                        m_capturing = false;
                        continue;
                    }
                }
            }

            // Any post-thread cleanup?
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        void EndSession() override
        {
            AZ_TracePrintf("WindowsMicrophone", "Ending Microphone session - Windows!\n");

            // Signal thread to end
            m_capturing = false;
            // Thread join
            if (m_captureThread.joinable())
            {
                m_captureThread.join();
                m_captureThread = AZStd::thread();  // destroy
            }

            AZ_TracePrintf("WindowsMicrophone", "Microphone capture thread ended - Windows!");

            if (m_audioClient)
            {
                // Stop recording!
                HRESULT hresult = m_audioClient->Stop();

                if (FAILED(hresult))
                {
                    AZ_Error("WindowsMicrophone", false, "Failed to stop Microphone recording - Windows!\n");
                }
            }

            if (m_audioCaptureClient)
            {
                m_audioCaptureClient->Release();
                m_audioCaptureClient = nullptr;
            }
            if (m_audioClient)
            {
                m_audioClient->Release();
                m_audioClient = nullptr;
            }
            CoTaskMemFree(m_streamFormat);
            m_streamFormat = nullptr;

            DeallocateBuffersInternal();
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        bool IsCapturing() override
        {
            return m_capturing;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        SAudioInputConfig GetFormatConfig() const override
        {
            return m_config;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        // Returns the number of sample frames obtained
        AZStd::size_t GetData(void** outputData, AZStd::size_t numFrames, const SAudioInputConfig& targetConfig, bool shouldDeinterleave) override
        {
            bool changeSampleType = (targetConfig.m_sampleType != m_config.m_sampleType);
            bool changeSampleRate = (targetConfig.m_sampleRate != m_config.m_sampleRate);
            bool changeNumChannels = (targetConfig.m_numChannels != m_config.m_numChannels);

        #if defined(USE_LIBSAMPLERATE)
            bool micFormatIsInt = (m_config.m_sampleType == AudioInputSampleType::Int);
            bool targetFormatIsInt = (targetConfig.m_sampleType == AudioInputSampleType::Int);
            bool stereoToMono = (targetConfig.m_numChannels == 1 && m_config.m_numChannels == 2);

            // Handle the default no-change case
            if (!(changeSampleType || changeSampleRate || changeNumChannels))
            {
                return m_captureData->ConsumeData(outputData, numFrames, m_config.m_numChannels, shouldDeinterleave);
            }

            // Consume mic data into the 'working' conversion buffer (In)...
            numFrames = m_captureData->ConsumeData(reinterpret_cast<void**>(&m_conversionBufferIn.m_data), numFrames, m_config.m_numChannels, false);

            if (micFormatIsInt && (changeSampleType || changeSampleRate || changeNumChannels))
            {
                // Do a prep [Int]-->[Float] conversion...
                src_short_to_float_array(
                    reinterpret_cast<AZ::s16*>(m_conversionBufferIn.m_data),
                    reinterpret_cast<float*>(m_conversionBufferOut.m_data),
                    static_cast<int>(numFrames * m_config.m_numChannels)
                );

                // Swap to move the 'working' buffer back to the 'In' buffer.
                AZStd::swap(m_conversionBufferIn.m_data, m_conversionBufferOut.m_data);
            }

            if (changeSampleRate)
            {
                if (m_srcState && targetConfig.m_sampleRate < m_config.m_sampleRate)
                {
                    // Setup Conversion Data
                    m_srcData.end_of_input = 0;
                    m_srcData.input_frames = static_cast<long>(numFrames);
                    m_srcData.output_frames = static_cast<long>(numFrames);
                    m_srcData.data_in = reinterpret_cast<float*>(m_conversionBufferIn.m_data);
                    m_srcData.data_out = reinterpret_cast<float*>(m_conversionBufferOut.m_data);

                    // Conversion ratio is output_sample_rate / input_sample_rate
                    m_srcData.src_ratio = static_cast<double>(targetConfig.m_sampleRate) / static_cast<double>(m_config.m_sampleRate);

                    // Process
                    int error = src_process(m_srcState, &m_srcData);
                    if (error != 0)
                    {
                        AZ_TracePrintf("WindowsMicrophone", "SRC(src_process): %s - Windows!\n", src_strerror(error));
                    }

                    AZ_Warning("WindowsMicrophone", numFrames == m_srcData.input_frames_used,
                        "SRC(src_process): Num Frames requested (%u) was different than Num Frames processed (%u) - Windows!\n",
                        numFrames, m_srcData.input_frames_used);

                    numFrames = m_srcData.output_frames_gen;

                    // Swap to move the 'working' buffer back to the 'In' buffer.
                    AZStd::swap(m_conversionBufferIn.m_data, m_conversionBufferOut.m_data);
                }
                else
                {
                    // Unable to continue: Either upsampling is requested or there is no resampler state.
                    // todo: Upsampling could be done if we reallocate the conversion buffers.  Right now we assume a max size of the buffer
                    // based on the size of the ringbuffer.  If samplerate is to increase, those buffers would need to increase in size
                    // too.  GetData is the only point which we know the target samplerate.
                    return 0;
                }
            }

            if (changeNumChannels)
            {
                if (stereoToMono)
                {
                    // Samples are interleaved now, copy only left channel to the output
                    float* bufferInputData = reinterpret_cast<float*>(m_conversionBufferIn.m_data);
                    float* bufferOutputData = reinterpret_cast<float*>(m_conversionBufferOut.m_data);
                    for (AZ::u32 frame = 0; frame < numFrames; ++frame)
                    {
                        bufferOutputData[frame] = *bufferInputData++;
                        ++bufferInputData;
                    }
                }
                else // monoToStereo
                {
                    // Split single samples to both left and right channels
                    if (shouldDeinterleave)
                    {
                        float* bufferInputData = reinterpret_cast<float*>(m_conversionBufferIn.m_data);
                        float** bufferOutputData = reinterpret_cast<float**>(m_conversionBufferOut.m_data);
                        for (AZ::u32 frame = 0; frame < numFrames; ++frame)
                        {
                            bufferOutputData[0][frame] = bufferOutputData[1][frame] = bufferInputData[frame];
                        }
                    }
                    else
                    {
                        float* bufferInputData = reinterpret_cast<float*>(m_conversionBufferIn.m_data);
                        float* bufferOutputData = reinterpret_cast<float*>(m_conversionBufferOut.m_data);
                        for (AZ::u32 frame = 0; frame < numFrames; ++frame)
                        {
                            *bufferOutputData++ = bufferInputData[frame];
                            *bufferOutputData++ = bufferInputData[frame];
                        }
                    }
                }

                // Swap to move the 'working' buffer back to the 'In' buffer.
                AZStd::swap(m_conversionBufferIn.m_data, m_conversionBufferOut.m_data);
            }

            if (targetFormatIsInt)
            {
                // Do a final [Float]-->[Int] conversion...
                src_float_to_short_array(
                    reinterpret_cast<float*>(m_conversionBufferIn.m_data),
                    *reinterpret_cast<AZ::s16**>(outputData),
                    static_cast<int>(numFrames * m_config.m_numChannels)
                );
            }
            else
            {
                // Otherwise, we're done, just memcpy the 'working' buffer to the output.
                ::memcpy(outputData, m_conversionBufferIn.m_data, numFrames * targetConfig.m_numChannels * (targetConfig.m_bitsPerSample >> 3));
            }

            return numFrames;

        #else

            if (changeSampleType || changeSampleRate || changeNumChannels)
            {
                // Without the SRC library, any change is unsupported!
                return 0;
            }
            else
            {
                // No change to the data from Input to Output
                return m_captureData->ConsumeData(outputData, numFrames, m_config.m_numChannels, shouldDeinterleave);
            }

        #endif // USE_LIBSAMPLERATE
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        void SetFormatInternal(AZ::u32 bufferFrameCount)
        {
            if (m_streamFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
            {
                auto streamFormatExt = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(m_streamFormat);
                if (streamFormatExt)
                {
                    if (streamFormatExt->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
                    {
                        AZ_TracePrintf("WindowsMicrophone", "PCM Format - Windows!\n");
                        m_config.m_sampleType = AudioInputSampleType::Int;
                    }
                    else if (streamFormatExt->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
                    {
                        AZ_TracePrintf("WindowsMicrophone", "IEEE Float Format - Windows!\n");
                        m_config.m_sampleType = AudioInputSampleType::Float;
                    }
                    else
                    {
                        m_config.m_sampleType = AudioInputSampleType::Unsupported;
                    }

                    if (streamFormatExt->dwChannelMask == KSAUDIO_SPEAKER_MONO)
                    {
                        AZ_TracePrintf("WindowsMicrophone", "Channel Format: Mono - Windows!\n");
                        m_config.m_numChannels = 1;
                    }
                    else if (streamFormatExt->dwChannelMask == KSAUDIO_SPEAKER_STEREO)
                    {
                        AZ_TracePrintf("WindowsMicrophone", "Channel Format: Stereo - Windows!\n");
                        m_config.m_numChannels = 2;
                    }
                    else
                    {
                        AZ_Error("WindowsMicrophone", false, "Only Mono and Stereo microphone inputs are supported - Windows!\n");
                        m_config.m_numChannels = 0;
                    }

                    m_config.m_sampleRate = m_streamFormat->nSamplesPerSec;
                    m_config.m_bitsPerSample = m_streamFormat->wBitsPerSample;

                    AZ_TracePrintf("WindowsMicrophone", "Sample Rate: %u - Windows!\n", m_config.m_sampleRate);
                    AZ_TracePrintf("WindowsMicrophone", "Bits Per Sample: %u - Windows!\n", m_config.m_bitsPerSample);

                    m_config.m_sourceType = Audio::AudioInputSourceType::Microphone;
                    m_config.SetBufferSizeFromFrameCount(bufferFrameCount);
                }
            }
            else
            {
                // Untested code path.
                // Every device I tested went through the wave format extensible path...
                if (m_streamFormat->wFormatTag == WAVE_FORMAT_PCM)
                {
                    m_config.m_sampleType = AudioInputSampleType::Int;
                }
                else if (m_streamFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
                {
                    m_config.m_sampleType = AudioInputSampleType::Float;
                }
                else
                {
                    m_config.m_sampleType = AudioInputSampleType::Unsupported;
                }

                m_config.m_numChannels = m_streamFormat->nChannels;
                m_config.m_sampleRate = m_streamFormat->nSamplesPerSec;
                m_config.m_bitsPerSample = m_streamFormat->wBitsPerSample;
                m_config.SetBufferSizeFromFrameCount(bufferFrameCount);
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        bool ValidateFormatInternal()
        {
            bool valid = true;
            if (m_config.m_numChannels < 1 || m_config.m_numChannels > 2)
            {
                AZ_TracePrintf("WindowsMicrophone", "Only Mono and Stereo Microphone inputs are supported - Windows!\n");
                valid = false;
            }

            if (m_config.m_sampleType == AudioInputSampleType::Unsupported)
            {
                AZ_TracePrintf("WindowsMicrophone", "Unsupported sample format detected - Windows!\n");
                valid = false;
            }

            if (m_config.m_sampleType == AudioInputSampleType::Int && m_config.m_bitsPerSample != 16)
            {
                AZ_TracePrintf("WindowsMicrophone", "Only bitdepths of 16 bits are supported with integer samples - Windows!\n");
                valid = false;
            }

            if (m_config.m_bufferSize == 0)
            {
                AZ_TracePrintf("WindowsMicrophone", "Buffer size for the Microphone input has not been set - Windows!\n");
                valid = false;
            }

            return valid;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        void AllocateBuffersInternal()
        {
            AZ_Assert(m_config.m_bufferSize > 0, "Format was checked already, but buffer size of the Microhpone input is zero - Windows!\n");

            DeallocateBuffersInternal();

            if (m_config.m_sampleType == AudioInputSampleType::Float)
            {
                AZ_Assert(m_config.m_bitsPerSample == 32, "Format was checked already, but non-32-bit float samples are detected - Windows!\n");
                m_captureData.reset(aznew Audio::RingBuffer<float>(m_config.GetSampleCountFromBufferSize()));
            }
            else if (m_config.m_sampleType == AudioInputSampleType::Int)
            {
                AZ_Assert(m_config.m_bitsPerSample == 16, "Format was checked already, but non-16-bit integer samples are detected - Windows!\n");
                m_captureData.reset(aznew Audio::RingBuffer<AZ::s16>(m_config.GetSampleCountFromBufferSize()));
            }

        #if defined(USE_LIBSAMPLERATE)
            // New SRC State
            if (!m_srcState)
            {
                int error = 0;
                m_srcState = src_new(SRC_SINC_MEDIUM_QUALITY, m_config.m_numChannels, &error);
                if (m_srcState)
                {
                    AZStd::size_t conversionBufferMaxSize = m_config.GetSampleCountFromBufferSize() * sizeof(float);    // Use this because float is the biggest sample type.

                    m_conversionBufferIn.m_data = static_cast<AZ::u8*>(azmalloc(conversionBufferMaxSize, MEMORY_ALLOCATION_ALIGNMENT, AZ::SystemAllocator));
                    m_conversionBufferIn.m_sizeBytes = conversionBufferMaxSize;

                    m_conversionBufferOut.m_data = static_cast<AZ::u8*>(azmalloc(conversionBufferMaxSize, MEMORY_ALLOCATION_ALIGNMENT, AZ::SystemAllocator));
                    m_conversionBufferOut.m_sizeBytes = conversionBufferMaxSize;

                    ::memset(m_conversionBufferIn.m_data, 0, m_conversionBufferIn.m_sizeBytes);
                    ::memset(m_conversionBufferOut.m_data, 0, m_conversionBufferOut.m_sizeBytes);
                }
                else
                {
                    AZ_TracePrintf("WindowsMicrophone", "SRC(src_new): %s - Windows!\n", src_strerror(error));
                }
            }
        #endif // USE_LIBSAMPLERATE
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        void DeallocateBuffersInternal()
        {
            if (m_captureData)
            {
                m_captureData.reset();
            }

        #if defined(USE_LIBSAMPLERATE)
            // Cleanup SRC State
            m_srcState = src_delete(m_srcState);
            m_srcState = nullptr;

            if (m_conversionBufferIn.m_data)
            {
                azfree(m_conversionBufferIn.m_data, AZ::SystemAllocator);
                m_conversionBufferIn.m_data = nullptr;
                m_conversionBufferIn.m_sizeBytes = 0;
            }

            if (m_conversionBufferOut.m_data)
            {
                azfree(m_conversionBufferOut.m_data, AZ::SystemAllocator);
                m_conversionBufferOut.m_data = nullptr;
                m_conversionBufferOut.m_sizeBytes = 0;
        }
        #endif // USE_LIBSAMPLERATE
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        bool CopyDataInternal(const AZ::u8* inputData, AZStd::size_t numFrames)
        {
            // This function should return false if unable to copy all the frames!  That way we can pass 0 to ReleaseBuffer to signal
            // that the buffer was not consumed yet.  The api states that between GetBuffer and corresponding ReleaseBuffer calls, you
            // need to read all of it or none of it.  If unable to copy the entire buffer, calling ReleaseBuffer with 0 means that the
            // next call to GetBuffer will return the same buffer that wasn't consumed.

            AZStd::size_t numFramesCopied = m_captureData->AddData(inputData, numFrames, m_config.m_numChannels);

            return numFramesCopied > 0;
        }

    private:
        // Device and format data...
        IAudioClient* m_audioClient = nullptr;
        IAudioCaptureClient* m_audioCaptureClient = nullptr;
        IMMDevice* m_device = nullptr;
        IMMDeviceEnumerator* m_enumerator = nullptr;
        WAVEFORMATEX* m_streamFormat = nullptr;

        // Thread data...
        AZStd::atomic<bool> m_capturing = false;
        AZStd::wstring m_deviceName;
        AZStd::thread m_captureThread;

        // Wasapi buffer data...
        double m_bufferDuration = 0;                    // in REFTIMES_PER_SEC (1e7)
        AZ::u32 m_bufferFrameCount = 0;                 // number of frames the Wasapi buffer holds

        Audio::SAudioInputConfig m_config;              // the format configuration

        AZStd::unique_ptr<Audio::RingBufferBase> m_captureData = nullptr;

    #if defined(USE_LIBSAMPLERATE)
        // Sample Rate Converter data...
        SRC_STATE* m_srcState = nullptr;
        SRC_DATA m_srcData;
        AudioStreamData m_conversionBufferIn;
        AudioStreamData m_conversionBufferOut;
    #endif // USE_LIBSAMPLERATE
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    MicrophoneSystemComponent::Implementation* MicrophoneSystemComponent::Implementation::Create()
    {
        return aznew MicrophoneSystemComponentWindows();
    }
} // namespace Audio
