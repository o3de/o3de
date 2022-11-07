/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorCapsuleShapeComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include "CapsuleShapeComponent.h"
#include "EditorShapeComponentConverters.h"
#include "ShapeDisplay.h"
#include <LmbrCentral/Geometry/GeometrySystemComponentBus.h>

namespace LmbrCentral
{
    void EditorCapsuleShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate: EditorCapsuleColliderComponent -> EditorCapsuleShapeComponent
            serializeContext->ClassDeprecate(
                "EditorCapsuleColliderComponent",
                AZ::Uuid("{63247EE1-B081-40D9-8AE2-98E5C738EBD8}"),
                &ClassConverters::DeprecateEditorCapsuleColliderComponent)
                ;

            serializeContext->Class<EditorCapsuleShapeComponent, EditorBaseShapeComponent>()
                ->Version(3, &ClassConverters::UpgradeEditorCapsuleShapeComponent)
                ->Field("CapsuleShape", &EditorCapsuleShapeComponent::m_capsuleShape)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorCapsuleShapeComponent>("Capsule Shape", "The Capsule Shape component creates a capsule around the associated entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Capsule_Shape.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Capsule_Shape.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/shape/capsule-shape/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCapsuleShapeComponent::m_capsuleShape, "Capsule Shape", "Capsule Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCapsuleShapeComponent::ConfigurationChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
            }
        }
    }

    void EditorCapsuleShapeComponent::Init()
    {
        EditorBaseShapeComponent::Init();

        SetShapeComponentConfig(&m_capsuleShape.ModifyCapsuleConfiguration());
    }

    void EditorCapsuleShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();
        m_capsuleShape.Activate(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());

        GenerateVertices();
    }

    void EditorCapsuleShapeComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_capsuleShape.Deactivate();
        EditorBaseShapeComponent::Deactivate();
    }

    void EditorCapsuleShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        EditorBaseShapeComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorCapsuleShapeComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DisplayShape(
            debugDisplay,
            [this]()
            {
                return CanDraw();
            },
            [this](AzFramework::DebugDisplayRequests& debugDisplay)
            {
                DrawShape(
                    debugDisplay,
                    ShapeDrawParams{ m_shapeColor, m_shapeWireColor, m_displayFilled },
                    m_capsuleShapeMesh,
                    m_capsuleShape.GetTranslationOffset());
            },
            m_capsuleShape.GetCurrentTransform());
    }

    void EditorCapsuleShapeComponent::ClampHeight()
    {
        // make sure the height is at least twice the radius so that the capsule is tall enough to accommodate the end caps
        const float height = m_capsuleShape.GetCapsuleConfiguration().m_height;
        const float radius = m_capsuleShape.GetCapsuleConfiguration().m_radius;
        m_capsuleShape.ModifyCapsuleConfiguration().m_height = AZ::GetMax(height, 2.0f * radius);
    }

    AZ::Crc32 EditorCapsuleShapeComponent::ConfigurationChanged()
    {
        ClampHeight();
        GenerateVertices();
        m_capsuleShape.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            GetEntityId(), &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    void EditorCapsuleShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto component = gameEntity->CreateComponent<CapsuleShapeComponent>())
        {
            component->SetConfiguration(m_capsuleShape.GetCapsuleConfiguration());
        }

        if (m_visibleInGameView)
        {
            if (auto component = gameEntity->CreateComponent<CapsuleShapeDebugDisplayComponent>())
            {
                component->SetConfiguration(m_capsuleShape.GetCapsuleConfiguration());
            }
        }
    }

    void EditorCapsuleShapeComponent::GenerateVertices()
    {
        CapsuleGeometrySystemRequestBus::Broadcast(
            &CapsuleGeometrySystemRequestBus::Events::GenerateCapsuleMesh,
            m_capsuleShape.GetCapsuleConfiguration().m_radius,
            m_capsuleShape.GetCapsuleConfiguration().m_height,
            g_capsuleDebugShapeSides,
            g_capsuleDebugShapeCapSegments,
            m_capsuleShapeMesh.m_vertexBuffer,
            m_capsuleShapeMesh.m_indexBuffer,
            m_capsuleShapeMesh.m_lineBuffer);
    }
} // namespace LmbrCentral
