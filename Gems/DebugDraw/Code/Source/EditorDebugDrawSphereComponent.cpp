/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "EditorDebugDrawSphereComponent.h"

namespace DebugDraw
{
    void EditorDebugDrawSphereComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorDebugDrawSphereComponent, EditorComponentBase>()
                ->Version(0)
                ->Field("Element", &EditorDebugDrawSphereComponent::m_element)
                ->Field("Settings", &EditorDebugDrawSphereComponent::m_settings)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorDebugDrawSphereComponent>(
                    "DebugDraw Sphere", "Draws debug ray on the screen from this entity's location to specified end entity's location.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Debugging")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/DebugDrawSphere.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/DebugDrawSphere.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(0, &EditorDebugDrawSphereComponent::m_element, "Sphere element settings", "Settings for the sphere element.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugDrawSphereComponent::OnPropertyUpdate)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorDebugDrawSphereComponent::m_settings, "Visibility settings", "Common settings for DebugDraw components.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugDrawSphereComponent::OnPropertyUpdate)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }
    }

    void EditorDebugDrawSphereComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (m_settings.m_visibleInGame)
        {
            gameEntity->CreateComponent<DebugDrawSphereComponent>(m_element);
        }
    }

    void EditorDebugDrawSphereComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        DebugDrawSphereComponent::GetProvidedServices(provided);
    }

    void EditorDebugDrawSphereComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        DebugDrawSphereComponent::GetIncompatibleServices(incompatible);
    }

    void EditorDebugDrawSphereComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        DebugDrawSphereComponent::GetRequiredServices(required);
    }

    void EditorDebugDrawSphereComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        DebugDrawSphereComponent::GetDependentServices(dependent);
    }

    void EditorDebugDrawSphereComponent::Init()
    {
        m_element.m_owningEditorComponent = GetId();
    }

    void EditorDebugDrawSphereComponent::Activate()
    {
        if (m_settings.m_visibleInEditor)
        {
            DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::RegisterDebugDrawComponent, this);
        }
    }

    void EditorDebugDrawSphereComponent::Deactivate()
    {
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::UnregisterDebugDrawComponent, this);
    }

    void EditorDebugDrawSphereComponent::OnPropertyUpdate()
    {
        // Property updated, we need to update the system component (which handles drawing the components) with our new info
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::UnregisterDebugDrawComponent, this);

        if (m_settings.m_visibleInEditor)
        {
            DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::RegisterDebugDrawComponent, this);
        }
    }
} // namespace DebugDraw
