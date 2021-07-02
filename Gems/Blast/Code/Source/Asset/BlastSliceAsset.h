/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
