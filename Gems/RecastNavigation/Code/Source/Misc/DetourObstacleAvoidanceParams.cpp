/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RecastNavigation/DetourObstacleAvoidanceParams.h>

#include <AzCore/RTTI/BehaviorContext.h>
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
        using Self = DetourObstacleAvoidanceParams;

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<Self>()
                ->Version(1)
                ->Field("Vel Bias", &Self::m_velBias)
                ->Field("Weight Desired Velocity", &Self::m_weightDesVel)
                ->Field("Weight Current Velocity", &Self::m_weightCurVel)
                ->Field("Weight Side", &Self::m_weightSide)
                ->Field("Weight Time Of Impact", &Self::m_weightToi)
                ->Field("Horizon Time", &Self::m_horizTime)
                ->Field("Grid Size", &Self::m_gridSize)
                ->Field("Adaptive Divisions", &Self::m_adaptiveDivs)
                ->Field("Adaptive Rings", &Self::m_adaptiveRings)
                ->Field("Adaptive Depth", &Self::m_adaptiveDepth);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<Self>("Detour Obstacle Avoidance Params", "Configuration parameters for obstacle avoidance.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_velBias, "Vel Bias", "Velocity bias.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &Self::m_weightDesVel, "Weight Desired Velocity", "Desired velocity weight.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &Self::m_weightCurVel, "Weight Current Velocity", "Current velocity weight.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_weightSide, "Weight Side", "Preferred side weight.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_weightToi, "Weight Time Of Impact", "Collision time weight.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_horizTime, "Horizon Time", "Avoidance horizon time.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_gridSize, "Grid Size", "Sampling grid size.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_adaptiveDivs, "Adaptive Divisions", "Adaptive divisions count.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_adaptiveRings, "Adaptive Rings", "Adaptive rings count.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Self::m_adaptiveDepth, "Adaptive Depth", "Adaptive depth count.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1);
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<Self>("DetourObstacleAvoidanceParams")
                ->Constructor<>()
                ->Property("velBias", BehaviorValueProperty(&Self::m_velBias))
                ->Property("weightDesVel", BehaviorValueProperty(&Self::m_weightDesVel))
                ->Property("weightCurVel", BehaviorValueProperty(&Self::m_weightCurVel))
                ->Property("weightSide", BehaviorValueProperty(&Self::m_weightSide))
                ->Property("weightToi", BehaviorValueProperty(&Self::m_weightToi))
                ->Property("horizTime", BehaviorValueProperty(&Self::m_horizTime))
                ->Property("gridSize", BehaviorValueProperty(&Self::m_gridSize))
                ->Property("adaptiveDivs", BehaviorValueProperty(&Self::m_adaptiveDivs))
                ->Property("adaptiveRings", BehaviorValueProperty(&Self::m_adaptiveRings))
                ->Property("adaptiveDepth", BehaviorValueProperty(&Self::m_adaptiveDepth));
        }
    }
} // namespace RecastNavigation
