/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorComponentBase.h"
#include "TransformComponent.h"
#include <AzCore/Math/Vector2.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>

DECLARE_EBUS_INSTANTIATION_WITH_TRAITS(AzToolsFramework::Components::EditorComponentDescriptor, AZ::ComponentDescriptorBusTraits);

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorComponentBase::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<EditorComponentBase, AZ::Component>()
                    ->Version(1)
                ;
            }
        }

        EditorComponentBase::EditorComponentBase()
        {
            m_transform = nullptr;
        }

        void EditorComponentBase::Init()
        {
        }

        void EditorComponentBase::Activate()
        {
            m_transform = GetEntity()->FindComponent<TransformComponent>();
        }

        void EditorComponentBase::Deactivate()
        {
            m_transform = nullptr;
        }

        void EditorComponentBase::OnAfterEntitySet()
        {
            if (!m_entity)
            {
                AZ_Assert(false, "OnAfterEntitySet - Entity should not be nullptr.");
                return;
            }

            // Sets up only if the component alias is empty to avoid setting the alias twice.
            // For a component added as a pending component, there are two places trying to call this function to set up the alias:
            // - EditorEntityActionComponent::AddExistingComponentsToEntityById()
            // - Entity::AddComponent()
            if (m_alias.empty())
            {
                AZStd::string newIdentifier = GenerateComponentSerializedIdentifier();
                SetSerializedIdentifier(AZStd::move(newIdentifier));
            }
        }

        AZStd::string EditorComponentBase::GenerateComponentSerializedIdentifier()
        {
            if (!m_entity)
            {
                AZ_Assert(false, "GenerateComponentSerializedIdentifier - Entity should not be nullptr.");
                return {};
            }

            AZStd::unordered_set<AZStd::string> existingSerializedIdentifiers;

            // Defines the function to add existing serialized identifiers from a given component list.
            auto captureExistingIdentifiersFunc =
                [&existingSerializedIdentifiers, this](AZStd::span<AZ::Component* const> componentsToCapture)
                {
                    for (const AZ::Component* component : componentsToCapture)
                    {
                        AZ_Assert(
                            component != this,
                            "GenerateComponentSerializedIdentifier - "
                            "Attempting to generate an identifier for a component that is already owned by the entity. ");

                        AZStd::string existingIdentifier = component->GetSerializedIdentifier();

                        if (!existingIdentifier.empty() && RTTI_GetType() == component->RTTI_GetType())
                        {
                            existingSerializedIdentifiers.emplace(AZStd::move(existingIdentifier));
                        }
                    }
                };

            // Checks all components of the entity, including pending components and disabled components.
            captureExistingIdentifiersFunc(m_entity->GetComponents());

            AZ::Entity::ComponentArrayType pendingComponents;
            AzToolsFramework::EditorPendingCompositionRequestBus::Event(
                m_entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);

            captureExistingIdentifiersFunc(pendingComponents);

            AZ::Entity::ComponentArrayType disabledComponents;
            AzToolsFramework::EditorDisabledCompositionRequestBus::Event(
                m_entity->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);

            captureExistingIdentifiersFunc(disabledComponents);

            // Generates a component typename string and updates its suffix count to avoid duplication.
            const AZStd::string componentTypeName = GetNameFromComponentClassData(this);
            AZStd::string serializedIdentifier = componentTypeName;
            AZ::u64 suffixOfNewComponent = 1;

            while (existingSerializedIdentifiers.find(serializedIdentifier) != existingSerializedIdentifiers.end())
            {
                serializedIdentifier = AZStd::string::format("%s_%llu", componentTypeName.c_str(), ++suffixOfNewComponent);
            }

            return serializedIdentifier;
        }

        void EditorComponentBase::SetDirty()
        {
            // Don't mark things for dirty that are not active.
            // Entities can be inactive for a number of reasons, for example, in a prefab file
            // being constructed on another thread, and while these might still invoke SetDirty
            // in response to properties changing, only actualized entities should interact with
            // the undo/redo system or the prefab change tracker for overrides, which is what
            // marking things dirty is for.
            if (AZ::Entity* entity = GetEntity();entity)
            {
                if (entity->GetState() == AZ::Entity::State::Active)
                {
                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entity->GetId());
                }

            }
            else
            {
                AZ_Warning("Editor", false, "EditorComponentBase::SetDirty() failed. Couldn't add dirty entity because the pointer to the entity is NULL. Make sure the entity is Init()'d properly.");
            }
        }

        AZ::TransformInterface* EditorComponentBase::GetTransform() const
        {
            AZ_Assert(m_transform, "Attempt to GetTransformComponent when the entity is inactive or does not have one.");
            return m_transform;
        }

        AZ::Transform EditorComponentBase::GetWorldTM() const
        {
            if (m_transform)
            {
                return m_transform->GetWorldTM();
            }
            else
            {
                return AZ::Transform::Identity();
            }
        }

        AZ::Transform EditorComponentBase::GetLocalTM() const
        {
            if (m_transform)
            {
                return m_transform->GetLocalTM();
            }
            else
            {
                return AZ::Transform::Identity();
            }
        }

        bool EditorComponentBase::IsSelected() const
        {
            return AzToolsFramework::IsSelected(GetEntityId());
        }

        void EditorComponentBase::InvalidatePropertyDisplay(PropertyModificationRefreshLevel refreshFlags)
        {
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplayForComponent,
                AZ::EntityComponentIdPair(GetEntityId(), GetId()), 
                static_cast<PropertyModificationRefreshLevel>(refreshFlags));
        }

        void EditorComponentBase::SetSerializedIdentifier(AZStd::string alias)
        {
            m_alias = alias;
        }

        AZStd::string EditorComponentBase::GetSerializedIdentifier() const
        {
            return m_alias;
        }
    }
} // namespace AzToolsFramework
