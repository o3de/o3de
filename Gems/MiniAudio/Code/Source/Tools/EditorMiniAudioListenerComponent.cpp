/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorMiniAudioListenerComponent.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Utils/Utils.h>

namespace MiniAudio
{
    AZ::ComponentDescriptor* EditorMiniAudioListenerComponent_CreateDescriptor()
    {
        return EditorMiniAudioListenerComponent::CreateDescriptor();
    }

    void EditorMiniAudioListenerComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorMiniAudioListenerComponent, BaseClass>()->Version(2);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorMiniAudioListenerComponent>("MiniAudio Listener", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "MiniAudio")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                editContext->Class<MiniAudioListenerComponentController>("MiniAudioListenerComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MiniAudioListenerComponentController::m_config, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                editContext
                    ->Class<MiniAudioListenerComponentConfig>(
                        "MiniAudioListenerComponent Config", "[Configuration for MiniAudioListenerComponent]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioListenerComponentConfig::m_followEntity,
                        "Follow Entity",
                        "The listener will follow the position and orientation of the specified entity.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioListenerComponentConfig::m_listenerIndex,
                        "Listener Index",
                        "MiniAudio listener index to control.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider,
                        &MiniAudioListenerComponentConfig::m_globalVolume,
                        "Global Volume",
                        "Sets the global volume of the audio engine, as a percentage.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " %")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioListenerComponentConfig::m_innerAngleInDegrees,
                        "Inner Cone Angle",
                        "Sets the listener's inner cone angle in Degrees.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioListenerComponentConfig::m_outerAngleInDegrees,
                        "Outer Cone Angle",
                        "Sets the listener's outer cone angle in Degrees.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider,
                        &MiniAudioListenerComponentConfig::m_outerVolume,
                        "Outer Volume",
                        "Sets the volume of the listener outside of the outer cone, as a percentage.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " %");
            }
        }
    }

    EditorMiniAudioListenerComponent::EditorMiniAudioListenerComponent(const MiniAudioListenerComponentConfig& config)
        : BaseClass(config)
    {
    }

    void EditorMiniAudioListenerComponent::Activate()
    {
        BaseClass::Activate();
    }

    void EditorMiniAudioListenerComponent::Deactivate()
    {
        BaseClass::Deactivate();
    }

    AZ::u32 EditorMiniAudioListenerComponent::OnConfigurationChanged()
    {
        m_controller.OnConfigurationUpdated();
        return AZ::Edit::PropertyRefreshLevels::None;
    }
} // namespace MiniAudio
