/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>
#include <AzFramework/Physics/PhysicsSystem.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(SceneQueryRequest, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(RayCastRequest, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(ShapeCastRequest, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(OverlapRequest, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(SceneQueryHit, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(SceneQueryHits, AZ::SystemAllocator);

    namespace Internal
    {
        AZStd::vector<AZStd::pair<CollisionGroup, AZStd::string>> PopulateCollisionGroups()
        {
            AZStd::vector<AZStd::pair<CollisionGroup, AZStd::string>> elems;
            const CollisionConfiguration& configuration = AZ::Interface<AzPhysics::SystemInterface>::Get()->GetConfiguration()->m_collisionConfig;
            for (const CollisionGroups::Preset& preset : configuration.m_collisionGroups.GetPresets())
            {
                elems.push_back({ CollisionGroup(preset.m_name), preset.m_name });
            }
            return elems;
        }
    }

    namespace SceneQuery
    {
        /*static*/ void ReflectSceneQueryObjects(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                if (auto* editContext = serializeContext->GetEditContext())
                {
                    editContext->Enum<QueryType>("Query Type Flags", "Object types to include in the query")
                        ->Value("Static", QueryType::Static)
                        ->Value("Dynamic", QueryType::Dynamic)
                        ->Value("Static and Dynamic", QueryType::StaticAndDynamic)
                        ;
                }
            }

            SceneQueryRequest::Reflect(context);
            RayCastRequest::Reflect(context);
            ShapeCastRequest::Reflect(context);
            OverlapRequest::Reflect(context);
            SceneQueryHits::Reflect(context);
        }
    }

    // class for exposing free functions to script
    class SceneQueries
    {
    public:
        AZ_TYPE_INFO(SceneQueries, "{4EFA3DA5-C0E3-4753-8C55-202228CA527E}");
        AZ_CLASS_ALLOCATOR(SceneQueries, AZ::SystemAllocator);

        SceneQueries() = default;
        ~SceneQueries() = default;
    };

    /*static*/ void SceneQueryRequest::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SceneQueryRequest>()
                ->Field("MaxResults", &SceneQueryRequest::m_maxResults)
                ->Field("CollisionGroup", &SceneQueryRequest::m_collisionGroup)
                ->Field("QueryType", &SceneQueryRequest::m_queryType)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SceneQueryRequest>("Scene Query Request", "Parameters for scene queries")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SceneQueryRequest::m_collisionGroup, "Collision Group", "The layers to include in the query")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &Internal::PopulateCollisionGroups)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SceneQueryRequest::m_queryType, "Query Type", "Object types to include in the query")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SceneQueryRequest::m_maxResults, "Max results", "The Maximum results for this request to return, this is limited by the value set in Physics Configuration")
                    ;
            }
        }

        if (auto* behaviorContext = azdynamic_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SceneQueryRequest>("SceneQueryRequest")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "Physics")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Property("Collision", BehaviorValueProperty(&SceneQueryRequest::m_collisionGroup))
                // Until enum class support for behavior context is done, expose this as an int
                ->Property("QueryType", [](const SceneQueryRequest& self) { return static_cast<int>(self.m_queryType); },
                    [](SceneQueryRequest& self, int newQueryType) { self.m_queryType = SceneQuery::QueryType(newQueryType); })
                ;
        }
    }

    /*static*/ void RayCastRequest::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RayCastRequest, SceneQueryRequest>()
                ->Field("Distance", &RayCastRequest::m_distance)
                ->Field("Start", &RayCastRequest::m_start)
                ->Field("Direction", &RayCastRequest::m_direction)
                ->Field("HitFlags", &RayCastRequest::m_hitFlags)
                ->Field("ReportMultipleHits", &RayCastRequest::m_reportMultipleHits)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<RayCastRequest>("RayCast Request", "Parameters for raycast")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RayCastRequest::m_start, "Start", "Start position of the raycast")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RayCastRequest::m_distance, "Distance", "Length of the raycast")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RayCastRequest::m_direction, "Direction", "Direction of the raycast")
                    ;
            }
        }

        if (auto* behaviorContext = azdynamic_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RayCastRequest>("RayCastRequest")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "Physics")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Property("Distance", BehaviorValueProperty(&RayCastRequest::m_distance))
                ->Property("Start", BehaviorValueProperty(&RayCastRequest::m_start))
                ->Property("Direction", BehaviorValueProperty(&RayCastRequest::m_direction))
                ->Property("ReportMultipleHits", BehaviorValueProperty(&RayCastRequest::m_reportMultipleHits))
                ;

            behaviorContext->Class<SceneQueries>("SceneQueries")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "Physics")
                ->Method(
                    "CreateRayCastRequest",
                    [](const AZ::Vector3& start, const AZ::Vector3& direction, float distance, const AZStd::string& collisionGroup)
                    {
                        RayCastRequest request;
                        request.m_start = start;
                        request.m_direction = direction;
                        request.m_distance = distance;
                        request.m_collisionGroup = CollisionGroup(collisionGroup);
                        return request;
                    });
        }
    }

    /*static*/ void ShapeCastRequest::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ShapeCastRequest, SceneQueryRequest>()
                ->Field("Distance", &ShapeCastRequest::m_distance)
                ->Field("Start", &ShapeCastRequest::m_start)
                ->Field("Direction", &ShapeCastRequest::m_direction)
                ->Field("ShapeConfiguration", &ShapeCastRequest::m_shapeConfiguration)
                ->Field("HitFlags", &ShapeCastRequest::m_hitFlags)
                ->Field("ReportMultipleHits", &ShapeCastRequest::m_reportMultipleHits)
                ;
        }

        if (auto* behaviorContext = azdynamic_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ShapeCastRequest>("ShapeCastRequest")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "Physics")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Property("Distance", BehaviorValueProperty(&ShapeCastRequest::m_distance))
                ->Property("Start", BehaviorValueProperty(&ShapeCastRequest::m_start))
                ->Property("Direction", BehaviorValueProperty(&ShapeCastRequest::m_direction))
                ;

            behaviorContext->Method(
                "CreateSphereCastRequest",
                [](float radius, const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
                   SceneQuery::QueryType queryType, CollisionGroup collisionGroup)
                {
                    return ShapeCastRequestHelpers::CreateSphereCastRequest(
                        radius, startPose, direction, distance, queryType, collisionGroup, nullptr);
                });
            behaviorContext->Method(
                "CreateBoxCastRequest",
                [](const AZ::Vector3& boxDimensions, const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
                   SceneQuery::QueryType queryType, CollisionGroup collisionGroup)
                {
                    return ShapeCastRequestHelpers::CreateBoxCastRequest(
                        boxDimensions, startPose, direction, distance, queryType, collisionGroup, nullptr);
                });
            behaviorContext->Method(
                "CreateCapsuleCastRequest",
                [](float capsuleRadius, float capsuleHeight, const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
                   SceneQuery::QueryType queryType, CollisionGroup collisionGroup)
                {
                    return ShapeCastRequestHelpers::CreateCapsuleCastRequest(
                        capsuleRadius, capsuleHeight, startPose, direction, distance, queryType, collisionGroup, nullptr);
                });
        }
    }

    namespace ShapeCastRequestHelpers
    {
        ShapeCastRequest CreateSphereCastRequest(float radius,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
            SceneQuery::QueryType queryType /*= SceneQuery::QueryType::StaticAndDynamic*/,
            CollisionGroup collisionGroup /*= CollisionGroup::All*/,
            SceneQuery::FilterCallback filterCallback /*= nullptr*/)
        {
            AzPhysics::ShapeCastRequest request;
            request.m_distance = distance;
            request.m_start = startPose;
            request.m_direction = direction;
            request.m_shapeConfiguration = AZStd::make_shared<Physics::SphereShapeConfiguration>(radius);
            request.m_queryType = queryType;
            request.m_collisionGroup = collisionGroup;
            request.m_filterCallback = filterCallback;
            return request;
        }

        ShapeCastRequest CreateBoxCastRequest(const AZ::Vector3& boxDimensions,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
            SceneQuery::QueryType queryType /*= SceneQuery::QueryType::StaticAndDynamic*/,
            CollisionGroup collisionGroup /*= CollisionGroup::All*/,
            SceneQuery::FilterCallback filterCallback /*= nullptr*/)
        {
            AzPhysics::ShapeCastRequest request;
            request.m_distance = distance;
            request.m_start = startPose;
            request.m_direction = direction;
            request.m_shapeConfiguration = AZStd::make_shared<Physics::BoxShapeConfiguration>(boxDimensions);
            request.m_queryType = queryType;
            request.m_collisionGroup = collisionGroup;
            request.m_filterCallback = filterCallback;
            return request;
        }

        ShapeCastRequest CreateCapsuleCastRequest(float capsuleRadius, float capsuleHeight,
            const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
            SceneQuery::QueryType queryType /*= SceneQuery::QueryType::StaticAndDynamic*/,
            CollisionGroup collisionGroup /*= CollisionGroup::All*/,
            SceneQuery::FilterCallback filterCallback /*= nullptr*/)
        {
            AzPhysics::ShapeCastRequest request;
            request.m_distance = distance;
            request.m_start = startPose;
            request.m_direction = direction;
            request.m_shapeConfiguration = AZStd::make_shared<Physics::CapsuleShapeConfiguration>(capsuleHeight, capsuleRadius);
            request.m_queryType = queryType;
            request.m_collisionGroup = collisionGroup;
            request.m_filterCallback = filterCallback;
            return request;
        }

    } // namespace ShapeCastRequestHelpers

    /*static*/ void OverlapRequest::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<OverlapRequest, SceneQueryRequest>()
                ->Field("Pose", &OverlapRequest::m_pose)
                ->Field("ShapeConfiguration", &OverlapRequest::m_shapeConfiguration)
                ;
        }

        if (auto* behaviorContext = azdynamic_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<OverlapRequest>("OverlapRequest")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "Physics")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Property("Pose", BehaviorValueProperty(&OverlapRequest::m_pose))
                ;

            behaviorContext->Method(
                "CreateSphereOverlapRequest",
                [](float radius, const AZ::Transform& pose)
                {
                    return OverlapRequestHelpers::CreateSphereOverlapRequest(
                        radius, pose, nullptr);
                });

            behaviorContext->Method(
                "CreateBoxOverlapRequest",
                [](const AZ::Vector3& boxDimensions, const AZ::Transform& pose)
                {
                    return OverlapRequestHelpers::CreateBoxOverlapRequest(
                        boxDimensions, pose, nullptr);
                });

            behaviorContext->Method(
                "CreateCapsuleOverlapRequest",
                [](float height, float radius, const AZ::Transform& pose)
                {
                    return OverlapRequestHelpers::CreateCapsuleOverlapRequest(
                        height, radius, pose, nullptr);
                });
        }
    }

    namespace OverlapRequestHelpers
    {
        OverlapRequest CreateSphereOverlapRequest(float radius, const AZ::Transform& pose,
            SceneQuery::OverlapFilterCallback filterCallback /*= nullptr*/)
        {
            AzPhysics::OverlapRequest overlapRequest;
            overlapRequest.m_pose = pose;
            overlapRequest.m_shapeConfiguration = AZStd::make_shared<Physics::SphereShapeConfiguration>(radius);
            overlapRequest.m_filterCallback = filterCallback;
            return overlapRequest;
        }

        OverlapRequest CreateBoxOverlapRequest(const AZ::Vector3& dimensions, const AZ::Transform& pose,
            SceneQuery::OverlapFilterCallback filterCallback /*= nullptr*/)
        {
            AzPhysics::OverlapRequest overlapRequest;
            overlapRequest.m_pose = pose;
            overlapRequest.m_shapeConfiguration = AZStd::make_shared<Physics::BoxShapeConfiguration>(dimensions);
            overlapRequest.m_filterCallback = filterCallback;
            return overlapRequest;
        }

        OverlapRequest CreateCapsuleOverlapRequest(float height, float radius, const AZ::Transform& pose,
            SceneQuery::OverlapFilterCallback filterCallback /*= nullptr*/)
        {
            AzPhysics::OverlapRequest overlapRequest;
            overlapRequest.m_pose = pose;
            overlapRequest.m_shapeConfiguration = AZStd::make_shared<Physics::CapsuleShapeConfiguration>(height, radius);
            overlapRequest.m_filterCallback = filterCallback;
            return overlapRequest;
        }

    } // namespace OverlapRequestHelpers

    /*static*/ void SceneQueryHit::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SceneQueryHit>()
                ->Field("Distance", &SceneQueryHit::m_distance)
                ->Field("Position", &SceneQueryHit::m_position)
                ->Field("Normal", &SceneQueryHit::m_normal)
                ->Field("BodyHandle", &SceneQueryHit::m_bodyHandle)
                ->Field("EntityId", &SceneQueryHit::m_entityId)
                ;
        }

        if (auto* behaviorContext = azdynamic_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SceneQueryHit>("SceneQueryHit")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "Physics")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Property("Distance", BehaviorValueProperty(&SceneQueryHit::m_distance))
                ->Property("Position", BehaviorValueProperty(&SceneQueryHit::m_position))
                ->Property("Normal", BehaviorValueProperty(&SceneQueryHit::m_normal))
                ->Property("BodyHandle", BehaviorValueProperty(&SceneQueryHit::m_bodyHandle))
                ->Property("EntityId", BehaviorValueProperty(&SceneQueryHit::m_entityId))
                ;
        }
    }

    /*static*/ void SceneQueryHits::Reflect(AZ::ReflectContext* context)
    {
        SceneQueryHit::Reflect(context);

        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SceneQueryHits>()
                ->Field("HitArray", &SceneQueryHits::m_hits)
                ;
        }

        if (auto* behaviorContext = azdynamic_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SceneQueryHits>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "Physics")
                ->Property("HitArray", BehaviorValueGetter(&SceneQueryHits::m_hits), nullptr)
                ;
        }
    }

} // namespace Physics
