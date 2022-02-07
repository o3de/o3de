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
    //! The product asset file from a .blast_chunks file product asset file
    class BlastChunksAsset final
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(BlastChunksAsset, "{993F0B0F-37D9-48C6-9CC2-E27D3F3E343E}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(BlastChunksAsset, AZ::SystemAllocator, 0);

        BlastChunksAsset() = default;
        ~BlastChunksAsset() override = default;

        void SetModelAssetIds(const AZStd::vector<AZ::Data::AssetId>& modelAssetIds);
        const AZStd::vector<AZ::Data::AssetId>& GetModelAssetIds() const;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::vector<AZ::Data::AssetId> m_modelAssetIds;
    };
} // namespace Blast
