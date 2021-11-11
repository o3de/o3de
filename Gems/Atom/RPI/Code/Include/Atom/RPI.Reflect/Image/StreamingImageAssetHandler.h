/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace AZ
{
    namespace RPI
    {
        //! The StreamingImageAsset's handler with customized loading steps in LoadAssetData override.
        class StreamingImageAssetHandler final
            : public AssetHandler<StreamingImageAsset>
        {
            using Base = AssetHandler<StreamingImageAsset>;
        public:
            Data::AssetHandler::LoadResult LoadAssetData(
                const Data::Asset<Data::AssetData>& asset,
                AZStd::shared_ptr<Data::AssetDataStream> stream,
                const Data::AssetFilterCB& assetLoadFilterCB) override;

            // Return a default fallback image if an asset is missing
            Data::AssetId AssetMissingInCatalog(const Data::Asset<Data::AssetData>& /*asset*/) override;
        };
    }
}
