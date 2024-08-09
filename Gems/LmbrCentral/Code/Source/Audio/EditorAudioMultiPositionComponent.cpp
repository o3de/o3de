/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorAudioMultiPositionComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    //=========================================================================
    void EditorAudioMultiPositionComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorAudioMultiPositionComponent, EditorComponentBase>()
                ->Version(0)
                ->Field("Entity Refs", &EditorAudioMultiPositionComponent::m_entityRefs)
                ->Field("Behavior Type", &EditorAudioMultiPositionComponent::m_behaviorType)
                ;

            serializeContext->Enum<Audio::MultiPositionBehaviorType>()
                ->Value("Separate", Audio::MultiPositionBehaviorType::Separate)
                ->Value("Blended", Audio::MultiPositionBehaviorType::Blended)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Enum<Audio::MultiPositionBehaviorType>("Behavior Type", "How multiple position audio behaves")
                    ->Value("Separate", Audio::MultiPositionBehaviorType::Separate)
                    ->Value("Blended", Audio::MultiPositionBehaviorType::Blended)
                    ;

                editContext->Class<EditorAudioMultiPositionComponent>("Multi-Position Audio", "The Multi-Position Audio component provides the ability to broadcast sounds through multiple positions")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Audio")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/AudioMultiPosition.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/AudioMultiPosition.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/audio/multi-position/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAudioMultiPositionComponent::m_entityRefs, "Entity References", "The entities from which positions will be obtained for multi-position audio")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorAudioMultiPositionComponent::m_behaviorType, "Behavior Type", "Determines how multi-postion sounds are treated, Separate or Blended")
                    ;
            }
        }
    }

    //=========================================================================
    void EditorAudioMultiPositionComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<AudioMultiPositionComponent>(m_entityRefs, m_behaviorType);
    }

} // namespace LmbrCentral
