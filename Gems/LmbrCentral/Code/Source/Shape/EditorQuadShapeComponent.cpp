/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "QuadShapeComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include "EditorQuadShapeComponent.h"
#include "EditorShapeComponentConverters.h"
#include "ShapeDisplay.h"

namespace LmbrCentral
{
    void EditorQuadShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorQuadShapeComponent, EditorBaseShapeComponent>()
                ->Version(1)
                ->Field("QuadShape", &EditorQuadShapeComponent::m_quadShape)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorQuadShapeComponent>(
                    "Quad Shape", "The Quad Shape component creates a quad around the associated entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Quad_Shape.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/shape/quad-shape/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorQuadShapeComponent::m_quadShape, "Quad Shape", "Quad Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorQuadShapeComponent::ConfigurationChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void EditorQuadShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        EditorBaseShapeComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("QuadShapeService"));
    }

    void EditorQuadShapeComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorQuadShapeComponent::Init()
    {
        EditorBaseShapeComponent::Init();

        SetShapeComponentConfig(&m_quadShape.ModifyShapeComponent());
    }

    void EditorQuadShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();
        m_quadShape.Activate(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorQuadShapeComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_quadShape.Deactivate();
        EditorBaseShapeComponent::Deactivate();
    }

    void EditorQuadShapeComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DisplayShape(
            debugDisplay, [this]() { return CanDraw(); },
            [this](AzFramework::DebugDisplayRequests& debugDisplay)
            {
                DrawQuadShape(
                    { m_quadShape.GetQuadConfiguration().GetDrawColor(), m_shapeWireColor, m_displayFilled },
                    m_quadShape.GetQuadConfiguration(), debugDisplay, m_quadShape.GetCurrentNonUniformScale());
            },
            m_quadShape.GetCurrentTransform());
    }

    void EditorQuadShapeComponent::ConfigurationChanged()
    {
        m_quadShape.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            GetEntityId(), &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void EditorQuadShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto component = gameEntity->CreateComponent<QuadShapeComponent>())
        {
            component->SetConfiguration(m_quadShape.GetQuadConfiguration());
        }

        if (m_visibleInGameView)
        {
            if (auto component = gameEntity->CreateComponent<QuadShapeDebugDisplayComponent>())
            {
                component->SetConfiguration(m_quadShape.GetQuadConfiguration());
            }
        }
    }
} // namespace LmbrCentral
