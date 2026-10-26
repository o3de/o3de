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
#include <AzFramework/Translation/TranslationDef.h>

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
            serializeContext->Class<EditorMiniAudioPlaybackComponent, BaseClass>()->Version(4);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorMiniAudioPlaybackComponent>(
                    QT_TRANSLATE_NOOP("MiniAudio", "MiniAudio Playback"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "MiniAudio")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->UIElement(AZ::Edit::UIHandlers::Button,
                        QT_TRANSLATE_NOOP("MiniAudio", "Play Sound"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Plays the assigned sound"))
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText,
                        QT_TRANSLATE_NOOP("MiniAudio", "Play Sound"))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMiniAudioPlaybackComponent::PlaySoundInEditor)

                    ->UIElement(AZ::Edit::UIHandlers::Button,
                        QT_TRANSLATE_NOOP("MiniAudio", "Stop Sound"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Stops playing the sound"))
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText,
                        QT_TRANSLATE_NOOP("MiniAudio", "Stop Sound"))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMiniAudioPlaybackComponent::StopSoundInEditor)

                    ->UIElement(AZ::Edit::UIHandlers::Button,
                        QT_TRANSLATE_NOOP("MiniAudio", "Pause Sound"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Pause playing the sound"))
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText,
                        QT_TRANSLATE_NOOP("MiniAudio", "Pause Sound"))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMiniAudioPlaybackComponent::PauseSoundInEditor);

                editContext->Class<MiniAudioPlaybackComponentController>(
                    QT_TRANSLATE_NOOP("MiniAudio", "MiniAudioPlaybackComponentController"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MiniAudioPlaybackComponentController::m_config,
                        QT_TRANSLATE_NOOP("MiniAudio", "Configuration"), "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                editContext
                    ->Class<MiniAudioPlaybackComponentConfig>(
                        QT_TRANSLATE_NOOP("MiniAudio", "MiniAudioPlaybackComponent Config"),
                        QT_TRANSLATE_NOOP("MiniAudio", "[Configuration for MiniAudioPlaybackComponent]"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &MiniAudioPlaybackComponentConfig::m_sound,
                        QT_TRANSLATE_NOOP("MiniAudio", "Sound Asset"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Sound asset to play"))

                    ->ClassElement(AZ::Edit::ClassElements::Group,
                        QT_TRANSLATE_NOOP("MiniAudio", "Configuration"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioPlaybackComponentConfig::m_autoplayOnActivate,
                        QT_TRANSLATE_NOOP("MiniAudio", "Autoplay"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Plays the sound on activation of the component."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MiniAudioPlaybackComponentConfig::m_loop,
                        QT_TRANSLATE_NOOP("MiniAudio", "Loop"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Loops the sound."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider,
                        &MiniAudioPlaybackComponentConfig::m_volume,
                        QT_TRANSLATE_NOOP("MiniAudio", "Volume"),
                        QT_TRANSLATE_NOOP("MiniAudio", "The volume of the sound when played, as a percentage."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("MiniAudio", " %"))

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioPlaybackComponentConfig::m_autoFollowEntity,
                        QT_TRANSLATE_NOOP("MiniAudio", "Auto-follow"),
                        QT_TRANSLATE_NOOP("MiniAudio", "The sound's position is updated to match the entity's position."))

                    ->ClassElement(AZ::Edit::ClassElements::Group,
                        QT_TRANSLATE_NOOP("MiniAudio", "Spatialization"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioPlaybackComponentConfig::m_enableSpatialization,
                        QT_TRANSLATE_NOOP("MiniAudio", "Spatialization"),
                        QT_TRANSLATE_NOOP("MiniAudio", "If true the sound will have 3D position in the world and will have effects applied to it based on the distance "
                        "from a sound listener."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioPlaybackComponentConfig::m_directionalAttenuationFactor,
                        QT_TRANSLATE_NOOP("MiniAudio", "Directional Attenuation Factor"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Sets the directional attenuation based on the listener's direction."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &MiniAudioPlaybackComponentConfig::m_attenuationModel,
                        QT_TRANSLATE_NOOP("MiniAudio", "Attenuation"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Attenuation model."))
                    ->EnumAttribute(AttenuationModel::Inverse,
                        QT_TRANSLATE_NOOP("MiniAudio", "Inverse"))
                    ->EnumAttribute(AttenuationModel::Exponential,
                        QT_TRANSLATE_NOOP("MiniAudio", "Exponential"))
                    ->EnumAttribute(AttenuationModel::Linear,
                        QT_TRANSLATE_NOOP("MiniAudio", "Linear"))

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioPlaybackComponentConfig::m_fixedDirection,
                        QT_TRANSLATE_NOOP("MiniAudio", "Fixed Direction"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Determines whether the direction of the sound source is fixed to what is entered in the editor or if the forward "
                        "direction of the entity is used."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioPlaybackComponentConfig::m_direction,
                        QT_TRANSLATE_NOOP("MiniAudio", "Direction"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Sets the direction that the sound source's inner and out cones point towards."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioPlaybackComponentConfig::m_minimumDistance,
                        QT_TRANSLATE_NOOP("MiniAudio", "Min Distance"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Minimum distance for attenuation."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioPlaybackComponentConfig::m_maximumDistance,
                        QT_TRANSLATE_NOOP("MiniAudio", "Max Distance"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Maximum distance for attenuation."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioPlaybackComponentConfig::m_innerAngleInDegrees,
                        QT_TRANSLATE_NOOP("MiniAudio", "Inner Cone Angle"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Sets the sound source's inner cone angle in Degrees."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("MiniAudio", " degrees"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioPlaybackComponentConfig::m_outerAngleInDegrees,
                        QT_TRANSLATE_NOOP("MiniAudio", "Outer Cone Angle"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Sets the sound source's outer cone angle in Degrees."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("MiniAudio", " degrees"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider,
                        &MiniAudioPlaybackComponentConfig::m_outerVolume,
                        QT_TRANSLATE_NOOP("MiniAudio", "Outer Volume"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Sets the volume of the sound source outside of the outer cone, as a percentage."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("MiniAudio", " %"))

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
        m_controller.Play();
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::Crc32 EditorMiniAudioPlaybackComponent::StopSoundInEditor()
    {
        m_controller.Stop();
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::Crc32 EditorMiniAudioPlaybackComponent::PauseSoundInEditor()
    {
        m_controller.Pause();
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::Crc32 EditorMiniAudioPlaybackComponent::OnVolumeChanged()
    {
        m_controller.SetVolumePercentage(m_controller.GetConfiguration().m_volume / 100.f);
        return AZ::Edit::PropertyRefreshLevels::None;
    }
} // namespace MiniAudio
