/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        AZ_RTTI(PoseDataFactory, "{F10014A0-2B6A-44E5-BA53-0E11ED566701}")
        AZ_CLASS_ALLOCATOR_DECL

        PoseDataFactory();
        virtual ~PoseDataFactory() = default;

        static PoseData* Create(Pose* pose, const AZ::TypeId& type);

        void AddPoseDataType(const AZ::TypeId& poseDataType);
        const AZStd::unordered_set<AZ::TypeId>& GetTypeIds() const;

    private:
        AZStd::unordered_set<AZ::TypeId> m_poseDataTypeIds;
    };
} // namespace EMotionFX
