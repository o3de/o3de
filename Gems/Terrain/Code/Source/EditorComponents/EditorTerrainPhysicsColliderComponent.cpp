/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorComponents/EditorTerrainPhysicsColliderComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace Terrain
{
    void EditorTerrainPhysicsColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorTerrainPhysicsColliderComponent>()
                ->Version(1)
                ->Field("Visible", &EditorTerrainPhysicsColliderComponent::m_visibleInEditor)
                ->Field("Color", &EditorTerrainPhysicsColliderComponent::m_drawColor);

            if (auto editContext = serialize->GetEditContext())
            {
                editContext->Class<EditorTerrainPhysicsColliderComponent>(s_componentName, s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, s_icon)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &EditorTerrainPhysicsColliderComponent::m_visibleInEditor, "Visible",
                        "Always display this shape in the editor viewport.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorTerrainPhysicsColliderComponent::m_drawColor, "Color",
                        "The color to draw the debug image.");
            }
        }
    }

    void EditorTerrainPhysicsColliderComponent::Activate()
    {
        BaseClassType::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();

        GenerateDebugDrawData();
    }

    void EditorTerrainPhysicsColliderComponent::Deactivate()
    {
        BaseClassType::Deactivate();
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
    }

    void EditorTerrainPhysicsColliderComponent::GenerateDebugDrawData()
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_drawMutex);

        AZ::Aabb boxBounds = AZ::Aabb::CreateNull();
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        LmbrCentral::ShapeComponentRequestsBus::Event(
            GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetTransformAndLocalBounds, transform, boxBounds);

        int32_t gridWidth, gridHeight;
        m_component.GetHeightfieldGridSize(gridWidth, gridHeight);

        const AZ::Vector2 gridResolution = m_component.GetHeightfieldGridSpacing();
        auto heights = m_component.GetHeights();

        m_debugQuads.clear();
        m_debugQuads.reserve((gridWidth - 1) * (gridHeight - 1));

        for (int xIndex = 0; xIndex < gridWidth - 1; xIndex++)
        {
            for (int yIndex = 0; yIndex < gridHeight - 1; yIndex++)
            {
                const int index0 = yIndex * gridWidth + xIndex;
                const int index1 = yIndex * gridWidth + xIndex + 1;
                const int index2 = (yIndex + 1) * gridWidth + xIndex + 1;
                const int index3 = (yIndex + 1) * gridWidth + xIndex;

                const float x0 = boxBounds.GetMin().GetX() + gridResolution.GetX() * xIndex;
                const float x1 = boxBounds.GetMin().GetX() + gridResolution.GetX() * (xIndex + 1);
                const float y0 = boxBounds.GetMin().GetY() + gridResolution.GetY() * (yIndex + 1);
                const float y1 = boxBounds.GetMin().GetY() + gridResolution.GetY() * yIndex;

                m_debugQuads.push_back(
                    DebugImageQuad{
                        AZ::Vector3(x0, y0, heights[index0]),
                        AZ::Vector3(x1, y0, heights[index1]),
                        AZ::Vector3(x1, y1, heights[index2]),
                        AZ::Vector3(x0, y1, heights[index3])
                    }
                );
            }
        }
    }

    void EditorTerrainPhysicsColliderComponent::OnTerrainDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion, [[maybe_unused]] TerrainDataChangedMask dataChangedMask)
    {
        GenerateDebugDrawData();
    }

    void EditorTerrainPhysicsColliderComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, [[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (!m_visibleInEditor && !IsSelected())
        {
            return;
        }

        AZStd::shared_lock<AZStd::shared_mutex> lock(m_drawMutex);

        AZ::Transform worldFromLocalWithUniformScale = m_component.GetHeightfieldTransform();
        worldFromLocalWithUniformScale.SetUniformScale(worldFromLocalWithUniformScale.GetUniformScale());

        debugDisplay.PushMatrix(worldFromLocalWithUniformScale);
        debugDisplay.SetColor(m_drawColor);

        for (const auto quad : m_debugQuads)
        {
            debugDisplay.DrawWireQuad(quad.m_point0, quad.m_point1, quad.m_point2, quad.m_point3);
        }

        debugDisplay.PopMatrix();
    }
}
