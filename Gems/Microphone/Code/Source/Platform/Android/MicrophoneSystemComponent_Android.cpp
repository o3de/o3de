/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MicrophoneSystemComponent.h"
#include "SimpleDownsample.h"

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/EBus/EBus.h>

#include <AudioRingBuffer.h>
#include <IAudioInterfacesCommonData.h>

#if defined(USE_LIBSAMPLERATE)
    #include <samplerate.h>
#endif // USE_LIBSAMPLERATE

#include <AzCore/Android/Utils.h>
#include <AzCore/Android/JNI/Object.h>

namespace Audio
{
    class MicrophoneSystemEventsAndroid : public AZ::EBusTraits 
    {
    public: 
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        virtual ~MicrophoneSystemEventsAndroid() = default;

        virtual void HandleIncomingData(AZ::s8* data, int size) {}
    };
    using MicrophoneSystemEventsAndroidBus = AZ::EBus<MicrophoneSystemEventsAndroid>;

    static void JNI_SendCurrentData(JNIEnv* jniEnv, jobject objectRef, jbyteArray data, jint size)
    {
        jbyte* bufferPtr = jniEnv->GetByteArrayElements(data, nullptr);
        MicrophoneSystemEventsAndroidBus::Broadcast(&MicrophoneSystemEventsAndroid::HandleIncomingData, bufferPtr, size);
        jniEnv->ReleaseByteArrayElements(data, bufferPtr, JNI_ABORT);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    class MicrophoneSystemComponentAndroid : public MicrophoneSystemComponent::Implementation
                                           , public MicrophoneSystemEventsAndroidBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(MicrophoneSystemComponentAndroid, AZ::SystemAllocator);

        bool InitializeDevice() override
        {
            AZ_TracePrintf("AndroidMicrophone", "Initializing Microphone device - Android!!\n");
    
            MicrophoneSystemEventsAndroidBus::Handler::BusConnect();

            m_JNIobject.RegisterStaticMethod("InitializeDevice", "()Z");
            m_JNIobject.RegisterStaticMethod("ShutdownDevice", "()V");
            m_JNIobject.RegisterStaticMethod("StartSession", "()Z");
            m_JNIobject.RegisterStaticMethod("EndSession", "()V");
            m_JNIobject.RegisterStaticMethod("IsCapturing", "()Z");
            m_JNIobject.RegisterNativeMethods(
                {
                    {"SendCurrentData", "([BI)V", (void*)JNI_SendCurrentData}
                }
            );

            // These are the Android "guaranteed" parameters
            // Note that this must match what is setup in MicrophoneSystemComponent.java
            // as this is the config that's will reflect the incoming data
            // and it will need to be compared to see if downsampling is required
            m_config.m_sampleRate = 44100;
            m_config.m_numChannels = 1;
            m_config.m_bitsPerSample = 16;
            m_config.m_sourceType = AudioInputSourceType::Microphone;
            m_config.m_sampleType = AudioInputSampleType::Int;
            m_config.SetBufferSizeFromFrameCount(512);

            return m_JNIobject.InvokeStaticBooleanMethod("InitializeDevice");
        }

        void ShutdownDevice() override
        {
            m_JNIobject.InvokeStaticVoidMethod("ShutdownDevice");

            MicrophoneSystemEventsAndroidBus::Handler::BusDisconnect();
        }

        bool StartSession() override
        {
            m_captureData.reset(aznew Audio::RingBuffer<AZ::s16>(4096));    // this is a good size to keep buffer filling and draining without gaps

            return m_JNIobject.InvokeStaticBooleanMethod("StartSession");
        }

        void EndSession() override
        {            
            m_JNIobject.InvokeStaticVoidMethod("EndSession");
            if(m_captureData)
            {
                m_captureData.reset();
            }
        }

        bool IsCapturing() override
        {
            return m_JNIobject.InvokeStaticBooleanMethod("IsCapturing");
        }

        SAudioInputConfig GetFormatConfig() const override
        {
            return m_config;
        }

        AZStd::size_t GetData(void** outputData, AZStd::size_t numFrames, const SAudioInputConfig& targetConfig, bool shouldDeinterleave)
        {
        #if defined(USE_LIBSAMPLERATE)
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

                    numFrames = targetSize;
                    // swap target data to output
                    ::memcpy(*outputData, targetBuffer, targetSize * 2); //*2 as two bytes per frame
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
        }

        void HandleIncomingData(AZ::s8* data, int size) override
        {
            m_captureData->AddData((AZ::s16*)data, size / 2, m_config.m_numChannels);
        }

    private:
        AZ::Android::JNI::Object m_JNIobject {"com/amazon/lumberyard/Microphone/MicrophoneSystemComponent"};
        Audio::SAudioInputConfig m_config; 
        AZStd::unique_ptr<Audio::RingBufferBase> m_captureData = nullptr;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    MicrophoneSystemComponent::Implementation* MicrophoneSystemComponent::Implementation::Create()
    {
        return aznew MicrophoneSystemComponentAndroid();
    }
}
