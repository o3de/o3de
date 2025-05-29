/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <LmbrCentral/Audio/AudioProxyComponentBus.h>

#include <Integration/Editor/Components/EditorAnimAudioComponent.h>
#include <Integration/Components/AnimAudioComponent.h>

namespace EMotionFX
{
    namespace Integration
    {
        void EditorAnimAudioComponent::BuildGameEntity(AZ::Entity* gameEntity)
        {
            auto animAudioComponent = aznew AnimAudioComponent;
            gameEntity->AddComponent(animAudioComponent);

            for (const auto& triggerEvent : m_editorTriggerEvents)
            {
                animAudioComponent->AddTriggerEvent(triggerEvent.m_event, triggerEvent.m_trigger.m_controlName, triggerEvent.m_joint);
            }
        }

        void EditorAnimAudioComponent::Reflect(AZ::ReflectContext* context)
        {
            EditorAudioTriggerEvent::Reflect(context);
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorAnimAudioComponent, AZ::Component>()
                    ->Version(0)
                    ->Field("Trigger Map", &EditorAnimAudioComponent::m_editorTriggerEvents);

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorAnimAudioComponent>("Audio Animation", "Adds ability to execute audio triggers when animation events occur.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Audio")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/AudioAnimation.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAnimAudioComponent::m_editorTriggerEvents, "Trigger Map", "Maps the animation events to executable audio triggers.");
                }
            }
        }
    } // namespace Integration
} // namespace EMotionFX
