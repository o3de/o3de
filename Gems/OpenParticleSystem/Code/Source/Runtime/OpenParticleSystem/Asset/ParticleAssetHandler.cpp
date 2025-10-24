/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParticleAsset.h"
#include "ParticleAssetHandler.h"

namespace OpenParticle
{
    AZ::Data::AssetHandler::LoadResult ParticleAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        if (Base::LoadAssetData(asset, stream, assetLoadFilterCB) == AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }

        return AZ::Data::AssetHandler::LoadResult::Error;
    }
} // namespace OpenParticle
