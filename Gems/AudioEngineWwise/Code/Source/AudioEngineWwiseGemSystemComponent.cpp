/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AudioEngineWwiseGemSystemComponent.h>

#include <AzCore/PlatformDef.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/Platform/PlatformDefaults.h>

#include <AudioAllocators.h>
#include <AudioSystemImplCVars.h>
#include <AudioSystemImpl_wwise.h>
#include <Common_wwise.h>
#include <Config_wwise.h>

#if defined(AUDIO_ENGINE_WWISE_EDITOR)
    #include <AudioSystemEditor_wwise.h>
#endif // AUDIO_ENGINE_WWISE_EDITOR


namespace Audio
{
    namespace Platform
    {
        void* InitializeSecondaryMemoryPool(size_t& secondarySize);
    }
} // namespace Audio


namespace AudioEngineWwiseGem
{
    void AudioEngineWwiseGemSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AudioEngineWwiseGemSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AudioEngineWwiseGemSystemComponent>("Audio Engine Wwise Gem", "Wwise implementation of the Audio Engine interfaces")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        Audio::Wwise::ConfigurationSettings::Reflect(context);
    }

    void AudioEngineWwiseGemSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("AudioEngineService"));
    }

    void AudioEngineWwiseGemSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("AudioEngineService"));
    }

    void AudioEngineWwiseGemSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("AudioSystemService"));
    }

    void AudioEngineWwiseGemSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("AudioSystemService"));
    }

    void AudioEngineWwiseGemSystemComponent::Init()
    {
    }

    void AudioEngineWwiseGemSystemComponent::Activate()
    {
        Audio::Gem::EngineRequestBus::Handler::BusConnect();

    #if defined(AUDIO_ENGINE_WWISE_EDITOR)
        AudioControlsEditor::EditorImplPluginEventBus::Handler::BusConnect();
    #endif // AUDIO_ENGINE_WWISE_EDITOR
    }

    void AudioEngineWwiseGemSystemComponent::Deactivate()
    {
        Audio::Gem::EngineRequestBus::Handler::BusDisconnect();

    #if defined(AUDIO_ENGINE_WWISE_EDITOR)
        AudioControlsEditor::EditorImplPluginEventBus::Handler::BusDisconnect();
    #endif // AUDIO_ENGINE_WWISE_EDITOR
    }

    bool AudioEngineWwiseGemSystemComponent::Initialize()
    {
        bool success = false;

        // Check memory-related Wwise Cvars...
        [[maybe_unused]] const AZ::u64 memorySubpartitionSizes = Audio::Wwise::Cvars::s_StreamDeviceMemorySize
#if !defined(WWISE_RELEASE)
            + Audio::Wwise::Cvars::s_MonitorQueueMemorySize
#endif // !WWISE_RELEASE
            + Audio::Wwise::Cvars::s_CommandQueueMemorySize;

        AZ_Assert(Audio::Wwise::Cvars::s_PrimaryMemorySize > memorySubpartitionSizes,
            "Wwise memory sizes of sub-categories add up to more than the primary memory pool size!")

        AZ::SettingsRegistryInterface::FixedValueString assetPlatform = AzFramework::OSPlatformToDefaultAssetPlatform(
            AZ_TRAIT_OS_PLATFORM_CODENAME);
        if (assetPlatform.empty())
        {
            if (const auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, assetPlatform,
                    AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, "assets");
            }
        }

        m_engineWwise = AZStd::make_unique<Audio::CAudioSystemImpl_wwise>(assetPlatform.c_str());
        if (m_engineWwise)
        {
            AZLOG_INFO("%s", "AudioEngineWwise created!");

            Audio::SystemRequest::Initialize initRequest;
            AZ::Interface<Audio::IAudioSystem>::Get()->PushRequestBlocking(AZStd::move(initRequest));

            success = true;
        }
        else
        {
            AZLOG_ERROR("%s", "Could not create AudioEngineWwise!");
        }

        return success;
    }

    void AudioEngineWwiseGemSystemComponent::Release()
    {
        m_engineWwise.reset();
    }

#if defined(AUDIO_ENGINE_WWISE_EDITOR)
    void AudioEngineWwiseGemSystemComponent::InitializeEditorImplPlugin()
    {
        m_editorImplPlugin.reset(new AudioControls::CAudioSystemEditor_wwise());
    }

    void AudioEngineWwiseGemSystemComponent::ReleaseEditorImplPlugin()
    {
        m_editorImplPlugin.reset();
    }

    AudioControls::IAudioSystemEditor* AudioEngineWwiseGemSystemComponent::GetEditorImplPlugin()
    {
        return m_editorImplPlugin.get();
    }
#endif // AUDIO_ENGINE_WWISE_EDITOR

} // namespace AudioEngineWwiseGem
