/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzCore/Serialization/EditContext.h>

namespace
{
    const float TimestepMin = 0.001f; //1000fps
    const float TimestepMax = 0.05f; //20fps
}

namespace Physics
{
    bool WorldConfiguration::VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        // conversion from version 1:
        // - remove AutoSimulate
        // - remove TerrainGroup
        // - remove TerrainLayer
        if (classElement.GetVersion() <= 1)
        {
            classElement.RemoveElementByName(AZ_CRC("AutoSimulate", 0xcce85fd9));
            classElement.RemoveElementByName(AZ_CRC("TerrainGroup", 0xca808c89));
            classElement.RemoveElementByName(AZ_CRC("TerrainLayer", 0x439be956));
        }

        // conversion from version 2:
        // - remove TerrainMaterials
        if (classElement.GetVersion() <= 2)
        {
            classElement.RemoveElementByName(AZ_CRC("TerrainMaterials", 0x6da24f86));
        }

        if (classElement.GetVersion() <= 3)
        {
            classElement.RemoveElementByName(AZ_CRC("HandleSimulationEvents", 0xba508787));
        }

        //clamping of time steps
        if (classElement.GetVersion() <= 4)
        {
            if (AZ::SerializeContext::DataElementNode* maxTimeStepElement = classElement.FindSubElement(AZ_CRC("MaxTimeStep", 0x34e83795)))
            {
                float maxTimeStep = TimestepMax;
                const bool foundMaxTimeStep = maxTimeStepElement->GetData<float>(maxTimeStep);
                if (foundMaxTimeStep)
                {
                    //clamp maxTimeStep between max and min
                    maxTimeStep = AZ::GetClamp(maxTimeStep, TimestepMin, TimestepMax);
                    maxTimeStepElement->SetData<float>(context, maxTimeStep);
                }

                if (AZ::SerializeContext::DataElementNode* fixedTimeStepElement = classElement.FindSubElement(AZ_CRC("FixedTimeStep", 0xd748ea77)))
                {
                    float fixedTimeStep = TimestepMax;
                    bool foundFixedTimeStep = fixedTimeStepElement->GetData<float>(fixedTimeStep);
                    if (foundFixedTimeStep)
                    {
                        //clamp fixedTimeStep between maxTimeStep and min
                        fixedTimeStep = AZ::GetClamp(fixedTimeStep, TimestepMin, maxTimeStep);
                        fixedTimeStepElement->SetData<float>(context, fixedTimeStep);
                    }
                }
            }
        }

