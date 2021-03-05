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

#include <AudioSystemGemSystemComponent.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AudioAllocators.h>
#include <AudioLogger.h>
#include <AudioSystem.h>
#include <NullAudioSystem.h>
#include <SoundCVars.h>

#if defined(AUDIO_SYSTEM_EDITOR)
    #include <AzToolsFramework/API/ToolsApplicationAPI.h>
    #include <AudioControlsEditorPlugin.h>
    struct IEditor;
#endif // AUDIO_SYSTEM_EDITOR


namespace Audio
{
    // Module globals/statics
    CSoundCVars g_audioCVars;
    CAudioLogger g_audioLogger;
    AZ::EnvironmentVariable<int*> g_audioVerbosityVar;
} // namespace Audio

namespace AudioSystemGem
{
#if defined(AUDIO_SYSTEM_EDITOR)
    IEditor* g_editor = nullptr;
#endif // AUDIO_SYSTEM_EDITOR

    void AudioSystemGemSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AudioSystemGemSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AudioSystemGemSystemComponent>("Audio System Gem", "Audio System handles requests and managages data related to the audio sub-system")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AudioSystemGemSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("AudioSystemGemService"));
    }

    void AudioSystemGemSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("AudioSystemGemService"));
    }

    void AudioSystemGemSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void AudioSystemGemSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AudioSystemGemSystemComponent::Init()
    {
        m_loseFocusRequest.nFlags = Audio::eARF_PRIORITY_HIGH;
        m_loseFocusRequest.pData = &m_loseFocusData;

        m_getFocusRequest.nFlags = Audio::eARF_PRIORITY_HIGH;
        m_getFocusRequest.pData = &m_getFocusData;
    }

    void AudioSystemGemSystemComponent::Activate()
    {
        Audio::Gem::AudioSystemGemRequestBus::Handler::BusConnect();
        AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();

    #if defined(AUDIO_SYSTEM_EDITOR)
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    #endif // AUDIO_SYSTEM_EDITOR
    }

    void AudioSystemGemSystemComponent::Deactivate()
    {
        Audio::Gem::AudioSystemGemRequestBus::Handler::BusDisconnect();
        AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();

    #if defined(AUDIO_SYSTEM_EDITOR)
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    #endif // AUDIO_SYSTEM_EDITOR
    }

    bool AudioSystemGemSystemComponent::Initialize(const SSystemInitParams* initParams)
    {
        // When nullptr is passed, create a NullAudioSystem instead of the real thing.
        if (!initParams)
        {
            return CreateNullAudioSystem();
        }

        Audio::g_audioCVars.RegisterVariables();

    #if !defined(AUDIO_RELEASE)
        Audio::g_audioVerbosityVar = AZ::Environment::CreateVariable<int*>("AudioLogVerbosity");
        Audio::g_audioVerbosityVar.Set(&Audio::g_audioCVars.m_nAudioLoggingOptions);
    #endif // !AUDIO_RELEASE

        bool success = false;

        // initialize audio system memory pool
        if (!AZ::AllocatorInstance<Audio::AudioSystemAllocator>::IsReady())
        {
            const size_t poolSize = Audio::g_audioCVars.m_nATLPoolSize << 10;

            Audio::AudioSystemAllocator::Descriptor allocDesc;

            // Generic Allocator:
            allocDesc.m_allocationRecords = true;
            allocDesc.m_heap.m_numFixedMemoryBlocks = 1;
            allocDesc.m_heap.m_fixedMemoryBlocksByteSize[0] = poolSize;

            allocDesc.m_heap.m_fixedMemoryBlocks[0] = AZ::AllocatorInstance<AZ::OSAllocator>::Get().Allocate(
                allocDesc.m_heap.m_fixedMemoryBlocksByteSize[0],
                allocDesc.m_heap.m_memoryBlockAlignment
            );

            AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Create(allocDesc);
        }

        if (CreateAudioSystem())
        {
            Audio::g_audioLogger.Log(Audio::eALT_ALWAYS, "AudioSystem created!");

            // Initialize the implementation module...
            bool initImplSuccess = false;
            Audio::Gem::AudioEngineGemRequestBus::BroadcastResult(initImplSuccess, &Audio::Gem::AudioEngineGemRequestBus::Events::Initialize, initParams);

            if (initImplSuccess)
            {
                PrepareAudioSystem();

                GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);
            }
            else
            {
                if (Audio::Gem::AudioEngineGemRequestBus::HasHandlers())
                {
                    Audio::g_audioLogger.Log(Audio::eALT_ERROR, "The Audio Engine did not initialize correctly!");
                }
                else
                {
                    Audio::g_audioLogger.Log(Audio::eALT_WARNING, "Running without any AudioEngine!");
                }
            }

            success = true;
        }

        return success;
    }

    bool AudioSystemGemSystemComponent::CreateNullAudioSystem()
    {
        m_audioSystem = AZStd::make_unique<Audio::NullAudioSystem>();
        return (m_audioSystem != nullptr);
    }

    void AudioSystemGemSystemComponent::Release()
    {
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::Release);
        Audio::Gem::AudioEngineGemRequestBus::Broadcast(&Audio::Gem::AudioEngineGemRequestBus::Events::Release);

        // Delete the Audio System
        // It should be the last object that is freed from the audio system memory pool before the allocator is destroyed.
        m_audioSystem.reset();

        if (AZ::AllocatorInstance<Audio::AudioSystemAllocator>::IsReady())
        {
            AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Destroy();
        }

        Audio::g_audioVerbosityVar.Reset();
        Audio::g_audioCVars.UnregisterVariables();

        GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
    }

    void AudioSystemGemSystemComponent::OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
    {
        switch (event)
        {
            case ESYSTEM_EVENT_LEVEL_LOAD_START:
            {
                AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().GarbageCollect();
                break;
            }
            case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
            {
                AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().GarbageCollect();
                break;
            }
        }
    }

    void AudioSystemGemSystemComponent::OnApplicationConstrained(Event)
    {
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, m_loseFocusRequest);
    }

    void AudioSystemGemSystemComponent::OnApplicationUnconstrained(Event)
    {
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, m_getFocusRequest);
    }

