/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "EditorDebugDrawLineComponent.h"

namespace DebugDraw
{
    void EditorDebugDrawLineComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorDebugDrawLineComponent, EditorComponentBase>()
                ->Version(0)
                ->Field("Element", &EditorDebugDrawLineComponent::m_element)
                ->Field("Settings", &EditorDebugDrawLineComponent::m_settings)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorDebugDrawLineComponent>(
                    "DebugDraw Line", "Draws debug line on the screen from this entity's location to specified end entity's location.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Debugging")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/DebugDrawLine.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/DebugDrawLine.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(0, &EditorDebugDrawLineComponent::m_element, "Line element settings", "Settings for the line element.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugDrawLineComponent::OnPropertyUpdate)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorDebugDrawLineComponent::m_settings, "Visibility settings", "Common settings for DebugDraw components.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugDrawLineComponent::OnPropertyUpdate)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }
    }

    void EditorDebugDrawLineComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (m_settings.m_visibleInGame)
        {
            gameEntity->CreateComponent<DebugDrawLineComponent>(m_element);
        }
    }

    void EditorDebugDrawLineComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        DebugDrawLineComponent::GetProvidedServices(provided);
    }

    void EditorDebugDrawLineComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        DebugDrawLineComponent::GetIncompatibleServices(incompatible);
    }

    void EditorDebugDrawLineComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        DebugDrawLineComponent::GetRequiredServices(required);
    }

    void EditorDebugDrawLineComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        DebugDrawLineComponent::GetDependentServices(dependent);
    }

    void EditorDebugDrawLineComponent::Init()
    {
        m_element.m_owningEditorComponent = GetId();
    }

    void EditorDebugDrawLineComponent::Activate()
    {
        if (m_settings.m_visibleInEditor)
        {
            DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::RegisterDebugDrawComponent, this);
        }
    }

    void EditorDebugDrawLineComponent::Deactivate()
    {
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::UnregisterDebugDrawComponent, this);
    }

    void EditorDebugDrawLineComponent::OnPropertyUpdate()
    {
        // Property updated, we need to update the system component (which handles drawing the components) with our new info
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::UnregisterDebugDrawComponent, this);

        if (m_settings.m_visibleInEditor)
        {
            DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::RegisterDebugDrawComponent, this);
        }
    }
} // namespace DebugDraw
