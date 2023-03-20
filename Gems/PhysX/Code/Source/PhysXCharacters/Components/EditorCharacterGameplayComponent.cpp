/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysXCharacters/Components/EditorCharacterGameplayComponent.h>
#include <AzCore/Serialization/EditContext.h>

namespace PhysX
{
    void EditorCharacterGameplayComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsCharacterGameplayService"));
    }

    void EditorCharacterGameplayComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsCharacterGameplayService"));
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorCharacterGameplayComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("PhysicsCharacterControllerService"));
    }

    void EditorCharacterGameplayComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void EditorCharacterGameplayComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorCharacterGameplayComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("GameplayConfig", &EditorCharacterGameplayComponent::m_gameplayConfig)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorCharacterGameplayComponent>(
                    "PhysX Character Gameplay", "An example implementation of character physics behavior such as gravity.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/PhysXCharacter.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/PhysXCharacter.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/character-gameplay/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCharacterGameplayComponent::m_gameplayConfig,
                        "Gameplay Configuration", "Gameplay Configuration.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void EditorCharacterGameplayComponent::Activate()
    {
    }

    void EditorCharacterGameplayComponent::Deactivate()
    {
    }

    // EditorComponentBase
    void EditorCharacterGameplayComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<CharacterGameplayComponent>(m_gameplayConfig);
    }
} // namespace PhysX
