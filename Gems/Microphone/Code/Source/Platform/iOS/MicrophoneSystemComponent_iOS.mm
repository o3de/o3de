/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>

#include "MicrophoneSystemComponent.h"
#include "SimpleDownsample.h"

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/EBus/EBus.h>

#include <AudioRingBuffer.h>
#include <IAudioInterfacesCommonData.h>

#if defined(USE_LIBSAMPLERATE)
    #include <samplerate.h>
#endif // USE_LIBSAMPLERATE

namespace Audio
{

class MicrophoneSystemComponentIOS : public MicrophoneSystemComponent::Implementation
{
public:
    bool InitializeDevice() override
    {
        m_isCapturing = false;

        OSStatus status;

        AudioComponentDescription desc;
        desc.componentType = kAudioUnitType_Output;
        desc.componentSubType = kAudioUnitSubType_RemoteIO;
        desc.componentFlags = 0;
        desc.componentFlagsMask = 0;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;

        AudioComponent inputComponent = AudioComponentFindNext(nullptr, &desc);

        status = AudioComponentInstanceNew(inputComponent, &m_audioUnit);
        if(status != 0)
        {
            AZ_Error("iOSMicrophone", false, "Error creating audio component: %d", status);
            return false;
        }

        AZ::u32 flag = 1;
        status = AudioUnitSetProperty(m_audioUnit,
            kAudioOutputUnitProperty_EnableIO,
            kAudioUnitScope_Input,
            kInputBus,
            &flag,
            sizeof(flag));
        if(status != 0)
        {
            AZ_Error("iOSMicrophone", false, "Error enabling audio input IO: %d", status);
            return false;
        }

        // disable output
        flag = 0;
        status = AudioUnitSetProperty(m_audioUnit,
            kAudioOutputUnitProperty_EnableIO,
            kAudioUnitScope_Output,
            kOutputBus,
            &flag,
            sizeof(flag));
        if(status != 0)
        {
            AZ_Error("iOSMicrophone", false, "Error enabling audio input IO: %d", status);
            return false;
        }

        AudioStreamBasicDescription audioFormat;
        audioFormat.mSampleRate = 44100.00;
        audioFormat.mFormatID = kAudioFormatLinearPCM;
        audioFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
        audioFormat.mFramesPerPacket = 1;
        audioFormat.mChannelsPerFrame = 1;
        audioFormat.mBitsPerChannel = 16;
        audioFormat.mBytesPerPacket = 2;
        audioFormat.mBytesPerFrame = 2;

           m_config.m_sampleRate = audioFormat.mSampleRate;
        m_config.m_numChannels = audioFormat.mChannelsPerFrame;
        m_config.m_bitsPerSample = audioFormat.mBitsPerChannel;
        m_config.SetBufferSizeFromFrameCount(8192);

        status = AudioUnitSetProperty(m_audioUnit,
            kAudioUnitProperty_StreamFormat,
            kAudioUnitScope_Output,
            kInputBus,
            &audioFormat,
            sizeof(audioFormat));

        if(status != 0)
        {
            AZ_Error("iOSMicrophone", false, "Error setting microphone inuput configuration: %d", status);
            return false;
        }

        AURenderCallbackStruct callbackStruct;
        callbackStruct.inputProc = recordingCallback;
        callbackStruct.inputProcRefCon = this;
        status = AudioUnitSetProperty(m_audioUnit,
            kAudioOutputUnitProperty_SetInputCallback, 
            kAudioUnitScope_Global, 
            kInputBus, 
            &callbackStruct, 
            sizeof(callbackStruct));

        if(status != 0)
        {
            AZ_Error("iOSMicrophone", false, "Error setting microphone callback: %d", status);
            return false;
        }


        // This disables buffer allocation as we provide our own
        flag = 0;
        status = AudioUnitSetProperty(m_audioUnit, 
            kAudioUnitProperty_ShouldAllocateBuffer,
            kAudioUnitScope_Output, 
            kInputBus,
            &flag, 
            sizeof(flag));
            
        // Initialise
        status = AudioUnitInitialize(m_audioUnit);
        if(status != 0)
        {
            AZ_Error("iOSMicrophone", false, "Error initializing microphone: %d", status);
            return false;
        }
        return true;
    }

    void ShutdownDevice() override
    {
        m_isCapturing = false;
        AudioUnitUninitialize(m_audioUnit);
    }

    bool StartSession() override
    {
        m_captureData.reset(aznew Audio::RingBuffer<AZ::s16>(m_config.GetSampleCountFromBufferSize()));
        OSStatus status = AudioOutputUnitStart(m_audioUnit);
        if(status != 0)
        {
            AZ_Error("iOSMicrophone", false, "Error starting microphone: %d", status);
            return false;
        }
        m_isCapturing = true;
        return true;
    }

