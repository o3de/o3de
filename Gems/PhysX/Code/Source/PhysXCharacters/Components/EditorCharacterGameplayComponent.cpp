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

#include <PhysXCharacters/Components/EditorCharacterGameplayComponent.h>
#include <AzCore/Serialization/EditContext.h>

namespace PhysX
{
    void EditorCharacterGameplayComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PhysXCharacterGameplayService", 0xfacd7876));
    }

    void EditorCharacterGameplayComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("PhysXCharacterGameplayService", 0xfacd7876));
    }

    void EditorCharacterGameplayComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("PhysXCharacterControllerService", 0x428de4fa));
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
                    "PhysX Character Gameplay", "PhysX Character Gameplay")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/PhysXCharacter.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/PhysXCharacter.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCharacterGameplayComponent::m_gameplayConfig,
                        "Gameplay Configuration", "Gameplay Configuration")
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
