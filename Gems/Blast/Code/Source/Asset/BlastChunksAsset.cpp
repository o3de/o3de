/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Asset/BlastChunksAsset.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Blast
{
    void BlastChunksAsset::SetModelAssetIds(const AZStd::vector<AZ::Data::AssetId>& modelAssetIds)
    {
        m_modelAssetIds = modelAssetIds;
    }

    const AZStd::vector<AZ::Data::AssetId>& BlastChunksAsset::GetModelAssetIds() const
    {
        return m_modelAssetIds;
    }

    void BlastChunksAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BlastChunksAsset, AZ::Data::AssetData>()
                ->Version(1)
                ->Field("modelAssetIds", &BlastChunksAsset::m_modelAssetIds);
        }
    }

} // namespace Blast
