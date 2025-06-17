/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <Atom/RPI.Edit/Configuration.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RPI.Reflect/ResourcePoolAsset.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {             
        // Enum type used to identify the type of resource pool source asset.
        enum class ResourcePoolAssetType : AZ::u8
        {
            BufferPool = 0,
            ImagePool,
            StreamingImagePool,
            Unknown,
        };
        
        // Source data of resource pool asset. It may generate different kind of production asset based on m_poolType
        struct ATOM_RPI_EDIT_API ResourcePoolSourceData
        {
            AZ_TYPE_INFO(ResourcePoolSourceData, "{8BFF0760-20E3-446D-9E3D-39D0266F7104}");
            AZ_CLASS_ALLOCATOR(ResourcePoolSourceData, AZ::SystemAllocator);
            
            static void Reflect(ReflectContext* context);

            ResourcePoolAssetType m_poolType = ResourcePoolAssetType::Unknown;
            AZStd::string m_poolName = "Unknown";
            AZ::u64 m_budgetInBytes = 0;

            // Configuration for buffer pool
            RHI::HeapMemoryLevel m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            RHI::HostMemoryAccess m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
            RHI::BufferBindFlags m_bufferPoolBindFlags = RHI::BufferBindFlags::None;

            // Configuration for image pool
            RHI::ImageBindFlags m_imagePoolBindFlags = RHI::ImageBindFlags::Color;
        };

    } // namespace RPI

    AZ_TYPE_INFO_SPECIALIZE(RPI::ResourcePoolAssetType, "{09D35821-2D42-456B-B53C-C02BF387C5C4}");
} // namespace AZ
