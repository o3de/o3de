/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

namespace EMotionFX
{
    struct RootMotionExtractionData
    {
        enum class SmoothingMethod : AZ::u8
        {
            None = 0,
            MovingAverage = 1
        };

        AZ_RTTI(EMotionFX::Pipeline::Rule::RootMotionExtractionData, "{7AA82E47-88CC-4430-9AEE-83BFB671D286}");
        AZ_CLASS_ALLOCATOR(RootMotionExtractionData, AZ::SystemAllocator, 0)

        virtual ~RootMotionExtractionData() = default;
        static void Reflect(AZ::ReflectContext* context);

        bool m_transitionZeroXAxis = false;
        bool m_transitionZeroYAxis = false;
        bool m_extractRotation = false;
        SmoothingMethod m_smoothingMethod = SmoothingMethod::None;
        AZStd::string m_sampleJoint = "Hip";
    };
}

