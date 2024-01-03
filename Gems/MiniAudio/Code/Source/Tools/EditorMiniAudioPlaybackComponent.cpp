/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorMiniAudioPlaybackComponent.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Utils/Utils.h>

namespace MiniAudio
{
    AZ::ComponentDescriptor* EditorMiniAudioPlaybackComponent_CreateDescriptor()
    {
        return EditorMiniAudioPlaybackComponent::CreateDescriptor();
    }

    void EditorMiniAudioPlaybackComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorMiniAudioPlaybackComponent, BaseClass>()
                ->Version(4);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorMiniAudioPlaybackComponent>("MiniAudio Playback",
                    "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "MiniAudio")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->UIElement(AZ::Edit::UIHandlers::Button, "Play Sound", "Plays the assigned sound")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Play Sound")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMiniAudioPlaybackComponent::PlaySoundInEditor)

                    ->UIElement(AZ::Edit::UIHandlers::Button, "Stop Sound", "Stops playing the sound")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Stop Sound")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMiniAudioPlaybackComponent::StopSoundInEditor)
                    ;

                editContext->Class<MiniAudioPlaybackComponentController>(
                    "MiniAudioPlaybackComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MiniAudioPlaybackComponentController::m_config, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                editContext->Class<MiniAudioPlaybackComponentConfig>("MiniAudioPlaybackComponent Config",
                    "[Configuration for MiniAudioPlaybackComponent]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &MiniAudioPlaybackComponentConfig::m_sound, "Sound Asset", "Sound asset to play")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Configuration")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &MiniAudioPlaybackComponentConfig::m_autoplayOnActivate, "Autoplay", "Plays the sound on activation of the component.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MiniAudioPlaybackComponentConfig::m_loop, "Loop", "Loops the sound.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MiniAudioPlaybackComponentConfig::m_volume, "Volume", "The volume of the sound when played.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 10.f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &MiniAudioPlaybackComponentConfig::m_autoFollowEntity, "Auto-follow",
                        "The sound's position is updated to match the entity's position.")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Spatialization")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MiniAudioPlaybackComponentConfig::m_enableSpatialization, "Spatialization", "If true the sound will have 3D position in the world and will have effects applied to it based on the distance from a sound listener.")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MiniAudioPlaybackComponentConfig::m_attenuationModel, "Attenuation", "Attenuation model.")
                    ->EnumAttribute(AttenuationModel::Inverse, "Inverse")
                    ->EnumAttribute(AttenuationModel::Exponential, "Exponential")
                    ->EnumAttribute(AttenuationModel::Linear, "Linear")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &MiniAudioPlaybackComponentConfig::m_minimumDistance, "Min Distance", "Minimum distance for attenuation.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MiniAudioPlaybackComponentConfig::m_maximumDistance, "Max Distance", "Maximum distance for attenuation.")

                    ;
            }
        }
    }

    EditorMiniAudioPlaybackComponent::EditorMiniAudioPlaybackComponent(const MiniAudioPlaybackComponentConfig& config)
        : BaseClass(config)
    {
    }

    void EditorMiniAudioPlaybackComponent::Activate()
    {
        BaseClass::Activate();
    }

    void EditorMiniAudioPlaybackComponent::Deactivate()
    {
        BaseClass::Deactivate();
    }

    AZ::u32 EditorMiniAudioPlaybackComponent::OnConfigurationChanged()
    {
        m_controller.OnConfigurationUpdated();
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::Crc32 EditorMiniAudioPlaybackComponent::PlaySoundInEditor()
    {
        m_controller.Stop();
        m_controller.Play();
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::Crc32 EditorMiniAudioPlaybackComponent::StopSoundInEditor()
    {
        m_controller.Stop();
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::Crc32 EditorMiniAudioPlaybackComponent::OnVolumeChanged()
    {
        m_controller.SetVolume(m_controller.GetConfiguration().m_volume);
        return AZ::Edit::PropertyRefreshLevels::None;
    }
} // namespace MiniAudio
