/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StdAfx.h"

#include <Asset/BlastSliceAsset.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace Blast
{
    void BlastSliceAsset::SetMeshIdList(const AZStd::vector<AZ::Data::AssetId>& meshAssetIdList)
    {
        m_meshAssetIdList = meshAssetIdList;
    }

    const AZStd::vector<AZ::Data::AssetId>& BlastSliceAsset::GetMeshIdList() const
    {
        return m_meshAssetIdList;
    }

    void BlastSliceAsset::SetMaterialId(const AZ::Data::AssetId& materialAssetId)
    {
        m_materialAssetId = materialAssetId;
    }

    const AZ::Data::AssetId& BlastSliceAsset::GetMaterialId() const
    {
        return m_materialAssetId;
    }

    void BlastSliceAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BlastSliceAsset, AZ::Data::AssetData>()
                ->Version(1)
                ->Field("meshAssetIdList", &BlastSliceAsset::m_meshAssetIdList)
                ->Field("materialAssetId", &BlastSliceAsset::m_materialAssetId);
        }

        if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->Class<BlastSliceAsset>("BlastSliceAsset")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "blast")
                ->Method("SetMeshIdList", &BlastSliceAsset::SetMeshIdList)
                ->Method("GetMeshIdList", &BlastSliceAsset::GetMeshIdList)
                ->Method("SetMaterialId", &BlastSliceAsset::SetMaterialId)
                ->Method("GetMaterialId", &BlastSliceAsset::GetMaterialId)
                ->Method(
                    "GetAssetTypeId",
                    [](BlastSliceAsset*)
                    {
                        return azrtti_typeid<BlastSliceAsset>();
                    });
        }
    }

} // namespace Blast
