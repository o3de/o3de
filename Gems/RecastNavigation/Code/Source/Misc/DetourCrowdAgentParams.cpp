/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RecastNavigation/DetourCrowdAgentParams.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <DetourCrowd.h>

namespace RecastNavigation
{
    dtCrowdAgentParams DetourCrowdAgentParams::ToDetourCrowdAgentParams() const
    {
        dtCrowdAgentParams output{};
        output.radius = m_radius;
        output.height = m_height;
        output.maxAcceleration = m_maxAcceleration;
        output.maxSpeed = m_maxSpeed;
        output.collisionQueryRange = m_collisionQueryRange;
        output.pathOptimizationRange = m_pathOptimizationRange;
        output.separationWeight = m_separationWeight;

        output.updateFlags = 0;
        if (m_anticipateTurns)
        {
            output.updateFlags |= DT_CROWD_ANTICIPATE_TURNS;
        }
        if (m_obstacleAvoidance)
        {
            output.updateFlags |= DT_CROWD_OBSTACLE_AVOIDANCE;
        }
        if (m_separation)
        {
            output.updateFlags |= DT_CROWD_SEPARATION;
        }
        if (m_optimizeVisibility)
        {
            output.updateFlags |= DT_CROWD_OPTIMIZE_VIS;
        }
        if (m_optimizeTopology)
        {
            output.updateFlags |= DT_CROWD_OPTIMIZE_TOPO;
        }

        output.obstacleAvoidanceType = m_obstacleAvoidanceType;
        output.queryFilterType = m_queryFilterType;
        output.userData = nullptr;

        return output;
    }

    void DetourCrowdAgentParams::Reflect(AZ::ReflectContext* context)
    {
        using Self = DetourCrowdAgentParams;

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<Self>()
                ->Version(1)
                ->Field("Radius", &Self::m_radius)
                ->Field("Height", &Self::m_height)
                ->Field("Max Acceleration", &Self::m_maxAcceleration)
                ->Field("Max Speed", &Self::m_maxSpeed)
                ->Field("Collision Query Range", &Self::m_collisionQueryRange)
                ->Field("Path Optimization Range", &Self::m_pathOptimizationRange)
                ->Field("Separation Weight", &Self::m_separationWeight)
                ->Field("Anticipate Turns", &Self::m_anticipateTurns)
                ->Field("Obstacle Avoidance", &Self::m_obstacleAvoidance)
                ->Field("Separation", &Self::m_separation)
                ->Field("Optimize Visibility", &Self::m_optimizeVisibility)
                ->Field("Optimize Topology", &Self::m_optimizeTopology)
                ->Field("Obstacle Avoidance Type", &Self::m_obstacleAvoidanceType)
                ->Field("Query Filter Type", &Self::m_queryFilterType);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<Self>("Detour Crowd Agent Params", "Configuration parameters for a crowd agent.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_radius, "Radius", "Agent radius. [Limit: >= 0]")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_height, "Height", "Agent height. [Limit: > 0]")
                    ->Attribute(AZ::Edit::Attributes::Min, AZ::Constants::FloatEpsilon)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &Self::m_maxAcceleration,
                        "Max Acceleration",
                        "Maximum allowed acceleration. [Limit: >= 0]")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_maxSpeed, "Max Speed", "Maximum allowed speed. [Limit: >= 0]")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &Self::m_collisionQueryRange,
                        "Collision Query Range",
                        "How close a collision element must be before being considered for steering. [Limit: > 0]")
                    ->Attribute(AZ::Edit::Attributes::Min, AZ::Constants::FloatEpsilon)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &Self::m_pathOptimizationRange,
                        "Path Optimization Range",
                        "Path visibility optimization range. [Limit: > 0]")
                    ->Attribute(AZ::Edit::Attributes::Min, AZ::Constants::FloatEpsilon)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &Self::m_separationWeight,
                        "Separation Weight",
                        "How aggressively other agents avoid this agent. [Limit: >= 0]")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &Self::m_anticipateTurns, "Anticipate Turns", "Enable DT_CROWD_ANTICIPATE_TURNS.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &Self::m_obstacleAvoidance,
                        "Obstacle Avoidance",
                        "Enable DT_CROWD_OBSTACLE_AVOIDANCE.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_separation, "Separation", "Enable DT_CROWD_SEPARATION.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &Self::m_optimizeVisibility, "Optimize Visibility", "Enable DT_CROWD_OPTIMIZE_VIS.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &Self::m_optimizeTopology, "Optimize Topology", "Enable DT_CROWD_OPTIMIZE_TOPO.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &Self::m_obstacleAvoidanceType,
                        "Obstacle Avoidance Type",
                        "Index of the avoidance configuration.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, DT_CROWD_MAX_OBSTAVOIDANCE_PARAMS)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &Self::m_queryFilterType,
                        "Query Filter Type",
                        "Index of the query filter used by this agent.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, DT_CROWD_MAX_QUERY_FILTER_TYPE);
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<Self>("DetourCrowdAgentParams")
                ->Constructor<>()
                ->Property("radius", BehaviorValueProperty(&Self::m_radius))
                ->Property("height", BehaviorValueProperty(&Self::m_height))
                ->Property("maxAcceleration", BehaviorValueProperty(&Self::m_maxAcceleration))
                ->Property("maxSpeed", BehaviorValueProperty(&Self::m_maxSpeed))
                ->Property("collisionQueryRange", BehaviorValueProperty(&Self::m_collisionQueryRange))
                ->Property("pathOptimizationRange", BehaviorValueProperty(&Self::m_pathOptimizationRange))
                ->Property("separationWeight", BehaviorValueProperty(&Self::m_separationWeight))
                ->Property("anticipateTurns", BehaviorValueProperty(&Self::m_anticipateTurns))
                ->Property("obstacleAvoidance", BehaviorValueProperty(&Self::m_obstacleAvoidance))
                ->Property("separation", BehaviorValueProperty(&Self::m_separation))
                ->Property("optimizeVisibility", BehaviorValueProperty(&Self::m_optimizeVisibility))
                ->Property("optimizeTopology", BehaviorValueProperty(&Self::m_optimizeTopology))
                ->Property("obstacleAvoidanceType", BehaviorValueProperty(&Self::m_obstacleAvoidanceType))
                ->Property("queryFilterType", BehaviorValueProperty(&Self::m_queryFilterType));
        }
    }
} // namespace RecastNavigation
