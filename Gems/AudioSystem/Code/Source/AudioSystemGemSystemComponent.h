/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <IAudioSystem.h>
#include <ISystem.h>

#if defined(AUDIO_SYSTEM_EDITOR)
    #include <AzCore/std/smart_ptr/unique_ptr.h>
    #include <Include/IPlugin.h>
    #include <AzToolsFramework/API/ToolsApplicationAPI.h>

    #include <Atom/RPI.Public/ViewportContextBus.h>
#endif // AUDIO_SYSTEM_EDITOR


namespace AudioSystemGem
{
    class AudioSystemGemSystemComponent
        : public AZ::Component
        , public ISystemEventListener
        , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
        , protected Audio::Gem::AudioSystemGemRequestBus::Handler
    #if defined(AUDIO_SYSTEM_EDITOR)
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , public AZ::RPI::ViewportContextNotificationBus::Handler
    #endif // AUDIO_SYSTEM_EDITOR
    {
    public:
        AZ_COMPONENT(AudioSystemGemSystemComponent, "{55095EE9-38E6-485F-8314-DF35CDFECC6B}", AZ::Component);
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        AudioSystemGemSystemComponent();
        ~AudioSystemGemSystemComponent() override;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // Audio::Gem::AudioSystemGemRequestBus interface implementation
        bool Initialize(const SSystemInitParams* initParams) override;
        void Release() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ISystemEventListener
        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzFramework::ApplicationLifecycleEvents interface implementation
        void OnApplicationConstrained(Event /*lastEvent*/) override;
        void OnApplicationUnconstrained(Event /*lastEvent*/) override;
        ////////////////////////////////////////////////////////////////////////

    #if defined(AUDIO_SYSTEM_EDITOR)
        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorEvents::Bus interface implementation
        void NotifyRegisterViews() override;
        void NotifyIEditorAvailable(IEditor*) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::RPI::ViewportContextNotificationBus::Handler interface implementation
        void OnViewportDefaultViewChanged(AZ::RPI::ViewPtr view) override;
        ////////////////////////////////////////////////////////////////////////
    #endif // AUDIO_SYSTEM_EDITOR

    private:
        ////////////////////////////////////////////////////////////////////////
        bool CreateAudioSystem();
        bool CreateNullAudioSystem();
        void PrepareAudioSystem();

        /// This is here to express ownership
        AZStd::unique_ptr<Audio::IAudioSystem> m_audioSystem;

    #if defined(AUDIO_SYSTEM_EDITOR)
        AZStd::unique_ptr<IPlugin> m_editorPlugin;
        AZ::RPI::MatrixChangedEvent::Handler m_cameraTransformHandler;
    #endif // AUDIO_SYSTEM_EDITOR
    };

} // namespace AudioSystemGem
