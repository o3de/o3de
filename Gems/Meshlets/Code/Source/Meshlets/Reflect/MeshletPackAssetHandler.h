/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/std/parallel/atomic.h>
#include <Meshlets/Reflect/MeshletPackAsset.h>

namespace AZ::Meshlets
{
    //! AssetHandler subclass for MeshletPackAsset. Loads the .azmeshletpack
    //! byte stream into a MeshletPackAsset's internal buffer; the asset's
    //! parser validates structure (magic, version, bounds).
    class MeshletPackAssetHandler : public AZ::Data::AssetHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(MeshletPackAssetHandler, AZ::SystemAllocator);

        MeshletPackAssetHandler();
        ~MeshletPackAssetHandler() override;

        // AssetHandler overrides...
        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id,
                                       const AZ::Data::AssetType& type) override;
        void DestroyAsset(AZ::Data::AssetPtr ptr) override;
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& types) override;
        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& filterCB) override;

        void Register();
        void Unregister();

    private:
        bool m_registered = false;
    };

} // namespace AZ::Meshlets
