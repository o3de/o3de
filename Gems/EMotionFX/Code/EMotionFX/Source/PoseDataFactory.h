/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/unordered_set.h>
#include <EMotionFX/Source/EMotionFXConfig.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class Pose;
    class PoseData;

    class EMFX_API PoseDataFactory
    {
    public:
        static PoseData* Create(Pose* pose, const AZ::TypeId& type);
        static const AZStd::unordered_set<AZ::TypeId>& GetTypeIds();
    };
} // namespace EMotionFX
