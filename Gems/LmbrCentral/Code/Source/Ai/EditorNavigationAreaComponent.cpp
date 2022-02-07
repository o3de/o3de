/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorNavigationAreaComponent.h"

#include "EditorNavigationUtil.h"
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>
#include <Shape/PolygonPrismShape.h>
#include <MathConversion.h>

namespace LmbrCentral
{
    static bool NavAgentValid(NavigationAgentTypeID navAgentId)
    {
        return navAgentId != NavigationAgentTypeID();
    }

    static bool NavVolumeValid(NavigationVolumeID navVolumeId)
    {
        return navVolumeId != NavigationVolumeID();
    }

    static bool NavMeshValid(NavigationMeshID navMeshId)
    {
        return navMeshId != NavigationMeshID();
    }

    void EditorNavigationAreaComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorNavigationAreaComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Field("AgentTypes", &EditorNavigationAreaComponent::m_agentTypes)
                ->Field("Exclusion", &EditorNavigationAreaComponent::m_exclusion)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorNavigationAreaComponent>("Navigation Area", "Navigation Area configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AddableByUser, false)
                        ->Attribute(AZ::Edit::Attributes::Category, "AI")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/NavigationArea.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/NavigationArea.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/ai/nav-area/")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &EditorNavigationAreaComponent::m_exclusion, "Exclusion", "Does this area add or subtract from the Navigation Mesh")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorNavigationAreaComponent::OnNavigationAreaChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorNavigationAreaComponent::m_agentTypes, "Agent Types", "All agents that could potentially be used with this area")
                        ->ElementAttribute(AZ::Edit::UIHandlers::Handler, AZ::Edit::UIHandlers::ComboBox)
                        ->ElementAttribute(AZ::Edit::Attributes::StringList, &PopulateAgentTypeList)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::AddNotify, &EditorNavigationAreaComponent::OnNavigationAreaChanged)
                        ->Attribute(AZ::Edit::Attributes::RemoveNotify, &EditorNavigationAreaComponent::OnNavigationAreaChanged)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorNavigationAreaComponent::OnNavigationAreaChanged)
                        ;
            }
        }
    }

    EditorNavigationAreaComponent::EditorNavigationAreaComponent()
        : m_navigationAreaChanged([this]() { UpdateMeshes(); ApplyExclusion(); })
    {
    }

    EditorNavigationAreaComponent::~EditorNavigationAreaComponent()
    {
        DestroyArea();
    }

    void EditorNavigationAreaComponent::Activate()
    {
        EditorComponentBase::Activate();

        const AZ::EntityId entityId = GetEntityId();
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        ShapeComponentNotificationsBus::Handler::BusConnect(entityId);
        NavigationAreaRequestBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        AzToolsFramework::EntityCompositionNotificationBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        // use the entity id as unique name to register area
        m_name = GetEntityId().ToString();

        INavigationSystem* aiNavigation = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
        if (aiNavigation)
        {
            // we only wish to register new areas (this area may have been
            // registered when the navmesh was loaded at level load)
            if (!aiNavigation->IsAreaPresent(m_name.c_str()))
            {
                aiNavigation->RegisterArea(m_name.c_str());
            }
        }

        // reset switching to game mode on activate
        m_switchingToGameMode = false;

        // We must relink during entity activation or the NavigationSystem will throw 
        // errors in SpawnJob. Don't force an unnecessary update of the game area.  
        // RelinkWithMesh will still update the game area if the volume hasn't been created.
        const bool updateGameArea = false;
        RelinkWithMesh(updateGameArea);
    }

    void EditorNavigationAreaComponent::Deactivate()
    {
        // only destroy the area if we know we're not currently switching to game mode
        // or changing our composition during scrubbing
        if (!m_switchingToGameMode && !m_compositionChanging)
        {
            DestroyArea();
        }

        const AZ::EntityId entityId = GetEntityId();
        AZ::TransformNotificationBus::Handler::BusDisconnect(entityId);
        ShapeComponentNotificationsBus::Handler::BusDisconnect(entityId);
        NavigationAreaRequestBus::Handler::BusDisconnect(entityId);
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EntityCompositionNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();

        EditorComponentBase::Deactivate();
    }

    void EditorNavigationAreaComponent::OnNavigationEvent(const INavigationSystem::ENavigationEvent event)
    {
        switch (event)
        {
        case INavigationSystem::MeshReloaded:
        case INavigationSystem::NavigationCleared:
            RelinkWithMesh(true);
            break;
        case INavigationSystem::MeshReloadedAfterExporting:
            RelinkWithMesh(false);
            break;
        default:
            AZ_Assert(false, "Unhandled ENavigationEvent: %d", event);
            break;
        }
    }

    void EditorNavigationAreaComponent::OnShapeChanged(ShapeChangeReasons /*changeReason*/)
    {
        UpdateGameArea();
    }

    void EditorNavigationAreaComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        UpdateGameArea();
    }

    void EditorNavigationAreaComponent::RefreshArea()
    {
        UpdateGameArea();
    }

    void EditorNavigationAreaComponent::UpdateGameArea()
    {
        using namespace PolygonPrismUtil;

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::ConstPolygonPrismPtr polygonPrismPtr = nullptr;
        PolygonPrismShapeComponentRequestBus::EventResult(polygonPrismPtr, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);

        if (!polygonPrismPtr)
        {
            AZ_Error("EditorNavigationAreaComponent", false, "Polygon prism does not exist for navigation area.");
            return;
        }

        const AZ::PolygonPrism& polygonPrism = *polygonPrismPtr;

        INavigationSystem* aiNavigation = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
        if (aiNavigation)
        {
            const size_t vertexCount = polygonPrism.m_vertexContainer.Size();
            const AZStd::vector<AZ::Vector2>& verticesLocal = polygonPrism.m_vertexContainer.GetVertices();

            if (vertexCount > 2)
            {
                AZStd::vector<AZ::Vector3> verticesWorld;
                verticesWorld.reserve(vertexCount);

                for (size_t i = 0; i < vertexCount; ++i)
                {
                    verticesWorld.push_back(transform.TransformPoint(AZ::Vector2ToVector3(verticesLocal[i])));
                }

                // The volume could be set but if the binary data didn't exist the volume was not correctly recreated
                if (!NavVolumeValid(NavigationVolumeID(m_volume)) || !aiNavigation->ValidateVolume(NavigationVolumeID(m_volume)))
                {
                    CreateVolume(&verticesWorld[0], verticesWorld.size(), NavigationVolumeID(m_volume));
                }
                else
                {
                    AZStd::vector<Vec3> cryVertices;
                    cryVertices.reserve(vertexCount);

                    for (size_t i = 0; i < vertexCount; ++i)
                    {
                        cryVertices.push_back(AZVec3ToLYVec3(verticesWorld[i]));
                    }

                    aiNavigation->SetVolume(NavigationVolumeID(m_volume), &cryVertices[0], cryVertices.size(), polygonPrism.GetHeight());
                }

                UpdateMeshes();
                ApplyExclusion();
            }
            else if (NavVolumeValid(NavigationVolumeID(m_volume)))
            {
                DestroyArea();
            }
        }
    }

    void EditorNavigationAreaComponent::UpdateMeshes()
    {
        INavigationSystem* aiNavigation = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
        if (aiNavigation)
        {
            if (m_exclusion)
            {
                DestroyMeshes();
            }
            else
            {
                const size_t agentTypeCount = m_agentTypes.size();
                m_meshes.resize(agentTypeCount);

                for (size_t i = 0; i < agentTypeCount; ++i)
                {
                    NavigationMeshID meshId = NavigationMeshID(m_meshes[i]);
                    const NavigationAgentTypeID agentTypeId = aiNavigation->GetAgentTypeID(m_agentTypes[i].c_str());

                    if (NavAgentValid(agentTypeId) && !meshId)
                    {
                        INavigationSystem::CreateMeshParams params; // TODO: expose at least the tile size
                        meshId = aiNavigation->CreateMesh(m_name.c_str(), agentTypeId, params);
                        aiNavigation->SetMeshBoundaryVolume(meshId, NavigationVolumeID(m_volume));

                        AZ::Transform transform = AZ::Transform::CreateIdentity();
                        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

                        AZ::ConstPolygonPrismPtr polygonPrismPtr;
                        PolygonPrismShapeComponentRequestBus::EventResult(polygonPrismPtr, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);

                        const AZ::PolygonPrism& polygonPrism = *polygonPrismPtr;
                        aiNavigation->QueueMeshUpdate(meshId, AZAabbToLyAABB(PolygonPrismUtil::CalculateAabb(polygonPrism, transform)));

                        m_meshes[i] = meshId;
                    }
                    else if (!NavAgentValid(agentTypeId) && meshId)
                    {
                        aiNavigation->DestroyMesh(meshId);
                        m_meshes[i] = 0;
                    }
                }
            }
        }
    }

    void EditorNavigationAreaComponent::ApplyExclusion()
    {
        INavigationSystem* aiNavigation = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
        if (aiNavigation)
        {
            std::vector<NavigationAgentTypeID> affectedAgentTypes;

            if (m_exclusion)
            {
                const size_t agentTypeCount = m_agentTypes.size();
                affectedAgentTypes.reserve(agentTypeCount);

                for (size_t i = 0; i < agentTypeCount; ++i)
                {
                    NavigationAgentTypeID agentTypeID = aiNavigation->GetAgentTypeID(m_agentTypes[i].c_str());
                    affectedAgentTypes.push_back(agentTypeID);
                }
            }

            if (affectedAgentTypes.empty())
            {
                // this will remove this volume from all agent type and mesh exclusion containers
                aiNavigation->SetExclusionVolume(0, 0, NavigationVolumeID(m_volume));
            }
            else
            {
                aiNavigation->SetExclusionVolume(&affectedAgentTypes[0], affectedAgentTypes.size(), NavigationVolumeID(m_volume));
            }
        }
    }

    void EditorNavigationAreaComponent::RelinkWithMesh(bool updateGameArea)
    {
        INavigationSystem* aiNavigation = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
        if (aiNavigation)
        {
            m_volume = aiNavigation->GetAreaId(m_name.c_str());

            if (!m_exclusion)
            {
                const size_t agentTypeCount = m_agentTypes.size();
                m_meshes.resize(agentTypeCount);

                for (size_t i = 0; i < agentTypeCount; ++i)
                {
                    const NavigationAgentTypeID agentTypeId = aiNavigation->GetAgentTypeID(m_agentTypes[i].c_str());
                    m_meshes[i] = aiNavigation->GetMeshID(m_name.c_str(), agentTypeId);
                }
            }

            // Update the game area if requested or in the case that the volume doesn't exist yet.
            // This can happen when a volume doesn't have an associated mesh which is always the  
            // case with exclusion volumes.
            if (updateGameArea || !aiNavigation->ValidateVolume(NavigationVolumeID(m_volume)))
            {
                UpdateGameArea();
            }
        }
    }

    void EditorNavigationAreaComponent::CreateVolume(AZ::Vector3* vertices, size_t vertexCount, NavigationVolumeID requestedID)
    {
        INavigationSystem* aiNavigation = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
        if (aiNavigation)
        {
            AZStd::vector<Vec3> cryVertices;
            cryVertices.reserve(vertexCount);

            for (size_t i = 0; i < vertexCount; ++i)
            {
                cryVertices.push_back(AZVec3ToLYVec3(vertices[i]));
            }

            AZ::ConstPolygonPrismPtr polygonPrismPtr;
            PolygonPrismShapeComponentRequestBus::EventResult(polygonPrismPtr, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);

            const AZ::PolygonPrism& polygonPrism = *polygonPrismPtr;
            m_volume = aiNavigation->CreateVolume(cryVertices.begin(), vertexCount, polygonPrism.GetHeight(), requestedID);
            aiNavigation->RegisterListener(this, m_name.c_str());

            if (!NavVolumeValid(requestedID))
            {
                aiNavigation->SetAreaId(m_name.c_str(), NavigationVolumeID(m_volume));
            }
        }
    }

    void EditorNavigationAreaComponent::DestroyVolume()
    {
        INavigationSystem* aiNavigation = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
        if (aiNavigation)
        {
            if (NavVolumeValid(NavigationVolumeID(m_volume)))
            {
                aiNavigation->DestroyVolume(NavigationVolumeID(m_volume));
                aiNavigation->UnRegisterListener(this);

                m_volume = NavigationVolumeID();
            }
        }
    }

    void EditorNavigationAreaComponent::DestroyMeshes()
    {
        INavigationSystem* aiNavigation = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
        if (aiNavigation)
        {
            for (size_t i = 0; i < m_meshes.size(); ++i)
            {
                if (NavMeshValid(NavigationMeshID(m_meshes[i])))
                {
                    aiNavigation->DestroyMesh(NavigationMeshID(m_meshes[i]));
                }
            }

            m_meshes.clear();
        }
    }

    void EditorNavigationAreaComponent::DestroyArea()
    {
        INavigationSystem* aiNavigation = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
        if (aiNavigation)
        {
            aiNavigation->UnRegisterArea(m_name.c_str());
            DestroyMeshes();
            DestroyVolume();
        }
    }

    void EditorNavigationAreaComponent::OnStartPlayInEditorBegin()
    {
        m_switchingToGameMode = true;
    }

    void EditorNavigationAreaComponent::OnEntityCompositionChanging(const AzToolsFramework::EntityIdList& entityIds)
    {
        if (AZStd::find(entityIds.begin(), entityIds.end(), GetEntityId()) != entityIds.end())
        {
            m_compositionChanging = true;
        }
    }

    void EditorNavigationAreaComponent::OnEntityCompositionChanged(const AzToolsFramework::EntityIdList& entityIds)
    {
        if (AZStd::find(entityIds.begin(), entityIds.end(), GetEntityId()) != entityIds.end())
        {
            m_compositionChanging = false;
        }
    }

    void EditorNavigationAreaComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        m_compositionChanging = false;

        // disconnect from the composition and tick bus because we no longer need to
        // be concerned with entity scrubbing causing our navigation area to get rebuilt
        AzToolsFramework::EntityCompositionNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

} // namespace LmbrCentral
