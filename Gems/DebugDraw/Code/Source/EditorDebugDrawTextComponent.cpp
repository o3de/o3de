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

#include "DebugDraw_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "EditorDebugDrawTextComponent.h"

namespace DebugDraw
{
    void EditorDebugDrawTextComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorDebugDrawTextComponent, EditorComponentBase>()
                ->Version(0)
                ->Field("Element", &EditorDebugDrawTextComponent::m_element)
                ->Field("Settings", &EditorDebugDrawTextComponent::m_settings)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorDebugDrawTextComponent>(
                    "DebugDraw Text", "Draws debug text on the screen at this entity's location.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Debugging")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/DebugDrawText.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->DataElement(0, &EditorDebugDrawTextComponent::m_element, "Text element settings", "Settings for the text element.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugDrawTextComponent::OnPropertyUpdate)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorDebugDrawTextComponent::m_settings, "Visibility settings", "Common settings for DebugDraw components.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDebugDrawTextComponent::OnPropertyUpdate)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }
    }

    void EditorDebugDrawTextComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (m_settings.m_visibleInGame)
        {
            gameEntity->CreateComponent<DebugDrawTextComponent>(m_element);
        }
    }

    void EditorDebugDrawTextComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        DebugDrawTextComponent::GetProvidedServices(provided);
    }

    void EditorDebugDrawTextComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        DebugDrawTextComponent::GetIncompatibleServices(incompatible);
    }

    void EditorDebugDrawTextComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        DebugDrawTextComponent::GetRequiredServices(required);
    }

    void EditorDebugDrawTextComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        DebugDrawTextComponent::GetDependentServices(dependent);
    }

    void EditorDebugDrawTextComponent::Init()
    {
        m_element.m_owningEditorComponent = GetId();
    }

    void EditorDebugDrawTextComponent::Activate()
    {
        if (m_settings.m_visibleInEditor)
        {
            DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::RegisterDebugDrawComponent, this);
        }
    }

    void EditorDebugDrawTextComponent::Deactivate()
    {
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::UnregisterDebugDrawComponent, this);
    }

    void EditorDebugDrawTextComponent::OnPropertyUpdate()
    {
        // Property updated, we need to update the system component (which handles drawing the components) with our new info
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::UnregisterDebugDrawComponent, this);

        if (m_settings.m_visibleInEditor)
        {
            DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::RegisterDebugDrawComponent, this);
        }
    }
} // namespace DebugDraw
