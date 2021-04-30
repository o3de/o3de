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

#include "LmbrCentral_precompiled.h"
#include "EditorCylinderShapeComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include "CylinderShapeComponent.h"
#include "EditorShapeComponentConverters.h"
#include "ShapeDisplay.h"

namespace LmbrCentral
{
    void EditorCylinderShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate: EditorCylinderColliderComponent -> EditorCylinderShapeComponent
            serializeContext->ClassDeprecate(
                "EditorCylinderColliderComponent",
                "{1C10CEE7-0A5C-4D4A-BBD9-5C3B6C6FE844}",
                &ClassConverters::DeprecateEditorCylinderColliderComponent)
                ;

            serializeContext->Class<EditorCylinderShapeComponent, EditorBaseShapeComponent>()
                ->Version(3, &ClassConverters::UpgradeEditorCylinderShapeComponent)
                ->Field("CylinderShape", &EditorCylinderShapeComponent::m_cylinderShape)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorCylinderShapeComponent>(
                    "Cylinder Shape", "The Cylinder Shape component creates a cylinder around the associated entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Cylinder_Shape.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Cylinder_Shape.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-shapes.html")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCylinderShapeComponent::m_cylinderShape, "Cylinder Shape", "Cylinder Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCylinderShapeComponent::ConfigurationChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
            }
        }
    }

    void EditorCylinderShapeComponent::Init()
    {
        EditorBaseShapeComponent::Init();

        SetShapeComponentConfig(&m_cylinderShape.ModifyConfiguration());
    }

    void EditorCylinderShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();
        m_cylinderShape.Activate(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorCylinderShapeComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_cylinderShape.Deactivate();
        EditorBaseShapeComponent::Deactivate();
    }

    void EditorCylinderShapeComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DisplayShape(
            debugDisplay, [this]() { return CanDraw(); },
            [this](AzFramework::DebugDisplayRequests& debugDisplay)
            {
                DrawCylinderShape(
                    { m_shapeColor, m_shapeWireColor, m_displayFilled },
                    m_cylinderShape.GetCylinderConfiguration(), debugDisplay);
            },
            m_cylinderShape.GetCurrentTransform());
    }

    void EditorCylinderShapeComponent::ConfigurationChanged()
    {
        m_cylinderShape.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            GetEntityId(), &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void EditorCylinderShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto component = gameEntity->CreateComponent<CylinderShapeComponent>())
        {
            component->SetConfiguration(m_cylinderShape.GetCylinderConfiguration());
        }

        if (m_visibleInGameView)
        {
            if (auto component = gameEntity->CreateComponent<CylinderShapeDebugDisplayComponent>())
            {
                component->SetConfiguration(m_cylinderShape.GetCylinderConfiguration());
            }
        }
    }
} // namespace LmbrCentral
