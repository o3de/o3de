/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::Prefab
{
    class PrefabGroupAssetHandler final
        : public AZ::Data::AssetHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabGroupAssetHandler, AZ::SystemAllocator);
        PrefabGroupAssetHandler();
        ~PrefabGroupAssetHandler() override;

        static AZStd::string_view s_Extension;

    protected:
        AZ::Data::AssetData* CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;
        void DestroyAsset(AZ::Data::AssetData* ptr) override;
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;
        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;

        class AssetTypeInfoHandler;
        AZStd::shared_ptr<AssetTypeInfoHandler> m_assetTypeInfoHandler;
    };
}
