/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "EditorDebugDrawObbComponent.h"

namespace DebugDraw
{
    void EditorDebugDrawObbComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorDebugDrawObbComponent, EditorComponentBase>()
                ->Version(0)
                ->Field("Element", &EditorDebugDrawObbComponent::m_element)
                ->Field("Settings", &EditorDebugDrawObbComponent::m_settings)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorDebugDrawObbComponent>(
                    "DebugDraw Obb", "Draws debug obb on the screen from this entity's location to specified end entity's location.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Debugging")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/DebugDrawObb.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/DebugDrawObb.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(0, &EditorDebugDrawObbComponent::m_element, "Obb element settings", "Settings for the obb element.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugDrawObbComponent::OnPropertyUpdate)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorDebugDrawObbComponent::m_settings, "Visibility settings", "Common settings for DebugDraw components.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugDrawObbComponent::OnPropertyUpdate)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }
    }

    void EditorDebugDrawObbComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (m_settings.m_visibleInGame)
        {
            gameEntity->CreateComponent<DebugDrawObbComponent>(m_element);
        }
    }

    void EditorDebugDrawObbComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        DebugDrawObbComponent::GetProvidedServices(provided);
    }

    void EditorDebugDrawObbComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        DebugDrawObbComponent::GetIncompatibleServices(incompatible);
    }

    void EditorDebugDrawObbComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        DebugDrawObbComponent::GetRequiredServices(required);
    }

    void EditorDebugDrawObbComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        DebugDrawObbComponent::GetDependentServices(dependent);
    }

    void EditorDebugDrawObbComponent::Init()
    {
        m_element.m_owningEditorComponent = GetId();
    }

    void EditorDebugDrawObbComponent::Activate()
    {
        if (m_settings.m_visibleInEditor)
        {
            DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::RegisterDebugDrawComponent, this);
        }
    }

    void EditorDebugDrawObbComponent::Deactivate()
    {
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::UnregisterDebugDrawComponent, this);
    }

    void EditorDebugDrawObbComponent::OnPropertyUpdate()
    {
        // Property updated, we need to update the system component (which handles drawing the components) with our new info
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::UnregisterDebugDrawComponent, this);

        if (m_settings.m_visibleInEditor)
        {
            DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::RegisterDebugDrawComponent, this);
        }
    }
} // namespace DebugDraw
