/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/base.h>

struct dtCrowdAgentParams;

namespace RecastNavigation
{
    //! Serializable/editable representation of Detour's dtCrowdAgentParams.
    class DetourCrowdAgentParams final
    {
    public:
        AZ_CLASS_ALLOCATOR(DetourCrowdAgentParams, AZ::SystemAllocator);
        AZ_TYPE_INFO(DetourCrowdAgentParams, "{626D6EFE-4E5D-4028-BF41-EEC95342D79A}");

        static void Reflect(AZ::ReflectContext* context);

        dtCrowdAgentParams ToDetourCrowdAgentParams() const;

        float m_radius = 0.6f;
        float m_height = 2.0f;
        float m_maxAcceleration = 8.0f;
        float m_maxSpeed = 3.5f;
        float m_collisionQueryRange = 7.2f;
        float m_pathOptimizationRange = 18.0f;
        float m_separationWeight = 2.0f;
        bool m_anticipateTurns = false;
        bool m_obstacleAvoidance = false;
        bool m_separation = false;
        bool m_optimizeVisibility = false;
        bool m_optimizeTopology = false;
        AZ::u8 m_obstacleAvoidanceType = 0;
        AZ::u8 m_queryFilterType = 0;
    };
} // namespace RecastNavigation
