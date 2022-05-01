/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecastNavigationTiledSurveyorComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/MathStringConversions.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#pragma optimize("", off)

namespace RecastNavigation
{
    void RecastNavigationTiledSurveyorComponent::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<RecastNavigationTiledSurveyorComponent, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<RecastNavigationTiledSurveyorComponent>("Recast Navigation Tiled Surveyor",
                    "[Collects the geometry for navigation mesh in small batches within the area defined by a shape component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<RecastNavigationSurveyorRequestBus>("RecastNavigationSurveyorRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Attribute(AZ::Script::Attributes::Category, "Navigation")
                ->Event("GetWorldBounds", &RecastNavigationSurveyorRequests::GetWorldBounds)
                ;

            behaviorContext->Class<RecastNavigationTiledSurveyorComponent>()->RequestBus("RecastNavigationSurveyorRequestBus");
        }
    }

    void RecastNavigationTiledSurveyorComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationTiledSurveyorComponent"));
        provided.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    void RecastNavigationTiledSurveyorComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationTiledSurveyorComponent"));
        incompatible.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    void RecastNavigationTiledSurveyorComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("BoxShapeService"));
    }

    void RecastNavigationTiledSurveyorComponent::CollectGeometryWithinVolume(const AZ::Aabb& volume, AzPhysics::SceneQueryHits& overlapHits)
    {
        AZ::Vector3 dimension = volume.GetExtents();
        AZ::Transform pose = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateIdentity(), volume.GetCenter());

        Physics::BoxShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_dimensions = dimension;

        AzPhysics::OverlapRequest request = AzPhysics::OverlapRequestHelpers::CreateBoxOverlapRequest(dimension, pose, nullptr);
        request.m_queryType = AzPhysics::SceneQuery::QueryType::Static;
        request.m_collisionGroup = AzPhysics::CollisionGroup::All;

        auto sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
        overlapHits = AZ::Interface<AzPhysics::SceneInterface>::Get()->QueryScene(sceneHandle, &request);
    }

    void RecastNavigationTiledSurveyorComponent::AppendColliderGeometry(
        TileGeometry& geometry,
        const AzPhysics::SceneQueryHits& overlapHits)
    {
        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;
        AZStd::size_t indicesCount = geometry.m_indices.size();

        for (const AzPhysics::SceneQueryHit& overlapHit : overlapHits.m_hits)
        {
            if ((overlapHit.m_resultFlags & AzPhysics::SceneQuery::EntityId) != 0)
            {
                AZ::EntityId hitEntityId = overlapHit.m_entityId;

                // most physics bodies just have world transforms, but some also have local transforms including terrain.
                // we are not applying the local orientation because it causes terrain geometry to be oriented incorrectly

                AZ::Transform t = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(t, hitEntityId, &AZ::TransformBus::Events::GetWorldTM);
                t.SetUniformScale(1.0f);

                /*AZ_Printf("NavMesh", "world %s, local %s & %s = %s", AZ::ToString(t).c_str(),
                    AZ::ToString(pose.first).c_str(), AZ::ToString(pose.second).c_str(), AZ::ToString(worldTransform).c_str());*/

                overlapHit.m_shape->GetGeometry(vertices, indices, nullptr);
                if (!vertices.empty() && !indices.empty())
                {
                    for (const AZ::Vector3& vertex : vertices)
                    {
                        const AZ::Vector3 translated = t.TransformPoint(vertex);
                        //AZ_Printf("NavMesh", "physx vertex %s -> %s", AZ::ToString(vertex).c_str(), AZ::ToString(translated).c_str());

                        geometry.m_verts.push_back(RecastVector3(translated));
                    }

                    for (size_t i = 2; i < indices.size(); i += 3)
                    {
                        geometry.m_indices.push_back(aznumeric_cast<AZ::u32>(indicesCount + indices[i]));
                        geometry.m_indices.push_back(aznumeric_cast<AZ::u32>(indicesCount + indices[i - 1]));
                        geometry.m_indices.push_back(aznumeric_cast<AZ::u32>(indicesCount + indices[i - 2]));
                    }

                    indicesCount += vertices.size();
                    vertices.clear();
                    indices.clear();
                }
            }
        }
    }

    void RecastNavigationTiledSurveyorComponent::Activate()
    {
        AZ::Vector3 position = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(position, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        RecastNavigationSurveyorRequestBus::Handler::BusConnect(GetEntityId());
    }

    void RecastNavigationTiledSurveyorComponent::Deactivate()
    {
        RecastNavigationSurveyorRequestBus::Handler::BusDisconnect();
    }

    AZStd::vector<AZStd::shared_ptr<TileGeometry>> RecastNavigationTiledSurveyorComponent::CollectGeometry(float tileSize)
    {
        if (tileSize == 0.f)
        {
            return {};
        }

        AZStd::vector<AZStd::shared_ptr<TileGeometry>> tiles;

        const AZ::Aabb worldVolume = GetWorldBounds();

        const AZ::Vector3 extents = worldVolume.GetExtents();
        int tilesAlongX = static_cast<int>(AZStd::ceilf(extents.GetX() / tileSize));
        int tilesAlongY = static_cast<int>(AZStd::ceilf(extents.GetY() / tileSize));

        const AZ::Vector3 worldMin = worldVolume.GetMin();
        const AZ::Vector3 worldMax = worldVolume.GetMax();

        const AZ::Vector3 border = AZ::Vector3::CreateOne() * 5.f;

        for (int y = 0; y < tilesAlongY; ++y)
        {
            for (int x = 0; x < tilesAlongX; ++x)
            {
                AZ::Vector3 tileMin{
                    worldMin.GetX() + x * tileSize,
                    worldMin.GetY() + y * tileSize,
                    worldMin.GetZ()
                };

                AZ::Vector3 tileMax{
                    worldMin.GetX() + (x + 1) * tileSize,
                    worldMin.GetY() + (y + 1) * tileSize,
                    worldMax.GetZ()
                };

                AZ::Aabb tileVolume = AZ::Aabb::CreateFromMinMax(tileMin, tileMax);
                AZ::Aabb scanVolume = AZ::Aabb::CreateFromMinMax(tileMin - border, tileMax + border);

                AzPhysics::SceneQueryHits results;
                AZStd::shared_ptr<TileGeometry> geometryData = AZStd::make_unique<TileGeometry>();
                geometryData->m_worldBounds = tileVolume;
                CollectGeometryWithinVolume(scanVolume, results);

                AZ_Printf("RecastNavigationTiledSurveyorComponent", "Found %llu physx meshes in volume %s",
                    results.m_hits.size(), AZStd::to_string(tileVolume).c_str());

                AppendColliderGeometry(*geometryData, results);

                geometryData->m_tileX = x;
                geometryData->m_tileY = y;
                tiles.push_back(geometryData);
            }
        }

        return tiles;
    }

    AZ::Aabb RecastNavigationTiledSurveyorComponent::GetWorldBounds() const
    {
        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(worldBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        return worldBounds;
    }

    int RecastNavigationTiledSurveyorComponent::GetNumberOfTiles(float tileSize) const
    {
        const AZ::Aabb worldVolume = GetWorldBounds();

        const AZ::Vector3 extents = worldVolume.GetExtents();
        int tilesAlongX = static_cast<int>(AZStd::ceilf(extents.GetX() / tileSize));
        int tilesAlongY = static_cast<int>(AZStd::ceilf(extents.GetY() / tileSize));

        return tilesAlongX * tilesAlongY;
    }
} // namespace RecastNavigation

#pragma optimize("", on)