#if defined(AUDIO_SYSTEM_EDITOR)
    void AudioSystemGemSystemComponent::NotifyIEditorAvailable(IEditor* editor)
    {
        g_editor = editor;
    }

    void AudioSystemGemSystemComponent::NotifyRegisterViews()
    {
        if (g_editor)
        {
            if (m_editorPlugin)
            {
                m_editorPlugin->Release();
            }

            m_editorPlugin.reset(new CAudioControlsEditorPlugin(g_editor));
        }
    }
#endif // AUDIO_SYSTEM_EDITOR

    bool AudioSystemGemSystemComponent::CreateAudioSystem()
    {
        AZ_Assert(!Audio::AudioSystemRequestBus::HasHandlers(), "CreateAudioSystem - The AudioSystemRequestBus is already set up and connected!");

        bool success = false;
        m_audioSystem = AZStd::make_unique<Audio::CAudioSystem>();
        if (m_audioSystem)
        {
            Audio::AudioSystemRequestBus::BroadcastResult(success, &Audio::AudioSystemRequestBus::Events::Initialize);
        }
        else
        {
            Audio::g_audioLogger.Log(Audio::eALT_ERROR, "Could not create AudioSystem!");
        }

        return success;
    }

    void AudioSystemGemSystemComponent::PrepareAudioSystem()
    {
        using namespace Audio;

        // This is called when a new audio implementation has been set,
        // so update the controls path before we start loading data...
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::UpdateControlsPath);

        // Must be blocking requests.
        SAudioRequest oAudioRequestData;
        oAudioRequestData.nFlags = eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING;

        const char* controlsPath = nullptr;
        AudioSystemRequestBus::BroadcastResult(controlsPath, &AudioSystemRequestBus::Events::GetControlsPath);

        SAudioManagerRequestData<eAMRT_PARSE_CONTROLS_DATA> oAMData(controlsPath, eADS_GLOBAL);
        oAudioRequestData.pData = &oAMData;
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);

        SAudioManagerRequestData<eAMRT_PARSE_PRELOADS_DATA> oAMData2(controlsPath, eADS_GLOBAL);
        oAudioRequestData.pData = &oAMData2;
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);

        SAudioManagerRequestData<eAMRT_PRELOAD_SINGLE_REQUEST> oAMData3(ATLInternalControlIDs::GlobalPreloadRequestID);
        oAudioRequestData.pData = &oAMData3;
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);
    }

} // namespace AudioSystemGem
