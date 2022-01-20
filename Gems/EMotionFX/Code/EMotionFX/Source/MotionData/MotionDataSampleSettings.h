/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/EMotionFXConfig.h>

namespace EMotionFX
{
    class ActorInstance;
    class Pose;

    struct EMFX_API MotionDataSampleSettings
    {
        const ActorInstance* m_actorInstance = nullptr;
        const Pose* m_inputPose = nullptr;
        float m_sampleTime = 0.0f;
        bool m_mirror = false;
        bool m_retarget = false;
        bool m_inPlace = false;
    };
} // namespace EMotionFX
