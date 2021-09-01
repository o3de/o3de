/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>

namespace AZ::Prefab
{
    class ProceduralPrefabAsset
        : public Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(ProceduralPrefabAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(ProceduralPrefabAsset, "{9B7C8459-471E-4EAD-A363-7990CC4065A9}", Data::AssetData);

        ProceduralPrefabAsset(const Data::AssetId& assetId = Data::AssetId());
        ~ProceduralPrefabAsset() override = default;
        ProceduralPrefabAsset(const ProceduralPrefabAsset& rhs) = delete;
        ProceduralPrefabAsset& operator=(const ProceduralPrefabAsset& rhs) = delete;

    private:
        AZStd::string m_templateName;
        AzToolsFramework::Prefab::PrefabDom m_prefabDom;
    };
}
