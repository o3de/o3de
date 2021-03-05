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
#include "EditorAudioRtpcComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>


namespace LmbrCentral
{
    //=========================================================================
    void EditorAudioRtpcComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorAudioRtpcComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Rtpc Name", &EditorAudioRtpcComponent::m_defaultRtpc)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorAudioRtpcComponent>("Audio Rtpc", "The Audio Rtpc component provides basic Real-Time Parameter Control (RTPC) functionality allowing you to tweak sounds in real time")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Audio")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/AudioRtpc.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/AudioRtpc.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-audio-rtpc.html")
                    ->DataElement("AudioControl", &EditorAudioRtpcComponent::m_defaultRtpc, "Default Rtpc", "The default ATL Rtpc control to use")
                    ;
            }
        }
    }

    //=========================================================================
    EditorAudioRtpcComponent::EditorAudioRtpcComponent()
    {
        m_defaultRtpc.m_propertyType = AzToolsFramework::AudioPropertyType::Rtpc;
    }

    //=========================================================================
    void EditorAudioRtpcComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<AudioRtpcComponent>(m_defaultRtpc.m_controlName);
    }

} // namespace LmbrCentral
