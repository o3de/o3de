/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AudioSystemGemSystemComponent.h"

#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AudioAllocators.h>
#include <AudioSystem.h>
#include <NullAudioSystem.h>

#if defined(AUDIO_SYSTEM_EDITOR)
    #include <AzToolsFramework/API/ToolsApplicationAPI.h>
    #include <AudioControlsEditorPlugin.h>
    struct IEditor;

    #include <Atom/RPI.Public/View.h>
    #include <Atom/RPI.Public/ViewportContext.h>
#endif // AUDIO_SYSTEM_EDITOR


namespace Audio
{
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
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AudioSystemGemSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AudioSystemGemService"));
    }

    void AudioSystemGemSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AudioSystemGemService"));
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
        CreateAudioSystem();
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
    #if defined(AUDIO_SYSTEM_EDITOR)
        m_cameraTransformHandler = AZ::RPI::MatrixChangedEvent::Handler(
            []([[maybe_unused]] const AZ::Matrix4x4& matrix)
            {
                auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
                auto viewportContext = atomViewportRequests->GetDefaultViewportContext();

                if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get();
                    audioSystem != nullptr && viewportContext != nullptr)
                {
                    Audio::ListenerRequest::SetWorldTransform setWorldTM;
                    setWorldTM.m_transform = viewportContext->GetCameraTransform();
                    audioSystem->PushRequest(AZStd::move(setWorldTM));
                }
            });
    #endif // AUDIO_SYSTEM_EDITOR
    }

    void AudioSystemGemSystemComponent::Activate()
    {
        Audio::Gem::SystemRequestBus::Handler::BusConnect();
        AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();
        AzFramework::LevelSystemLifecycleNotificationBus::Handler::BusConnect();

    #if defined(AUDIO_SYSTEM_EDITOR)
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();

        auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        auto defaultContextName = atomViewportRequests->GetDefaultViewportContextName();
        AZ::RPI::ViewportContextNotificationBus::Handler::BusConnect(defaultContextName);
    #endif // AUDIO_SYSTEM_EDITOR
    }

    void AudioSystemGemSystemComponent::Deactivate()
    {
        Audio::Gem::SystemRequestBus::Handler::BusDisconnect();
        AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
        AzFramework::LevelSystemLifecycleNotificationBus::Handler::BusDisconnect();

    #if defined(AUDIO_SYSTEM_EDITOR)
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();

        AZ::RPI::ViewportContextNotificationBus::Handler::BusDisconnect();
        m_cameraTransformHandler.Disconnect();
    #endif // AUDIO_SYSTEM_EDITOR
    }

    bool AudioSystemGemSystemComponent::Initialize()
    {
        using namespace Audio;

        bool success = AZ::Interface<Audio::IAudioSystem>::Get()->Initialize();
        if (success)
        {
            // Initialize the implementation module...
            bool initImplSuccess = false;
            Gem::EngineRequestBus::BroadcastResult(initImplSuccess, &Gem::EngineRequestBus::Events::Initialize);

            if (initImplSuccess)
            {
                PrepareAudioSystem();
            }
            else
            {
                if (Gem::EngineRequestBus::HasHandlers())
                {
                    AZLOG_ERROR("%s", "The Audio Engine did not initialize correctly!");
                }
                else
                {
                    AZLOG_NOTICE("%s", "Running without any Audio Engine!");
                }
            }

            success = true;
        }

        return success;
    }

    void AudioSystemGemSystemComponent::Release()
    {
        AZ_Assert(AZ::Interface<Audio::IAudioSystem>::Get() != nullptr, "The IAudioSystem interface has already been unregistered!");
        AZ::Interface<Audio::IAudioSystem>::Get()->Release();

        Audio::Gem::EngineRequestBus::Broadcast(&Audio::Gem::EngineRequestBus::Events::Release);
    }

    void AudioSystemGemSystemComponent::OnLoadingStart([[maybe_unused]] const char* levelName)
    {
        AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().GarbageCollect();
    }

    void AudioSystemGemSystemComponent::OnUnloadComplete([[maybe_unused]] const char* levelName)
    {
        AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().GarbageCollect();
    }

    void AudioSystemGemSystemComponent::OnApplicationConstrained(Event)
    {
        if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
        {
            Audio::SystemRequest::LoseFocus loseFocus;
            audioSystem->PushRequest(AZStd::move(loseFocus));
        }
    }

    void AudioSystemGemSystemComponent::OnApplicationUnconstrained(Event)
    {
        if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
        {
            Audio::SystemRequest::GetFocus getFocus;
            audioSystem->PushRequest(AZStd::move(getFocus));
        }
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

    void AudioSystemGemSystemComponent::OnViewportDefaultViewChanged([[maybe_unused]] AZ::RPI::ViewPtr view)
    {
        auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        auto viewportContext = atomViewportRequests->GetDefaultViewportContext();

        if (viewportContext != nullptr)
        {
            m_cameraTransformHandler.Disconnect();
            viewportContext->ConnectViewMatrixChangedHandler(m_cameraTransformHandler);
        }
    }
#endif // AUDIO_SYSTEM_EDITOR

    void AudioSystemGemSystemComponent::CreateAudioSystem()
    {
        int audioIsDisabled = 0;
        if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            [[maybe_unused]] auto result = console->GetCvarValue("sys_audio_disable", audioIsDisabled);
            AZ_Warning("AudioSystem", result == AZ::GetValueResult::Success,
                "Failed to get the 's_AudioDisable' Cvar, result is %d\n", static_cast<int>(result));
        }

        if (audioIsDisabled)
        {
            m_audioSystem = AZStd::make_unique<Audio::NullAudioSystem>();
            AZLOG_INFO("%s", "Null AudioSystem created!");
        }
        else
        {
            m_audioSystem = AZStd::make_unique<Audio::CAudioSystem>();
            AZLOG_INFO("%s", "AudioSystem created!");
        }
    }

    void AudioSystemGemSystemComponent::PrepareAudioSystem()
    {
        using namespace Audio;
        if (auto audioSystem = AZ::Interface<IAudioSystem>::Get(); audioSystem != nullptr)
        {
            // This is called when a new audio implementation has been set,
            // so update the controls path before we start loading data...
            audioSystem->UpdateControlsPath();
            const char* controlsPath = audioSystem->GetControlsPath();

            Audio::SystemRequest::LoadControls loadControls;
            loadControls.m_controlsPath = (controlsPath ? controlsPath : "");
            loadControls.m_scope = eADS_GLOBAL;
            audioSystem->PushRequestBlocking(AZStd::move(loadControls));

            Audio::SystemRequest::LoadBank loadBank;
            loadBank.m_asyncLoad = false;
            loadBank.m_preloadRequestId = ATLInternalControlIDs::GlobalPreloadRequestID;
            audioSystem->PushRequestBlocking(AZStd::move(loadBank));
        }
    }

} // namespace AudioSystemGem
