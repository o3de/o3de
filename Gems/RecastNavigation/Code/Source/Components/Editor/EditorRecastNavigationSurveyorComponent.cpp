/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRecastNavigationSurveyorComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <Components/RecastNavigationSurveyorComponent.h>
#include <LmbrCentral/Scripting/TagComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#pragma optimize("", off)

namespace RecastNavigation
{
    void EditorRecastNavigationSurveyorComponent::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorRecastNavigationSurveyorComponent, AZ::Component>()
                ->Field("Select by Tags", &EditorRecastNavigationSurveyorComponent::m_tagSelectionList)
                ->Version(1)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EditorRecastNavigationSurveyorComponent>("Recast Navigation Surveyor",
                    "[Collects the geometry for navigation mesh within the area defined by a shape component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(nullptr, &EditorRecastNavigationSurveyorComponent::m_tagSelectionList, "Select by Tags",
                        "if specified, only entities with Tag component of provided tag values will be considered when building navigation mesh. "
                        "If no tags are specified, any static PhysX object within the area will be included in navigation mesh calculations.")
                    ;
            }
        }
    }

    void EditorRecastNavigationSurveyorComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationSurveyorComponent"));
        provided.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    void EditorRecastNavigationSurveyorComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationSurveyorComponent"));
        incompatible.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    void EditorRecastNavigationSurveyorComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("BoxShapeService"));
    }

    void EditorRecastNavigationSurveyorComponent::AppendColliderGeometry(
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

    void EditorRecastNavigationSurveyorComponent::Activate()
    {
        for (const AZStd::string& tagName : m_tagSelectionList)
        {
            m_tags.push_back(AZ_CRC(tagName));
        }

        AZ::Vector3 position = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(position, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        RecastNavigationSurveyorRequestBus::Handler::BusConnect(GetEntityId());
    }

    void EditorRecastNavigationSurveyorComponent::Deactivate()
    {
        RecastNavigationSurveyorRequestBus::Handler::BusDisconnect();
    }

    void EditorRecastNavigationSurveyorComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<RecastNavigationSurveyorComponent>(m_tags);
    }

    AZStd::vector<AZStd::shared_ptr<TileGeometry>> EditorRecastNavigationSurveyorComponent::CollectGeometry(
        [[maybe_unused]] float tileSize)
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
        AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::EditorPhysicsSceneName);
        AzPhysics::SceneQueryHits results = AZ::Interface<AzPhysics::SceneInterface>::Get()->QueryScene(sceneHandle, &request);

        if (results.m_hits.empty())
        {
            return {};
        }

        AppendColliderGeometry(*geometryData, results);

        return { geometryData };
    }

    AZ::Aabb EditorRecastNavigationSurveyorComponent::GetWorldBounds() const
    {
        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(worldBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        return worldBounds;
    }
} // namespace RecastNavigation

#pragma optimize("", on)
