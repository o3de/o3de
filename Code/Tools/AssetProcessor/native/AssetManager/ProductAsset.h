/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetDatabase/AssetDatabaseConnection.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>
#include <native/utilities/assetUtils.h>

namespace AssetProcessor
{
    //! Represents a single product asset file, either in the cache or the intermediate directory
    class ProductAsset
    {
    public:
        ProductAsset(AZ::IO::Path absolutePath);

        bool IsValid() const;
        bool ExistsOnDisk(bool printErrorMessage) const;
        bool DeleteFile(bool sendNotification) const;
        AZ::u64 ComputeHash() const;

    protected:
        const AZ::IO::Path m_absolutePath;
    };

    //! Represents a single job output, which itself can be either a cache product, intermediate product, or both
    class ProductAssetWrapper
    {
    public:
        ProductAssetWrapper(const AssetBuilderSDK::JobProduct& jobProduct, const AssetUtilities::ProductPath& productPath);
        ProductAssetWrapper(const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product, const AssetUtilities::ProductPath& productPath);

        bool IsValid() const;
        bool ExistOnDisk(bool printErrorMessage) const;
        bool DeleteFiles(bool sendNotification) const;
        AZ::u64 ComputeHash() const;
        bool HasCacheProduct() const;
        bool HasIntermediateProduct() const;

    protected:
        AZStd::fixed_vector<AZStd::unique_ptr<ProductAsset>, 2> m_products;
        bool m_cacheProduct = false;
        bool m_intermediateProduct = false;
    };
}
