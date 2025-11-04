/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorAudioTriggerComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    //=========================================================================
    void EditorAudioTriggerComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorAudioTriggerComponent, EditorComponentBase>()
                ->Version(2)
                ->Field("Play Trigger", &EditorAudioTriggerComponent::m_defaultPlayTrigger)
                ->Field("Stop Trigger", &EditorAudioTriggerComponent::m_defaultStopTrigger)
                ->Field("Obstruction Type", &EditorAudioTriggerComponent::m_obstructionType)
                ->Field("Plays Immediately", &EditorAudioTriggerComponent::m_playsImmediately)
                ;

            serializeContext->Enum<Audio::ObstructionType>()
                ->Value("Ignore", Audio::ObstructionType::Ignore)
                ->Value("SingleRay", Audio::ObstructionType::SingleRay)
                ->Value("MultiRay", Audio::ObstructionType::MultiRay)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Enum<Audio::ObstructionType>("Obstruction Type", "The types of ray-casts available for obstruction and occlusion")
                    ->Value("Ignore", Audio::ObstructionType::Ignore)
                    ->Value("SingleRay", Audio::ObstructionType::SingleRay)
                    ->Value("MultiRay", Audio::ObstructionType::MultiRay)
                    ;

                editContext->Class<EditorAudioTriggerComponent>("Audio Trigger", "The Audio Trigger component provides Audio Translation Layer (ATL) triggers for play/stop functionality and on-demand execution")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Audio")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/AudioTrigger.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/AudioTrigger.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/audio/trigger/")
                    ->DataElement("AudioControl", &EditorAudioTriggerComponent::m_defaultPlayTrigger, "Default 'play' Trigger", "The default ATL Trigger control used by 'Play'")
                    ->DataElement("AudioControl", &EditorAudioTriggerComponent::m_defaultStopTrigger, "Default 'stop' Trigger", "The default ATL Trigger control used by 'Stop'")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorAudioTriggerComponent::m_obstructionType, "Obstruction Type", "Ray-casts used in calculation of obstruction and occlusion")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAudioTriggerComponent::m_playsImmediately, "Plays immediately", "Play when this component is Activated")
                    ;
            }
        }
    }

    //=========================================================================
    EditorAudioTriggerComponent::EditorAudioTriggerComponent()
    {
        m_defaultPlayTrigger.m_propertyType = AzToolsFramework::AudioPropertyType::Trigger;
        m_defaultStopTrigger.m_propertyType = AzToolsFramework::AudioPropertyType::Trigger;
    }

    //=========================================================================
    void EditorAudioTriggerComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<AudioTriggerComponent>(m_defaultPlayTrigger.m_controlName, m_defaultStopTrigger.m_controlName, m_obstructionType, m_playsImmediately);
    }

} // namespace LmbrCentral
