/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzFramework/AzFrameworkAPI.h>

namespace AzFramework
{
    struct AZF_API Curve
    {
        enum class CurveTickMode
        {
            EMIT_DURATION = 0,
            PARTICLE_LIFETIME,
            NORMALIZED_AGE,
            CUSTOM
        };
        enum class CurveExtrapMode
        {
            CYCLE = 0,
            CYCLE_WHIT_OFFSET,
            CONSTANT
        };

        enum class KeyPointInterpMode
        {
            LINEAR = 0,
            STEP,
            CUBIC_IN,
            CUBIC_OUT,
            SINE_IN,
            SINE_OUT,
            CIRCLE_IN,
            CIRCLE_OUT
        };

        struct KeyPoint
        {
            AZ_CLASS_ALLOCATOR(KeyPoint, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(KeyPoint, "{28F834C3-DA0C-4E0D-8D4E-BEDE1CE9D8FF}");

            static void Reflect(AZ::ReflectContext* context);
            float m_time = 0.0f;
            float m_value = 0.0f;
            KeyPointInterpMode m_interpMode = KeyPointInterpMode::LINEAR;
        };

        AZ_CLASS_ALLOCATOR(Curve, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(Curve, "{D4B7E91C-DD02-423E-B8BC-699E0DFC249B}");
        static void Reflect(AZ::ReflectContext* context);

        Curve();

        CurveExtrapMode m_leftExtrapMode = CurveExtrapMode::CYCLE;
        CurveExtrapMode m_rightExtrapMode = CurveExtrapMode::CYCLE;
        float m_valueFactor = 1.0f;
        float m_timeFactor = 1.0f;
        CurveTickMode m_tickMode = CurveTickMode::EMIT_DURATION;
        AZStd::vector<KeyPoint> m_keyPoints;
    };

}
