/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecastNavigationSurveyorComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <LmbrCentral/Scripting/TagComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#pragma optimize("", off)

namespace RecastNavigation
{
    RecastNavigationSurveyorComponent::RecastNavigationSurveyorComponent(const AZStd::vector<AZ::u32>& tags)
        : m_tags(tags)
    {
    }

    void RecastNavigationSurveyorComponent::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<RecastNavigationSurveyorComponent, AZ::Component>()
                ->Version(1)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<RecastNavigationSurveyorRequestBus>("RecastNavigationSurveyorRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Attribute(AZ::Script::Attributes::Category, "Navigation")
                ->Event("GetWorldBounds", &RecastNavigationSurveyorRequests::GetWorldBounds)
                ;

            behaviorContext->Class<RecastNavigationSurveyorComponent>()->RequestBus("RecastNavigationSurveyorRequestBus");
        }
    }

    void RecastNavigationSurveyorComponent::AppendColliderGeometry(
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

    void RecastNavigationSurveyorComponent::Activate()
    {
        RecastNavigationSurveyorRequestBus::Handler::BusConnect(GetEntityId());
    }

    void RecastNavigationSurveyorComponent::Deactivate()
    {
        RecastNavigationSurveyorRequestBus::Handler::BusDisconnect();
    }

    AZStd::vector<AZStd::shared_ptr<TileGeometry>> RecastNavigationSurveyorComponent::CollectGeometry(
        [[maybe_unused]] float tileSize, [[maybe_unused]] float borderSize)
    {
        AZStd::shared_ptr<TileGeometry> geometryData = AZStd::make_unique<TileGeometry>();

        LmbrCentral::ShapeComponentRequestsBus::EventResult(geometryData->m_worldBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        AZ::Vector3 dimension = geometryData->m_worldBounds.GetExtents();
        AZ::Transform pose = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateIdentity(), geometryData->m_worldBounds.GetCenter());

        Physics::BoxShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_dimensions = dimension;

        AzPhysics::SceneQuery::OverlapFilterCallback filterCallback = [this](const AzPhysics::SimulatedBody* body, const Physics::Shape*) -> bool
        {
            LmbrCentral::Tags tags;
            LmbrCentral::TagComponentRequestBus::EventResult(tags, body->GetEntityId(), &LmbrCentral::TagComponentRequestBus::Events::GetTags);
            for (const auto allowedTag : m_tags)
            {
                if (tags.find(allowedTag) != tags.end())
                {
                    return true;
                }
            }

            return false;
        };

        AzPhysics::OverlapRequest request = AzPhysics::OverlapRequestHelpers::CreateBoxOverlapRequest(dimension, pose,
            m_tags.empty() ? nullptr : filterCallback);
        request.m_queryType = AzPhysics::SceneQuery::QueryType::Static;
        request.m_collisionGroup = AzPhysics::CollisionGroup::All;

        auto sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
        AzPhysics::SceneQueryHits results = AZ::Interface<AzPhysics::SceneInterface>::Get()->QueryScene(sceneHandle, &request);

        if (results.m_hits.empty())
        {
            return {};
        }

        AZ_Printf("RecastNavigationSurveyorComponent", "found %llu physx meshes", results.m_hits.size());

        AppendColliderGeometry(*geometryData, results);

        return { geometryData };
    }

    AZ::Aabb RecastNavigationSurveyorComponent::GetWorldBounds() const
    {
        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(worldBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        return worldBounds;
    }
} // namespace RecastNavigation

#pragma optimize("", on)
