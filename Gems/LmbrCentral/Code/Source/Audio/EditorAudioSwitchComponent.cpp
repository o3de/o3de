/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorAudioSwitchComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    //=========================================================================
    void EditorAudioSwitchComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorAudioSwitchComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Switch name", &EditorAudioSwitchComponent::m_defaultSwitch)
                ->Field("State name", &EditorAudioSwitchComponent::m_defaultState)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorAudioSwitchComponent>("Audio Switch", "The Audio Switch component provides basic Audio Translation Layer (ATL) switch functionality to specify the state of an entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Audio")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/AudioSwitch.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/AudioSwitch.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/audio/switch/")
                    ->DataElement("AudioControl", &EditorAudioSwitchComponent::m_defaultSwitch, "Default Switch", "The default ATL Switch to use when Activated")
                    ->DataElement("AudioControl", &EditorAudioSwitchComponent::m_defaultState, "Default State", "The default ATL State to set on the default Switch when Activated")
                    ;
            }
        }
    }

    //=========================================================================
    EditorAudioSwitchComponent::EditorAudioSwitchComponent()
    {
        m_defaultSwitch.m_propertyType = AzToolsFramework::AudioPropertyType::Switch;
        m_defaultState.m_propertyType = AzToolsFramework::AudioPropertyType::SwitchState;
    }

    //=========================================================================
    void EditorAudioSwitchComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<AudioSwitchComponent>(m_defaultSwitch.m_controlName, m_defaultState.m_controlName);
    }

} // namespace LmbrCentral
