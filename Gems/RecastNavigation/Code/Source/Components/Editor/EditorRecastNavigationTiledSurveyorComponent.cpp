/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRecastNavigationTiledSurveyorComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <Components/RecastNavigationTiledSurveyorComponent.h>
#include <LmbrCentral/Scripting/TagComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#pragma optimize("", off)

AZ_DECLARE_BUDGET(Navigation);

namespace RecastNavigation
{
    void EditorRecastNavigationTiledSurveyorComponent::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorRecastNavigationTiledSurveyorComponent, AZ::Component>()
                ->Field("Select by Tags", &EditorRecastNavigationTiledSurveyorComponent::m_tagSelectionList)
                ->Version(1)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EditorRecastNavigationTiledSurveyorComponent>("Recast Navigation Tiled Surveyor",
                    "[Collects the geometry for navigation mesh within the area defined by a shape component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(nullptr, &EditorRecastNavigationTiledSurveyorComponent::m_tagSelectionList, "Select by Tags",
                        "if specified, only entities with Tag component of provided tag values will be considered when building navigation mesh. "
                        "If no tags are specified, any static PhysX object within the area will be included in navigation mesh calculations.")
                    ;
            }
        }
    }

    void EditorRecastNavigationTiledSurveyorComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationSurveyorComponent"));
        provided.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    void EditorRecastNavigationTiledSurveyorComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationSurveyorComponent"));
        incompatible.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    void EditorRecastNavigationTiledSurveyorComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("BoxShapeService"));
    }

    int EditorRecastNavigationTiledSurveyorComponent::GetNumberOfTiles(float tileSize) const
    {
        const AZ::Aabb worldVolume = GetWorldBounds();

        const AZ::Vector3 extents = worldVolume.GetExtents();
        const int tilesAlongX = static_cast<int>(AZStd::ceilf(extents.GetX() / tileSize));
        const int tilesAlongY = static_cast<int>(AZStd::ceilf(extents.GetY() / tileSize));

        return tilesAlongX * tilesAlongY;
    }

    void EditorRecastNavigationTiledSurveyorComponent::AppendColliderGeometry(
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

                overlapHit.m_shape->GetGeometry(vertices, indices, nullptr);
                if (!vertices.empty() && !indices.empty())
                {
                    for (const AZ::Vector3& vertex : vertices)
                    {
                        const AZ::Vector3 translated = t.TransformPoint(vertex);
                        geometry.m_vertices.push_back(RecastVector3(translated));
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

    void EditorRecastNavigationTiledSurveyorComponent::Activate()
    {
        for (const AZStd::string& tagName : m_tagSelectionList)
        {
            m_tags.push_back(AZ_CRC(tagName));
        }

        AZ::Vector3 position = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(position, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        RecastNavigationSurveyorRequestBus::Handler::BusConnect(GetEntityId());
    }

    void EditorRecastNavigationTiledSurveyorComponent::Deactivate()
    {
        RecastNavigationSurveyorRequestBus::Handler::BusDisconnect();
    }

    void EditorRecastNavigationTiledSurveyorComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<RecastNavigationTiledSurveyorComponent>();
    }

    AZStd::vector<AZStd::shared_ptr<TileGeometry>> EditorRecastNavigationTiledSurveyorComponent::CollectGeometry(
        [[maybe_unused]] float tileSize)
    {
        if (tileSize == 0.f)
        {
            return {};
        }

        AZ_PROFILE_SCOPE(Navigation, "Navigation: CollectGeometry tiled");

        AZStd::vector<AZStd::shared_ptr<TileGeometry>> tiles;

        const AZ::Aabb worldVolume = GetWorldBounds();

        const AZ::Vector3 extents = worldVolume.GetExtents();
        int tilesAlongX = static_cast<int>(AZStd::ceilf(extents.GetX() / tileSize));
        int tilesAlongY = static_cast<int>(AZStd::ceilf(extents.GetY() / tileSize));

        const AZ::Vector3& worldMin = worldVolume.GetMin();
        const AZ::Vector3& worldMax = worldVolume.GetMax();

        const AZ::Vector3 border = AZ::Vector3::CreateOne() * 5.f;

        for (int y = 0; y < tilesAlongY; ++y)
        {
            for (int x = 0; x < tilesAlongX; ++x)
            {
                const AZ::Vector3 tileMin{
                    worldMin.GetX() + aznumeric_cast<float>(x) * tileSize,
                    worldMin.GetY() + aznumeric_cast<float>(y) * tileSize,
                    worldMin.GetZ()
                };

                const AZ::Vector3 tileMax{
                    worldMin.GetX() + aznumeric_cast<float>(x + 1) * tileSize,
                    worldMin.GetY() + aznumeric_cast<float>(y + 1) * tileSize,
                    worldMax.GetZ()
                };

                AZ::Aabb tileVolume = AZ::Aabb::CreateFromMinMax(tileMin, tileMax);
                AZ::Aabb scanVolume = AZ::Aabb::CreateFromMinMax(tileMin - border, tileMax + border);

                AzPhysics::SceneQueryHits results;
                AZStd::shared_ptr<TileGeometry> geometryData = AZStd::make_unique<TileGeometry>();
                geometryData->m_worldBounds = tileVolume;
                CollectGeometryWithinVolume(scanVolume, results);

                AppendColliderGeometry(*geometryData, results);

                geometryData->m_tileX = x;
                geometryData->m_tileY = y;
                tiles.push_back(geometryData);
            }
        }

        return tiles;
    }

    void EditorRecastNavigationTiledSurveyorComponent::CollectGeometryWithinVolume(const AZ::Aabb& volume, AzPhysics::SceneQueryHits& overlapHits)
    {
        AZ::Vector3 dimension = volume.GetExtents();
        AZ::Transform pose = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateIdentity(), volume.GetCenter());

        Physics::BoxShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_dimensions = dimension;

        AzPhysics::OverlapRequest request = AzPhysics::OverlapRequestHelpers::CreateBoxOverlapRequest(dimension, pose, nullptr);
        request.m_queryType = AzPhysics::SceneQuery::QueryType::Static;
        request.m_collisionGroup = AzPhysics::CollisionGroup::All;

        auto sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::EditorPhysicsSceneName);
        overlapHits = AZ::Interface<AzPhysics::SceneInterface>::Get()->QueryScene(sceneHandle, &request);
    }

    AZ::Aabb EditorRecastNavigationTiledSurveyorComponent::GetWorldBounds() const
    {
        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(worldBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        return worldBounds;
    }
} // namespace RecastNavigation

#pragma optimize("", on)
