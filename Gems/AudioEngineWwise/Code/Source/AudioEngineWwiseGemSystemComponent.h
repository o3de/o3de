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

#pragma once

#include <AzCore/Component/Component.h>

#include <IAudioSystem.h>
#include <IAudioSystemImplementation.h>

#if defined(AUDIO_ENGINE_WWISE_EDITOR)
    #include <AzCore/std/smart_ptr/unique_ptr.h>
    #include <IAudioSystemEditor.h>
#endif // AUDIO_ENGINE_WWISE_EDITOR

struct SSystemInitParams;


namespace AudioEngineWwiseGem
{
    class AudioEngineWwiseGemSystemComponent
        : public AZ::Component
        , protected Audio::Gem::AudioEngineGemRequestBus::Handler
    #if defined(AUDIO_ENGINE_WWISE_EDITOR)
        , protected AudioControlsEditor::EditorImplPluginEventBus::Handler
    #endif // AUDIO_ENGINE_WWISE_EDITOR
    {
    public:
        AZ_COMPONENT(AudioEngineWwiseGemSystemComponent, "{521FA289-DC8B-4BC1-BBA0-A7D35EAC656E}", AZ::Component);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // Audio::Gem::AudioEngineGemRequestBus interface implementation
        bool Initialize(const SSystemInitParams* initParams) override;
        void Release() override;
        ////////////////////////////////////////////////////////////////////////

    #if defined(AUDIO_ENGINE_WWISE_EDITOR)
        ////////////////////////////////////////////////////////////////////////
        // AudioControlsEditor::EditorImplPluginEventBus interface implementation
        void InitializeEditorImplPlugin() override;
        void ReleaseEditorImplPlugin() override;
        AudioControls::IAudioSystemEditor* GetEditorImplPlugin() override;
        ////////////////////////////////////////////////////////////////////////
    #endif // AUDIO_ENGINE_WWISE_EDITOR

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        AZStd::unique_ptr<Audio::AudioSystemImplementation> m_engineWwise;

    #if defined(AUDIO_ENGINE_WWISE_EDITOR)
        AZStd::unique_ptr<AudioControls::IAudioSystemEditor> m_editorImplPlugin;
    #endif // AUDIO_ENGINE_WWISE_EDITOR
    };

} // namespace AudioEngineWwiseGem
