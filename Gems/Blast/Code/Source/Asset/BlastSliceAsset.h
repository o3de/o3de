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

namespace Blast
{
    //! The product asset file from a .blast_slice file product asset file
    class BlastSliceAsset final : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(BlastSliceAsset, "{D04AAF07-EB12-4E50-8964-114A9B9C1FD1}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(BlastSliceAsset, AZ::SystemAllocator, 0);

        BlastSliceAsset() = default;
        ~BlastSliceAsset() override = default;

        void SetMeshIdList(const AZStd::vector<AZ::Data::AssetId>& meshAssetIdList);
        const AZStd::vector<AZ::Data::AssetId>& GetMeshIdList() const;

        void SetMaterialId(const AZ::Data::AssetId& materialAssetId);
        const AZ::Data::AssetId& GetMaterialId() const;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::vector<AZ::Data::AssetId> m_meshAssetIdList;
        AZ::Data::AssetId m_materialAssetId;
    };
} // namespace Blast
