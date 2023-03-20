/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/EntityId.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>

namespace Vegetation
{
    /**
    * Defines configuration used to create a vegetation instance
    */
    struct InstanceData final
    {
        AZ_RTTI(InstanceData, "{1DD3D37D-0855-44F9-94F8-76F0128491A1}");
        AZ_CLASS_ALLOCATOR(InstanceData, AZ::SystemAllocator);

        AZ::EntityId m_id;
        InstanceId m_instanceId = InvalidInstanceId;
        AZ::u32 m_changeIndex = {};
        AZ::Vector3 m_position = {};
        AZ::Vector3 m_normal = AZ::Vector3::CreateAxisZ();
        AZ::Quaternion m_rotation = AZ::Quaternion::CreateIdentity();
        AZ::Quaternion m_alignment = AZ::Quaternion::CreateIdentity();
        float m_scale = 1.0f;
        SurfaceData::SurfaceTagWeights m_masks; //[LY-90908] remove when surface mask filtering is done in area
        DescriptorPtr m_descriptorPtr;

        // Determine if two different sets of instance data are similar enough to be considered the same when placing
        // new instances.
        static bool IsSameInstanceData(const InstanceData& lhs, const InstanceData& rhs)
        {
            return
                lhs.m_id == rhs.m_id &&
                lhs.m_position.IsClose(rhs.m_position) &&
                lhs.m_rotation.IsClose(rhs.m_rotation) &&
                lhs.m_alignment.IsClose(rhs.m_alignment) &&
                lhs.m_scale == rhs.m_scale &&
                lhs.m_descriptorPtr == rhs.m_descriptorPtr;
        }

        static void Reflect(AZ::ReflectContext* context);
    };

} // namespace Vegetation
