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
        AZ_CLASS_ALLOCATOR(DescriptorListAsset, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);
    
        AZStd::vector<Descriptor> m_descriptors;
    };

} // namespace Vegetation
