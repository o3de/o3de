/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/AudioRtpc.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/AudioRtpc.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/audio/rtpc/")
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
