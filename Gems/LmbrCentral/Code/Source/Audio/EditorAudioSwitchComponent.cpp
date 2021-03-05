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

#include "LmbrCentral_precompiled.h"
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
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/AudioSwitch.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/AudioSwitch.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-audio-switch.html")
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
