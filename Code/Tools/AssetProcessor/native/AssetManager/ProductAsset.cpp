/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/AssetManager/ProductAsset.h>

namespace AssetProcessor
{
    ProductAsset::ProductAsset(const AssetBuilderSDK::JobProduct& jobProduct, AZ::IO::Path absolutePath)
        : m_product(jobProduct)
        , m_absolutePath(AZStd::move(absolutePath))
    {
    }

    bool ProductAsset::IsValid() const
    {
        return AZ::IO::PathView(m_product.m_productFileName).IsRelative() && ExistsOnDisk(false);
    }

    bool ProductAsset::ExistsOnDisk(bool printErrorMessage) const
    {
        bool exists = AZ::IO::SystemFile::Exists(m_absolutePath.c_str());

        if (!exists && printErrorMessage)
        {
            AZ_TracePrintf(
                AssetProcessor::ConsoleChannel, "Was expecting product asset to exist at `%s` but it was not found\n",
                m_absolutePath.c_str());
        }

        return exists;
    }

    bool ProductAsset::DeleteFile() const
    {
        if (!ExistsOnDisk(false))
        {
            AZ_TracePrintf(
                AssetProcessor::ConsoleChannel, "Was expecting to delete product file %s but it already appears to be gone.\n",
                m_absolutePath.c_str());
            return false;
        }
        else if (!AZ::IO::SystemFile::Delete(m_absolutePath.c_str()))
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Was unable to delete product file %s\n", m_absolutePath.c_str());
            return false;
        }

        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Deleted product file %s\n", m_absolutePath.c_str());
        return true;
    }

    AZ::u64 ProductAsset::ComputeHash() const
    {
        return AssetUtilities::GetFileHash(m_absolutePath.c_str());
    }

    ProductAssetWrapper::ProductAssetWrapper(
        const AssetBuilderSDK::JobProduct& jobProduct, const AssetUtilities::ProductPath& productPath)
    {
        if ((jobProduct.m_outputFlags & AssetBuilderSDK::ProductOutputFlags::ProductAsset) ==
            AssetBuilderSDK::ProductOutputFlags::ProductAsset)
        {
            m_products.emplace_back(AZStd::make_unique<ProductAsset>(jobProduct, productPath.m_cachePath));
        }

        if ((jobProduct.m_outputFlags & AssetBuilderSDK::ProductOutputFlags::IntermediateAsset) ==
            AssetBuilderSDK::ProductOutputFlags::IntermediateAsset)
        {
            m_products.emplace_back(AZStd::make_unique<ProductAsset>(jobProduct, productPath.m_intermediatePath));
        }
    }

    bool ProductAssetWrapper::IsValid() const
    {
        return AZStd::all_of(
            m_products.begin(), m_products.end(),
            [](const AZStd::unique_ptr<ProductAsset>& productAsset)
            {
                return productAsset && productAsset->IsValid();
            });
    }

    bool ProductAssetWrapper::ExistOnDisk() const
    {
        return AZStd::all_of(
            m_products.begin(), m_products.end(),
            [](const AZStd::unique_ptr<ProductAsset>& productAsset)
            {
                return productAsset->ExistsOnDisk(true);
            });
    }

    bool ProductAssetWrapper::DeleteFiles() const
    {
        return AZStd::all_of(
            m_products.begin(), m_products.end(),
            [](const AZStd::unique_ptr<ProductAsset>& productAsset)
            {
                return productAsset->DeleteFile();
            });
    }

    AZ::u64 ProductAssetWrapper::ComputeHash() const
    {
        for (const auto& product : m_products)
        {
            if (product)
            {
                // We just need one of the hashes, they should all be the same
                return product->ComputeHash();
            }
        }

        return 0;
    }
}
