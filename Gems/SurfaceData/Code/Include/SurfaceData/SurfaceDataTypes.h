/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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