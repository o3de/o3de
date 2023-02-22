/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>

#include <Vegetation/Descriptor.h>

namespace Vegetation
{
    /**
    * Contains an ordered list of vegetation descriptors used to create instances
    */
    class DescriptorListAsset final
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(DescriptorListAsset, "{60961B36-E3CA-4877-B197-1462C1363F6E}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(DescriptorListAsset, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);
    
        AZStd::vector<Descriptor> m_descriptors;
    };

} // namespace Vegetation
