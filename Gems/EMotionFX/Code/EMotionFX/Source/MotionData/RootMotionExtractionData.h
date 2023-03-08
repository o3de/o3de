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
    class Actor;

    // Root motion extraction data is a collection of export settings used to extract hip/pelvis animation to root bone.
    class RootMotionExtractionData
    {
    public:
        enum class SmoothingMethod : AZ::u8
        {
            None = 0,
            MovingAverage = 1
        };

        AZ_RTTI(RootMotionExtractionData, "{7AA82E47-88CC-4430-9AEE-83BFB671D286}");
        AZ_CLASS_ALLOCATOR(RootMotionExtractionData, AZ::SystemAllocator)

        virtual ~RootMotionExtractionData() = default;
        static void Reflect(AZ::ReflectContext* context);
        AZ::Crc32 GetVisibilitySmoothEnabled() const;
        void FindBestMatchedJoints(const Actor* actor);

        bool m_transitionZeroXAxis = false;
        bool m_transitionZeroYAxis = false;
        bool m_extractRotation = false;
        SmoothingMethod m_smoothingMethod = SmoothingMethod::None;
        bool m_smoothPosition = true;
        bool m_smoothRotation = true;
        // For moving average smoothing, smoothFrameNumber decides how many frames from the given frame you are taking it to calculate the average.
        // e.g when smoothFrameNum is 1, it takes the previous 1 frame and 1 frame after and the given frame, total of 3 frames and divided it by 3.
        //     when smoothFrameNum is 2, it takes the previous 2 frames and 2 frames after, total of 5 frames and divided it by 5.
        size_t m_smoothFrameNum = 1;
        AZStd::string m_sampleJoint = "Hip";
    };
}

