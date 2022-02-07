/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

#if defined(AUDIO_SYSTEM_EDITOR)
    #include <AzToolsFramework/API/ToolsApplicationAPI.h>
    #include <AudioControlsEditorPlugin.h>
    struct IEditor;
#endif // AUDIO_SYSTEM_EDITOR


namespace Audio
{
    // Module globals/statics
    CAudioLogger g_audioLogger;

    namespace Platform
    {
        void InitializeAudioAllocators();
        void ShutdownAudioAllocators();
    }
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

    AudioSystemGemSystemComponent::AudioSystemGemSystemComponent()
    {
        Audio::Platform::InitializeAudioAllocators();
    }

    AudioSystemGemSystemComponent::~AudioSystemGemSystemComponent()
    {
        // The audio system uses the Audio::AudioSystemAllocator
        // so it needs to be deleted before the allocator is shutdown
        m_audioSystem.reset();
        Audio::Platform::ShutdownAudioAllocators();
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
        using namespace Audio;

        // When nullptr is passed, create a NullAudioSystem instead of the real thing.
        if (!initParams)
        {
            return CreateNullAudioSystem();
        }

        bool success = false;

        if (CreateAudioSystem())
        {
            g_audioLogger.Log(eALT_ALWAYS, "AudioSystem created!");

            // Initialize the implementation module...
            bool initImplSuccess = false;
            Gem::AudioEngineGemRequestBus::BroadcastResult(initImplSuccess, &Gem::AudioEngineGemRequestBus::Events::Initialize);

            if (initImplSuccess)
            {
                PrepareAudioSystem();

                GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);
            }
            else
            {
                if (Gem::AudioEngineGemRequestBus::HasHandlers())
                {
                    g_audioLogger.Log(eALT_ERROR, "The Audio Engine did not initialize correctly!");
                }
                else
                {
                    g_audioLogger.Log(eALT_WARNING, "Running without any AudioEngine!");
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
        using namespace Audio;
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::Release);
        Gem::AudioEngineGemRequestBus::Broadcast(&Gem::AudioEngineGemRequestBus::Events::Release);

        // Delete the Audio System
        // It should be the last object that is freed from the audio system memory pool before the allocator is destroyed.
        m_audioSystem.reset();

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
