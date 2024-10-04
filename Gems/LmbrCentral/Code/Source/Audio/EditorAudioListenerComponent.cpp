/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorAudioListenerComponent.h"
#include "AudioListenerComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    //=========================================================================
    void EditorAudioListenerComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorAudioListenerComponent, AZ::Component>()
                ->Version(2)
                ->Field("Rotation Entity", &EditorAudioListenerComponent::m_rotationEntity)
                ->Field("Position Entity", &EditorAudioListenerComponent::m_positionEntity)
                ->Field("Fixed offset", &EditorAudioListenerComponent::m_fixedOffset)
                ->Field("DefaultListenerState", &EditorAudioListenerComponent::m_defaultListenerState)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorAudioListenerComponent>("Audio Listener", "The Audio Listener component allows a virtual microphone to be placed in the environment")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Audio")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/AudioListener.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/AudioListener.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/audio/listener/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAudioListenerComponent::m_rotationEntity,
                        "Rotation Entity", "The Entity whose rotation the audio listener will adopt.  If none set, will assume 'this' Entity")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAudioListenerComponent::m_positionEntity,
                        "Position Entity", "The Entity whose position the audio listener will adopt.  If none set, will assume 'this' Entity")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAudioListenerComponent::m_fixedOffset,
                        "Fixed offset", "A fixed world-space offset to add to the listener position.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAudioListenerComponent::m_defaultListenerState,
                        "Listener Enabled", "Controls the initial state of this AudioListener on Component Activation.")
                    ;
            }
        }
    }

    //=========================================================================
    void EditorAudioListenerComponent::Activate()
    {
    }

    //=========================================================================
    void EditorAudioListenerComponent::Deactivate()
    {
    }

    //=========================================================================
    void EditorAudioListenerComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto audioListenerComponent = gameEntity->CreateComponent<AudioListenerComponent>())
        {
            audioListenerComponent->m_defaultListenerState = m_defaultListenerState;
            audioListenerComponent->m_rotationEntity = m_rotationEntity;
            audioListenerComponent->m_positionEntity = m_positionEntity;
            audioListenerComponent->m_fixedOffset = m_fixedOffset;
        }
    }

} // namespace LmbrCentral
