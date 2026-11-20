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

struct dtObstacleAvoidanceParams;

namespace RecastNavigation
{
    //! Serializable/editable representation of Detour's dtObstacleAvoidanceParams.
    class DetourObstacleAvoidanceParams final
    {
    public:
        AZ_CLASS_ALLOCATOR(DetourObstacleAvoidanceParams, AZ::SystemAllocator);
        AZ_TYPE_INFO(DetourObstacleAvoidanceParams, "{7A6D9C3B-0D57-417C-92FB-E89148FB2F37}");

        static void Reflect(AZ::ReflectContext* context);

        static DetourObstacleAvoidanceParams CreateLowQuality();
        static DetourObstacleAvoidanceParams CreateMediumQuality();
        static DetourObstacleAvoidanceParams CreateGoodQuality();
        static DetourObstacleAvoidanceParams CreateHighQuality();

        dtObstacleAvoidanceParams ToDetourObstacleAvoidanceParams() const;

        float m_velBias = 0.4f;
        float m_weightDesVel = 2.0f;
        float m_weightCurVel = 0.75f;
        float m_weightSide = 0.75f;
        float m_weightToi = 2.5f;
        float m_horizTime = 2.5f;
        AZ::u8 m_gridSize = 33;
        AZ::u8 m_adaptiveDivs = 7;
        AZ::u8 m_adaptiveRings = 2;
        AZ::u8 m_adaptiveDepth = 5;
    };
} // namespace RecastNavigation
