/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRecastNavigationMeshComponent.h"
#include "EditorRecastNavigationMeshConfig.h"

#include <DetourDebugDraw.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/Shape.h>
#include <Components/RecastNavigationMeshComponent.h>
#include <RecastNavigation/RecastNavigationSurveyorBus.h>

AZ_DECLARE_BUDGET(Navigation);

//#pragma optimize("", off)

namespace RecastNavigation
{
    EditorRecastNavigationMeshComponent::EditorRecastNavigationMeshComponent()
        : m_autoUpdateHandler([this](bool changed) { OnAutoUpdateChanged(changed); })
        , m_showMeshHandler([this](bool changed) { OnShowNavMeshChanged(changed); })
        , m_tickEvent([this]() { OnTick(); }, AZ::Name("EditorRecastNavigationDebugViewTick"))
        , m_updateNavMeshEvent([this]() { OnUpdateNavMeshEvent(); }, AZ::Name("EditorRecastNavigationUpdateNavMeshInEditor"))
    {
    }

    void EditorRecastNavigationMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorRecastNavigationMeshConfig::Reflect(context);

        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorRecastNavigationMeshComponent, AZ::Component>()
                ->Field("Configurations", &EditorRecastNavigationMeshComponent::m_meshConfig)
                ->Field("Debug Options", &EditorRecastNavigationMeshComponent::m_meshEditorConfig)
                ->Field("Update Navigation Mesh", &EditorRecastNavigationMeshComponent::m_updateNavigationMeshComponentFlag)
                ->Version(1)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EditorRecastNavigationMeshComponent>("Recast Navigation Mesh", "[Calculates the walkable navigation mesh]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(nullptr, &EditorRecastNavigationMeshComponent::m_meshConfig,
                        "Configurations", "Navigation Mesh configuration")
                    ->DataElement(nullptr, &EditorRecastNavigationMeshComponent::m_meshEditorConfig,
                        "Debug Options", "Various helper options for Editor viewport")

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
        OnUpdateNavMeshEvent();

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

        m_navigationTaskExecutor = AZStd::make_unique<AZ::TaskExecutor>(m_meshEditorConfig.m_backgroundThreadsToUse);

        m_meshEditorConfig.BindAutoUpdateChangedEventHandler(m_autoUpdateHandler);
        m_meshEditorConfig.BindShowNavMeshChangedEventHandler(m_showMeshHandler);

        OnAutoUpdateChanged(m_meshEditorConfig.m_autoUpdateNavigationMesh);
        OnShowNavMeshChanged(m_meshEditorConfig.m_showNavigationMesh);
    }

    void EditorRecastNavigationMeshComponent::Deactivate()
    {
        m_autoUpdateHandler.Disconnect();
        m_showMeshHandler.Disconnect();

        if (m_taskGraphEvent)
        {
            m_taskGraphEvent->Wait();
            m_taskGraphEvent = {};
        }
        m_navigationTaskExecutor = {};
        m_graph = {};

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

        if (m_meshEditorConfig.m_showNavigationMesh)
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

        if (m_meshEditorConfig.m_showNavigationMesh)
        {
            bool updateNow = false;
            {
                AZStd::lock_guard lock(m_navigationMeshMutex);
                if (!m_updatingNavMeshInProgress)
                {
                    m_updatingNavMeshInProgress = true;
                    updateNow = true;
                }
            }

            if (updateNow)
            {
                AZ_PROFILE_SCOPE(Navigation, "Navigation: OnUpdateNavMeshEvent");

                m_graph = AZStd::make_unique<AZ::TaskGraph>();

                AZStd::vector<AZStd::shared_ptr<TileGeometry>> tiles;

                {
                    AZ_PROFILE_SCOPE(Navigation, "Navigation: CollectGeometry");
                    RecastNavigationSurveyorRequestBus::EventResult(tiles, GetEntityId(),
                        &RecastNavigationSurveyorRequests::CollectGeometry,
                        m_meshConfig.m_tileSize, m_meshConfig.m_borderSize * m_meshConfig.m_cellSize);
                }

                AZ::TaskToken updateDoneTask = m_graph->AddTask(
                    m_taskDescriptor,
                    [this]
                    {
                        AZ_PROFILE_SCOPE(Navigation, "Navigation: update finished");
                        AZStd::lock_guard lock(m_navigationMeshMutex);
                        m_updatingNavMeshInProgress = false;
                    });

                AZStd::vector<AZ::TaskToken*> tasks;

                for (AZStd::shared_ptr<TileGeometry>& tile : tiles)
                {
                    if (tile->IsEmpty())
                    {
                        continue;
                    }

                    AZ::TaskToken processAndAddTileTask = m_graph->AddTask(
                        m_taskDescriptor,
                        [this, tile]
                        {
                            AZ_PROFILE_SCOPE(Navigation, "Navigation: processing a tile");

                            NavigationTileData navigationTileData = CreateNavigationTile(
                                tile.get(),
                                m_meshConfig, m_context.get());

                            {
                                AZStd::lock_guard lock(m_navigationMeshMutex);
                                m_navMesh->removeTile(m_navMesh->getTileRefAt(tile->m_tileX, tile->m_tileY, 0), nullptr, nullptr);

                                if (navigationTileData.IsValid())
                                {
                                    AttachNavigationTileToMesh(navigationTileData);
                                }
                            }
                        });

                    tasks.push_back(&processAndAddTileTask);
                }

                for (AZ::TaskToken* task : tasks)
                {
                    task->Precedes(updateDoneTask);
                }

                m_taskGraphEvent = AZStd::make_unique<AZ::TaskGraphEvent>();
                m_graph->SubmitOnExecutor(*m_navigationTaskExecutor, m_taskGraphEvent.get());
            }
        }
    }

    void EditorRecastNavigationMeshComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<RecastNavigationMeshComponent>(m_meshConfig, m_meshEditorConfig.m_showNavigationMesh);
    }

    void EditorRecastNavigationMeshComponent::OnAutoUpdateChanged(bool changed)
    {
        if (changed)
        {
            m_updateNavMeshEvent.Enqueue(AZ::TimeMs{ 1000 }, true);
        }
        else
        {
            m_updateNavMeshEvent.RemoveFromQueue();
        }
    }

    void EditorRecastNavigationMeshComponent::OnShowNavMeshChanged(bool changed)
    {
        if (changed)
        {
            m_tickEvent.Enqueue(AZ::TimeMs{ 0 }, true);
        }
        else
        {
            m_tickEvent.RemoveFromQueue();
        }
    }
} // namespace RecastNavigation

//#pragma optimize("", on)
