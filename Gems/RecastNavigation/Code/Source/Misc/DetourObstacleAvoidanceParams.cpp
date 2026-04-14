/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RecastNavigation/DetourObstacleAvoidanceParams.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <DetourObstacleAvoidance.h>

namespace RecastNavigation
{
    dtObstacleAvoidanceParams DetourObstacleAvoidanceParams::ToDetourObstacleAvoidanceParams() const
    {
        dtObstacleAvoidanceParams output{};
        output.velBias = m_velBias;
        output.weightDesVel = m_weightDesVel;
        output.weightCurVel = m_weightCurVel;
        output.weightSide = m_weightSide;
        output.weightToi = m_weightToi;
        output.horizTime = m_horizTime;
        output.gridSize = m_gridSize;
        output.adaptiveDivs = m_adaptiveDivs;
        output.adaptiveRings = m_adaptiveRings;
        output.adaptiveDepth = m_adaptiveDepth;
        return output;
    }

    DetourObstacleAvoidanceParams DetourObstacleAvoidanceParams::CreateLowQuality()
    {
        DetourObstacleAvoidanceParams params;
        params.m_velBias = 0.5f;
        params.m_adaptiveDivs = 5;
        params.m_adaptiveRings = 2;
        params.m_adaptiveDepth = 1;
        return params;
    }

    DetourObstacleAvoidanceParams DetourObstacleAvoidanceParams::CreateMediumQuality()
    {
        DetourObstacleAvoidanceParams params;
        params.m_velBias = 0.5f;
        params.m_adaptiveDivs = 5;
        params.m_adaptiveRings = 2;
        params.m_adaptiveDepth = 2;
        return params;
    }

    DetourObstacleAvoidanceParams DetourObstacleAvoidanceParams::CreateGoodQuality()
    {
        DetourObstacleAvoidanceParams params;
        params.m_velBias = 0.5f;
        params.m_adaptiveDivs = 7;
        params.m_adaptiveRings = 2;
        params.m_adaptiveDepth = 3;
        return params;
    }

    DetourObstacleAvoidanceParams DetourObstacleAvoidanceParams::CreateHighQuality()
    {
        DetourObstacleAvoidanceParams params;
        params.m_velBias = 0.5f;
        params.m_adaptiveDivs = 7;
        params.m_adaptiveRings = 3;
        params.m_adaptiveDepth = 3;
        return params;
    }

    void DetourObstacleAvoidanceParams::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            using DetourObstacleAvoidanceParams = DetourObstacleAvoidanceParams;

            serialize->Class<DetourObstacleAvoidanceParams>()
                ->Version(1)
                ->Field("Vel Bias", &DetourObstacleAvoidanceParams::m_velBias)
                ->Field("Weight Desired Velocity", &DetourObstacleAvoidanceParams::m_weightDesVel)
                ->Field("Weight Current Velocity", &DetourObstacleAvoidanceParams::m_weightCurVel)
                ->Field("Weight Side", &DetourObstacleAvoidanceParams::m_weightSide)
                ->Field("Weight Time Of Impact", &DetourObstacleAvoidanceParams::m_weightToi)
                ->Field("Horizon Time", &DetourObstacleAvoidanceParams::m_horizTime)
                ->Field("Grid Size", &DetourObstacleAvoidanceParams::m_gridSize)
                ->Field("Adaptive Divisions", &DetourObstacleAvoidanceParams::m_adaptiveDivs)
                ->Field("Adaptive Rings", &DetourObstacleAvoidanceParams::m_adaptiveRings)
                ->Field("Adaptive Depth", &DetourObstacleAvoidanceParams::m_adaptiveDepth);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext
                    ->Class<DetourObstacleAvoidanceParams>(
                        "Detour Obstacle Avoidance Params", "Configuration parameters for obstacle avoidance.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DetourObstacleAvoidanceParams::m_velBias, "Vel Bias", "Velocity bias.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &DetourObstacleAvoidanceParams::m_weightDesVel,
                        "Weight Desired Velocity",
                        "Desired velocity weight.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &DetourObstacleAvoidanceParams::m_weightCurVel,
                        "Weight Current Velocity",
                        "Current velocity weight.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &DetourObstacleAvoidanceParams::m_weightSide,
                        "Weight Side",
                        "Preferred side weight.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &DetourObstacleAvoidanceParams::m_weightToi,
                        "Weight Time Of Impact",
                        "Collision time weight.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &DetourObstacleAvoidanceParams::m_horizTime,
                        "Horizon Time",
                        "Avoidance horizon time.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &DetourObstacleAvoidanceParams::m_gridSize, "Grid Size", "Sampling grid size.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &DetourObstacleAvoidanceParams::m_adaptiveDivs,
                        "Adaptive Divisions",
                        "Adaptive divisions count.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &DetourObstacleAvoidanceParams::m_adaptiveRings,
                        "Adaptive Rings",
                        "Adaptive rings count.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &DetourObstacleAvoidanceParams::m_adaptiveDepth,
                        "Adaptive Depth",
                        "Adaptive depth count.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1);
            }
        }
    }
} // namespace RecastNavigation
