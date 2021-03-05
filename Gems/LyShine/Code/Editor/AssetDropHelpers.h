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
#include "ComponentAssetHelpers.h"

class QMimeData;

//! Helpers for dragging and dropping assets from the asset browser to the UI Editor
namespace AssetDropHelpers
{
    using AssetList = AZStd::vector<AZ::Data::AssetId>;

    //! Returns whether mime type is accepted as an asset
    bool AcceptsMimeType(const QMimeData* mimeData);

    //! Returns whether mime data contains slice assets or assets that are associated with components
    bool DoesMimeDataContainSliceOrComponentAssets(const QMimeData* mimeData);

    //! Decode asset mime data and find the slice assets and the assets associated with components
    void DecodeSliceAndComponentAssetsFromMimeData(const QMimeData* mimeData, ComponentAssetHelpers::ComponentAssetPairs& componentAssetPairs, AssetList& sliceAssets);

    //! Decode asset mime data and find the UiCanvas assets
    void DecodeUiCanvasAssetsFromMimeData(const QMimeData* mimeData, AssetList& canvasAssets);
}   // namespace AssetDropHelpers
