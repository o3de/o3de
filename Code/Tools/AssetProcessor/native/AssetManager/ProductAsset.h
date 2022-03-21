/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Path/Path.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/IO/SystemFile.h>
#include <native/utilities/assetUtils.h>

namespace AssetProcessor
{
    //! Represents a single product asset file, either in the cache or the intermediate directory
    class ProductAsset
    {
    public:
        ProductAsset(const AssetBuilderSDK::JobProduct& jobProduct, AZ::IO::Path absolutePath);

        bool IsValid() const;
        bool ExistsOnDisk(bool printErrorMessage) const;
        bool DeleteFile() const;

    protected:
        const AssetBuilderSDK::JobProduct& m_product;
        const AZ::IO::Path m_absolutePath;
    };

    //! Represents a single job output, which itself can be either a cache product, intermediate product, or both
    class ProductAssetWrapper
    {
    public:
        ProductAssetWrapper(const AssetBuilderSDK::JobProduct& jobProduct, const AssetUtilities::ProductPath& productPath);

        bool IsValid() const;
        bool ExistOnDisk() const;
        bool DeleteFiles() const;

    protected:
        AZStd::fixed_vector<AZStd::unique_ptr<ProductAsset>, 2> m_products;
    };
}
