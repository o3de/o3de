/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "EditorDebugDrawRayComponent.h"

namespace DebugDraw
{
    void EditorDebugDrawRayComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorDebugDrawRayComponent, EditorComponentBase>()
                ->Version(0)
                ->Field("Element", &EditorDebugDrawRayComponent::m_element)
                ->Field("Settings", &EditorDebugDrawRayComponent::m_settings)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorDebugDrawRayComponent>(
                    "DebugDraw Ray", "Draws debug ray on the screen from this entity's location to specified end entity's location.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Debugging")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/DebugDrawRay.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/DebugDrawRay.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(0, &EditorDebugDrawRayComponent::m_element, "Ray element settings", "Settings for the ray element.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugDrawRayComponent::OnPropertyUpdate)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorDebugDrawRayComponent::m_settings, "Visibility settings", "Common settings for DebugDraw components.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugDrawRayComponent::OnPropertyUpdate)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }
    }

    void EditorDebugDrawRayComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (m_settings.m_visibleInGame)
        {
            gameEntity->CreateComponent<DebugDrawRayComponent>(m_element);
        }
    }

    void EditorDebugDrawRayComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        DebugDrawRayComponent::GetProvidedServices(provided);
    }

    void EditorDebugDrawRayComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        DebugDrawRayComponent::GetIncompatibleServices(incompatible);
    }

    void EditorDebugDrawRayComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        DebugDrawRayComponent::GetRequiredServices(required);
    }

    void EditorDebugDrawRayComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        DebugDrawRayComponent::GetDependentServices(dependent);
    }

    void EditorDebugDrawRayComponent::Init()
    {
        m_element.m_owningEditorComponent = GetId();
    }

    void EditorDebugDrawRayComponent::Activate()
    {
        if (m_settings.m_visibleInEditor)
        {
            DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::RegisterDebugDrawComponent, this);
        }
    }

    void EditorDebugDrawRayComponent::Deactivate()
    {
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::UnregisterDebugDrawComponent, this);
    }

    void EditorDebugDrawRayComponent::OnPropertyUpdate()
    {
        // Property updated, we need to update the system component (which handles drawing the components) with our new info
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::UnregisterDebugDrawComponent, this);

        if (m_settings.m_visibleInEditor)
        {
            DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::RegisterDebugDrawComponent, this);
        }
    }
} // namespace DebugDraw
