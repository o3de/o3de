/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRecastNavigationMeshComponent.h"

#include <DetourDebugDraw.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <Components/RecastNavigationMeshComponent.h>
#include <RecastNavigation/RecastNavigationSurveyorBus.h>

#pragma optimize("", off)

namespace RecastNavigation
{
    void EditorRecastNavigationMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorRecastNavigationMeshComponent, AZ::Component>()
                ->Field("Config", &EditorRecastNavigationMeshComponent::m_meshConfig)
                ->Field("Show NavMesh in Game", &EditorRecastNavigationMeshComponent::m_showNavigationMesh)
                ->Field("Update Navigation Mesh", &EditorRecastNavigationMeshComponent::m_updateNavigationMeshComponentFlag)
                ->Version(1)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EditorRecastNavigationMeshComponent>("Recast Navigation Mesh", "[Calculates the walkable navigation mesh]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(nullptr, &EditorRecastNavigationMeshComponent::m_showNavigationMesh,
                        "Show NavMesh in Game", "if true, draws a helper overlay over the navigation mesh area")
                    ->DataElement(nullptr, &EditorRecastNavigationMeshComponent::m_meshConfig, "Config", "Navigation Mesh configuration")

                    ->DataElement(AZ::Edit::UIHandlers::Button, &EditorRecastNavigationMeshComponent::m_updateNavigationMeshComponentFlag,
                        "Update Navigation Mesh", "Recalculates and draws the debug view of the mesh in the Editor viewport")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Update Navigation Mesh")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRecastNavigationMeshComponent::UpdatedNavigationMeshInEditor)
                    ;
            }
        }
    }

    void EditorRecastNavigationMeshComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationMeshComponent"));
    }

    void EditorRecastNavigationMeshComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationMeshComponent"));
    }

    void EditorRecastNavigationMeshComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }


    AZ::Crc32 EditorRecastNavigationMeshComponent::UpdatedNavigationMeshInEditor()
    {
        AZStd::vector<AZStd::shared_ptr<TileGeometry>> tiles;
        
        RecastNavigationSurveyorRequestBus::EventResult(tiles, GetEntityId(),
            &RecastNavigationSurveyorRequests::CollectGeometry,
            m_meshConfig.m_tileSize);

        for (AZStd::shared_ptr<TileGeometry>& tile : tiles)
        {
            if (tile->IsEmpty())
            {
                continue;
            }

            NavigationTileData navigationTileData = CreateNavigationTile(tile.get(),
                m_meshConfig, m_context.get());

            m_navMesh->removeTile(m_navMesh->getTileRefAt(tile->m_tileX, tile->m_tileY, 0), nullptr, nullptr);

            if (navigationTileData.IsValid())
            {
                AttachNavigationTileToMesh(navigationTileData);
            }
        }

        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }
    
    void EditorRecastNavigationMeshComponent::Activate()
    {
        EditorComponentBase::Activate();

        m_context = AZStd::make_unique<RecastCustomContext>();

        AZ::Vector3 position = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(position, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);
        
        bool usingTiledSurveyor = false;
        RecastNavigationSurveyorRequestBus::EventResult(usingTiledSurveyor, GetEntityId(), &RecastNavigationSurveyorRequests::IsTiled);
        if (!usingTiledSurveyor)
        {
            // We are using a non-tiled surveyor. Force the tile to cover the entire area.
            AZ::Aabb entireVolume = AZ::Aabb::CreateNull();
            RecastNavigationSurveyorRequestBus::EventResult(entireVolume, GetEntityId(), &RecastNavigationSurveyorRequests::GetWorldBounds);
            m_meshConfig.m_tileSize = AZStd::max(entireVolume.GetExtents().GetX(), entireVolume.GetExtents().GetY());
        }

        CreateNavigationMesh(GetEntityId(), m_meshConfig.m_tileSize);

        m_tickEvent.Enqueue(AZ::TimeMs{ 0 }, true);
        m_updateNavMeshEvent.Enqueue(AZ::TimeMs{ 1000 }, true);
    }

    void EditorRecastNavigationMeshComponent::Deactivate()
    {
        m_tickEvent.RemoveFromQueue();
        m_updateNavMeshEvent.RemoveFromQueue();

        m_context = {};
        m_navQuery = {};
        m_navMesh = {};

        EditorComponentBase::Deactivate();
    }

    void EditorRecastNavigationMeshComponent::OnTick()
    {
        if (!m_navMesh)
        {
            return;
        }

        if (m_showNavigationMesh)
        {
            duDebugDrawNavMesh(&m_customDebugDraw, *m_navMesh, DU_DRAWNAVMESH_COLOR_TILES);
        }
    }

    void EditorRecastNavigationMeshComponent::OnUpdateNavMeshEvent()
    {
        if (!m_navMesh)
        {
            return;
        }

        if (m_showNavigationMesh)
        {
            UpdatedNavigationMeshInEditor();
        }
    }

    void EditorRecastNavigationMeshComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<RecastNavigationMeshComponent>(m_meshConfig, m_showNavigationMesh);
    }
} // namespace RecastNavigation

#pragma optimize("", on)
