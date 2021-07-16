/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_set.h>
#include <SurfaceData/SurfaceTag.h>

namespace SurfaceData
{
    //map of id or crc to contribution factor
    using SurfaceTagWeightMap = AZStd::unordered_map<AZ::Crc32, float>;
    using SurfaceTagNameSet = AZStd::unordered_set<AZStd::string>;
    using SurfaceTagVector = AZStd::vector<SurfaceTag>;

    struct SurfacePoint final
    {
        AZ_CLASS_ALLOCATOR(SurfacePoint, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(SurfacePoint, "{0DC7E720-68D6-47D4-BB6D-B89EF23C5A5C}");

        AZ::EntityId m_entityId;
        AZ::Vector3 m_position;
        AZ::Vector3 m_normal;
        SurfaceTagWeightMap m_masks;
    };

    using SurfacePointList = AZStd::vector<SurfacePoint>;
    using SurfacePointListPerPosition = AZStd::vector<AZStd::pair<AZ::Vector3, SurfacePointList>>;

    struct SurfaceDataRegistryEntry
    {
        AZ::EntityId m_entityId;
        AZ::Aabb m_bounds = AZ::Aabb::CreateNull();
        SurfaceTagVector m_tags;
    };

    using SurfaceDataRegistryHandle = AZ::u32;
    const SurfaceDataRegistryHandle InvalidSurfaceDataRegistryHandle = 0;
}