        return true;
    }

    AZ::u32 WorldConfiguration::OnMaxTimeStepChanged()
    {
        m_fixedTimeStep = AZStd::GetMin(m_fixedTimeStep, GetFixedTimeStepMax()); //since m_maxTimeStep has changed, m_fixedTimeStep might be larger then the max.
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    float WorldConfiguration::GetFixedTimeStepMax() const
    {
        return m_maxTimeStep;
    }

    AZ::Crc32 WorldConfiguration::GetCcdVisibility() const
    {
        return m_enableCcd ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void WorldConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WorldConfiguration>()
                ->Version(5, &VersionConverter)
                ->Field("WorldBounds", &WorldConfiguration::m_worldBounds)
                ->Field("MaxTimeStep", &WorldConfiguration::m_maxTimeStep)
                ->Field("FixedTimeStep", &WorldConfiguration::m_fixedTimeStep)
                ->Field("Gravity", &WorldConfiguration::m_gravity)
                ->Field("RaycastBufferSize", &WorldConfiguration::m_raycastBufferSize)
                ->Field("SweepBufferSize", &WorldConfiguration::m_sweepBufferSize)
                ->Field("OverlapBufferSize", &WorldConfiguration::m_overlapBufferSize)
                ->Field("EnableCcd", &WorldConfiguration::m_enableCcd)
                ->Field("MaxCcdPasses", &WorldConfiguration::m_maxCcdPasses)
                ->Field("EnableCcdResweep", &WorldConfiguration::m_enableCcdResweep)
                ->Field("EnableActiveActors", &WorldConfiguration::m_enableActiveActors)
                ->Field("EnablePcm", &WorldConfiguration::m_enablePcm)
                ->Field("BounceThresholdVelocity", &WorldConfiguration::m_bounceThresholdVelocity)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<WorldConfiguration>("World Configuration", "Default world configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_worldBounds, "World Bounds", "World bounds")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_maxTimeStep, "Max Time Step (sec)", "Max time step in seconds")
                        ->Attribute(AZ::Edit::Attributes::Min, TimestepMin)
                        ->Attribute(AZ::Edit::Attributes::Max, TimestepMax)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WorldConfiguration::OnMaxTimeStepChanged)//need to clamp m_fixedTimeStep if this value changes
                        ->Attribute(AZ::Edit::Attributes::Decimals, 8)
                        ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 8)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_fixedTimeStep, "Fixed Time Step (sec)", "Fixed time step in seconds. Limited by 'Max Time Step'")
                        ->Attribute(AZ::Edit::Attributes::Min, TimestepMin)
                        ->Attribute(AZ::Edit::Attributes::Max, &WorldConfiguration::GetFixedTimeStepMax)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 8)
                        ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 8)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_gravity, "Gravity", "Gravity")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_raycastBufferSize, "Raycast Buffer Size", "Maximum number of hits from a raycast")
                        ->Attribute(AZ::Edit::Attributes::Min, 1u)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_sweepBufferSize, "Shapecast Buffer Size", "Maximum number of hits from a shapecast")
                        ->Attribute(AZ::Edit::Attributes::Min, 1u)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_overlapBufferSize, "Overlap Query Buffer Size", "Maximum number of hits from a overlap query")
                        ->Attribute(AZ::Edit::Attributes::Min, 1u)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Continuous Collision Detection")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_enableCcd, "Enable CCD", "Enabled continuous collision detection in the world")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_maxCcdPasses,
                        "Max CCD Passes", "Maximum number of continuous collision detection passes")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &WorldConfiguration::GetCcdVisibility)
                        ->Attribute(AZ::Edit::Attributes::Min, 1u)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_enableCcdResweep,
                        "Enable CCD Resweep", "Enable a more accurate but more expensive continuous collision detection method")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &WorldConfiguration::GetCcdVisibility)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "") // end previous group by starting new unnamed expanded group
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_enablePcm, "Persistent Contact Manifold", "Enabled the persistent contact manifold narrow-phase algorithm")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_bounceThresholdVelocity,
                        "Bounce Threshold Velocity", "Relative velocity below which colliding objects will not bounce")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ;
            }
        }
    }

    bool WorldConfiguration::operator==(const WorldConfiguration& other) const
    {
        constexpr const float timeStepTolerance = 0.0001f;
        return m_enableCcd == other.m_enableCcd
            && m_enableCcdResweep == other.m_enableCcdResweep
            && m_enableActiveActors == other.m_enableActiveActors
            && m_enablePcm == other.m_enablePcm
            && m_kinematicFiltering == other.m_kinematicFiltering
            && m_kinematicStaticFiltering == other.m_kinematicStaticFiltering
            && m_customUserData == other.m_customUserData
            && m_raycastBufferSize == other.m_raycastBufferSize
            && m_sweepBufferSize == other.m_sweepBufferSize
            && m_overlapBufferSize == other.m_overlapBufferSize
            && m_maxCcdPasses == other.m_maxCcdPasses
            && AZ::IsClose(m_maxTimeStep, other.m_maxTimeStep, timeStepTolerance)
            && AZ::IsClose(m_fixedTimeStep, other.m_fixedTimeStep, timeStepTolerance)
            && AZ::IsClose(m_bounceThresholdVelocity, other.m_bounceThresholdVelocity)
            && m_gravity.IsClose(other.m_gravity)
            && m_worldBounds == other.m_worldBounds
            ;
    }

    bool WorldConfiguration::operator!=(const WorldConfiguration& other) const
    {
        return !(*this == other);
    }

    AZStd::vector<OverlapHit> World::OverlapSphere(float radius, const AZ::Transform& pose,
        OverlapFilterCallback filterCallback)
    {
        SphereShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_radius = radius;

        OverlapRequest overlapRequest;
        overlapRequest.m_pose = pose;
        overlapRequest.m_shapeConfiguration = &shapeConfiguration;
        overlapRequest.m_filterCallback = filterCallback;
        return Overlap(overlapRequest);
    }

    AZStd::vector<OverlapHit> World::OverlapBox(const AZ::Vector3& dimensions, const AZ::Transform& pose,
        OverlapFilterCallback filterCallback)
    {
        BoxShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_dimensions = dimensions;

        OverlapRequest overlapRequest;
        overlapRequest.m_pose = pose;
        overlapRequest.m_shapeConfiguration = &shapeConfiguration;
        overlapRequest.m_filterCallback = filterCallback;
        return Overlap(overlapRequest);
    }

    AZStd::vector<OverlapHit> World::OverlapCapsule(float height, float radius, const AZ::Transform& pose,
        OverlapFilterCallback filterCallback)
    {
        CapsuleShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_height = height;
        shapeConfiguration.m_radius = radius;

        OverlapRequest overlapRequest;
        overlapRequest.m_pose = pose;
        overlapRequest.m_shapeConfiguration = &shapeConfiguration;
        overlapRequest.m_filterCallback = filterCallback;
        return Overlap(overlapRequest);
    }

    Physics::RayCastHit World::SphereCast(float radius, const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
        QueryType queryType, AzPhysics::CollisionGroup collisionGroup, FilterCallback filterCallback)
    {
        SphereShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_radius = radius;

        ShapeCastRequest request;
        request.m_distance = distance;
        request.m_start = startPose;
        request.m_direction = direction;
        request.m_shapeConfiguration = &shapeConfiguration;
        request.m_queryType = queryType;
        request.m_collisionGroup = collisionGroup;
        request.m_filterCallback = filterCallback;
        return ShapeCast(request);
    }

    AZStd::vector<Physics::RayCastHit> World::SphereCastMultiple(float radius, const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
        QueryType queryType, AzPhysics::CollisionGroup collisionGroup, FilterCallback filterCallback)
    {
        SphereShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_radius = radius;

        ShapeCastRequest request;
        request.m_distance = distance;
        request.m_start = startPose;
        request.m_direction = direction;
        request.m_shapeConfiguration = &shapeConfiguration;
        request.m_queryType = queryType;
        request.m_collisionGroup = collisionGroup;
        request.m_filterCallback = filterCallback;
        return ShapeCastMultiple(request);
    }

    Physics::RayCastHit World::BoxCast(const AZ::Vector3& boxDimensions, const AZ::Transform& startPose,
        const AZ::Vector3& direction, float distance, QueryType queryType, AzPhysics::CollisionGroup collisionGroup, FilterCallback filterCallback)
    {
        BoxShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_dimensions = boxDimensions;

        ShapeCastRequest request;
        request.m_distance = distance;
        request.m_start = startPose;
        request.m_direction = direction;
        request.m_shapeConfiguration = &shapeConfiguration;
        request.m_queryType = queryType;
        request.m_collisionGroup = collisionGroup;
        request.m_filterCallback = filterCallback;
        return ShapeCast(request);
    }

    AZStd::vector<Physics::RayCastHit> World::BoxCastMultiple(const AZ::Vector3& boxDimensions, const AZ::Transform& startPose,
        const AZ::Vector3& direction, float distance, QueryType queryType, AzPhysics::CollisionGroup collisionGroup, FilterCallback filterCallback)
    {
        BoxShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_dimensions = boxDimensions;

        ShapeCastRequest request;
        request.m_distance = distance;
        request.m_start = startPose;
        request.m_direction = direction;
        request.m_shapeConfiguration = &shapeConfiguration;
        request.m_queryType = queryType;
        request.m_collisionGroup = collisionGroup;
        request.m_filterCallback = filterCallback;
        return ShapeCastMultiple(request);
    }

    Physics::RayCastHit World::CapsuleCast(float capsuleRadius, float capsuleHeight, const AZ::Transform& startPose,
        const AZ::Vector3& direction, float distance, QueryType queryType, AzPhysics::CollisionGroup collisionGroup, FilterCallback filterCallback)
    {
        CapsuleShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_height = capsuleHeight;
        shapeConfiguration.m_radius = capsuleRadius;

        ShapeCastRequest request;
        request.m_distance = distance;
        request.m_start = startPose;
        request.m_direction = direction;
        request.m_shapeConfiguration = &shapeConfiguration;
        request.m_queryType = queryType;
        request.m_collisionGroup = collisionGroup;
        request.m_filterCallback = filterCallback;
        return ShapeCast(request);
    }

    AZStd::vector<Physics::RayCastHit> World::CapsuleCastMultiple(float capsuleRadius, float capsuleHeight, const AZ::Transform& startPose,
        const AZ::Vector3& direction, float distance, QueryType queryType, AzPhysics::CollisionGroup collisionGroup, FilterCallback filterCallback)
    {
        CapsuleShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_height = capsuleHeight;
        shapeConfiguration.m_radius = capsuleRadius;

        ShapeCastRequest request;
        request.m_distance = distance;
        request.m_start = startPose;
        request.m_direction = direction;
        request.m_shapeConfiguration = &shapeConfiguration;
        request.m_queryType = queryType;
        request.m_collisionGroup = collisionGroup;
        request.m_filterCallback = filterCallback;
        return ShapeCastMultiple(request);
    }
} // namespace Physics