    void EndSession() override
    {
        OSStatus status = AudioOutputUnitStop(m_audioUnit);
        if(status != 0)
        {
            AZ_Error("iOSMicrophone", false, "Error stopping microphone: %d", status);
        }

        m_isCapturing = false;
        if(m_captureData)
        {
            m_captureData.reset();
        }
    }

    bool IsCapturing() override
    {
        return m_isCapturing;
    }

    SAudioInputConfig GetFormatConfig() const override
    {
        return m_config;
    }

    AZStd::size_t GetData(void** outputData, AZStd::size_t numFrames, const SAudioInputConfig& targetConfig, bool shouldDeinterleave) override
    {
#if defined(USE_LIBSAMPLERATE)
// pending port of LIBSAMPLERATE to iOS
        return {};
#else
        bool changeSampleType = (targetConfig.m_sampleType != m_config.m_sampleType);
        bool changeSampleRate = (targetConfig.m_sampleRate != m_config.m_sampleRate);
        bool changeNumChannels = (targetConfig.m_numChannels != m_config.m_numChannels);

        if (changeSampleType || changeNumChannels)
        {
            // Without the SRC library, any change is unsupported!
            return {};
        }
        else if (changeSampleRate)
        {
            if(targetConfig.m_sampleRate > m_config.m_sampleRate)
            {
                AZ_Error("MacOSMicrophone", false, "Target sample rate is larger than source sample rate, this is not supported");
                return {};
            }

            auto sourceBuffer =  new AZ::s16[numFrames];
            AZStd::size_t targetSize = GetDownsampleSize(numFrames, m_config.m_sampleRate, targetConfig.m_sampleRate);
            auto targetBuffer = new AZ::s16[targetSize];

            numFrames = m_captureData->ConsumeData(reinterpret_cast<void**>(&sourceBuffer), numFrames, m_config.m_numChannels, false);

            if(numFrames > 0)
            {
                Downsample(sourceBuffer, numFrames, m_config.m_sampleRate, targetBuffer, targetSize, targetConfig.m_sampleRate);

                // swap target data to output
                ::memcpy(*outputData, targetBuffer, numFrames * targetConfig.m_numChannels * (targetConfig.m_bitsPerSample >> 3));
            }

            delete [] sourceBuffer;
            delete [] targetBuffer;

            return numFrames;
        }
        else
        {
            // No change to the data from Input to Output
            return m_captureData->ConsumeData(outputData, numFrames, m_config.m_numChannels, shouldDeinterleave);
        }
#endif   
        return {};
    }

    void ProcessAudio(AudioBufferList* bufferList)
    {
        AudioBuffer sourceBuffer = bufferList->mBuffers[0];
        m_captureData->AddData((AZ::s16*)sourceBuffer.mData, sourceBuffer.mDataByteSize / 2, m_config.m_numChannels);

    }

    AudioComponentInstance m_audioUnit;

private:
    static OSStatus recordingCallback(void* inRefCon, 
        AudioUnitRenderActionFlags* ioActionFlags, 
        const AudioTimeStamp* inTimeStamp, 
        AZ::u32 inBusNumber, 
        AZ::u32 inNumberFrames, 
        AudioBufferList* ioData) {
        
        auto impl = reinterpret_cast<MicrophoneSystemComponentIOS*>(inRefCon);

        // Samples are 16 bits = 2 bytes.
        // 1 frame includes only 1 sample
        // one channel as mono was selected in setup
        AudioBuffer buffer;
        
        buffer.mNumberChannels = 1;
        buffer.mDataByteSize = inNumberFrames * 2;
        buffer.mData = new AZ::u8[buffer.mDataByteSize];
        
        // Put buffer in a AudioBufferList
        AudioBufferList bufferList;
        bufferList.mNumberBuffers = 1;
        bufferList.mBuffers[0] = buffer;
        
        // Obtain recorded samples
        
        AudioUnitRender(impl->m_audioUnit, 
            ioActionFlags, 
            inTimeStamp, 
            inBusNumber, 
            inNumberFrames, 
            &bufferList);
        

        impl->ProcessAudio(&bufferList);
        
        // release the malloc'ed data in the buffer we created earlier
        delete [] (AZ::u8*)bufferList.mBuffers[0].mData;
        
        return noErr;
    }
    

    Audio::SAudioInputConfig m_config;
    bool m_isCapturing = false;
    AZStd::unique_ptr<Audio::RingBufferBase> m_captureData = nullptr;

    const AudioUnitElement kInputBus = 1;
    const AudioUnitElement kOutputBus = 0;

};

///////////////////////////////////////////////////////////////////////////////////////////////
MicrophoneSystemComponent::Implementation* MicrophoneSystemComponent::Implementation::Create()
{
    return aznew MicrophoneSystemComponentIOS();
}
}
