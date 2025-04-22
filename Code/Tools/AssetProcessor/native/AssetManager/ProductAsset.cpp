/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/AssetManager/ProductAsset.h>
#include <QDir>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/Metadata/MetadataManager.h>

namespace AssetProcessor
{
    ProductAsset::ProductAsset(AZ::IO::Path absolutePath)
        : m_absolutePath(AZStd::move(absolutePath))
    {
    }

    bool ProductAsset::IsValid() const
    {
        return ExistsOnDisk(false);
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

    bool ProductAsset::DeleteFile(bool sendNotification) const
    {
        if (!ExistsOnDisk(false))
        {
            AZ_TracePrintf(
                AssetProcessor::ConsoleChannel, "Was expecting to delete product file %s but it already appears to be gone.\n",
                m_absolutePath.c_str());
            return false;
        }

        if(sendNotification)
        {
            AssetProcessor::ProcessingJobInfoBus::Broadcast(
                &AssetProcessor::ProcessingJobInfoBus::Events::BeginCacheFileUpdate, m_absolutePath.AsPosix().c_str());
        }

        bool wasRemoved = AZ::IO::SystemFile::Delete(m_absolutePath.c_str());

        // Another process may be holding on to the file currently, so wait for a very brief period and then retry deleting
        // once in case we were just too quick to delete the file before.
        if (!wasRemoved)
        {
            constexpr int DeleteRetryDelay = 10;
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(DeleteRetryDelay));
            wasRemoved = AZ::IO::SystemFile::Delete(m_absolutePath.c_str());
        }

        // Try to delete the metadata file too if one exists
        auto metadataPath = AzToolsFramework::MetadataManager::ToMetadataPath(m_absolutePath).AsPosix();
        if(!AZ::IO::SystemFile::Delete(metadataPath.c_str()))
        {
            if (AZ::IO::SystemFile::Exists(metadataPath.c_str()))
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to remove metadata file " AZ_STRING_FORMAT, AZ_STRING_ARG(metadataPath));
                wasRemoved = false;
            }
        }

        if(sendNotification)
        {
            AssetProcessor::ProcessingJobInfoBus::Broadcast(
                &AssetProcessor::ProcessingJobInfoBus::Events::EndCacheFileUpdate, m_absolutePath.AsPosix().c_str(), false);
        }

        if(!wasRemoved)
        {
            return false;
        }

        // Try to clean up empty folder
        if (QDir(m_absolutePath.AsPosix().c_str()).entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot).empty())
        {
            AZ::IO::SystemFile::DeleteDir(m_absolutePath.ParentPath().FixedMaxPathStringAsPosix().c_str());
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
        AZ_Error("ProductAsset", AZ::IO::PathView(jobProduct.m_productFileName).IsRelative(), "Job Product m_productFileName (%s) must be relative",
            jobProduct.m_productFileName.c_str());

        if ((jobProduct.m_outputFlags & AssetBuilderSDK::ProductOutputFlags::ProductAsset) ==
            AssetBuilderSDK::ProductOutputFlags::ProductAsset)
        {
            m_cacheProduct = true;
            m_products.emplace_back(AZStd::make_unique<ProductAsset>(productPath.GetCachePath()));
        }

        if ((jobProduct.m_outputFlags & AssetBuilderSDK::ProductOutputFlags::IntermediateAsset) ==
            AssetBuilderSDK::ProductOutputFlags::IntermediateAsset)
        {
            m_intermediateProduct = true;
            m_products.emplace_back(AZStd::make_unique<ProductAsset>(productPath.GetIntermediatePath()));
        }
    }

    ProductAssetWrapper::ProductAssetWrapper(
        const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product,
        const AssetUtilities::ProductPath& productPath)
    {
        if((static_cast<AssetBuilderSDK::ProductOutputFlags>(product.m_flags.to_ullong()) & AssetBuilderSDK::ProductOutputFlags::ProductAsset) ==
            AssetBuilderSDK::ProductOutputFlags::ProductAsset)
        {
            m_cacheProduct = true;
            m_products.emplace_back(AZStd::make_unique<ProductAsset>(productPath.GetCachePath()));
        }

        if((static_cast<AssetBuilderSDK::ProductOutputFlags>(product.m_flags.to_ullong()) & AssetBuilderSDK::ProductOutputFlags::IntermediateAsset) ==
            AssetBuilderSDK::ProductOutputFlags::IntermediateAsset)
        {
            m_intermediateProduct = true;
            m_products.emplace_back(AZStd::make_unique<ProductAsset>(productPath.GetIntermediatePath()));
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

    bool ProductAssetWrapper::ExistOnDisk(bool printErrorMessage) const
    {
        return AZStd::all_of(
            m_products.begin(), m_products.end(),
            [printErrorMessage](const AZStd::unique_ptr<ProductAsset>& productAsset)
            {
                return productAsset->ExistsOnDisk(printErrorMessage);
            });
    }

    bool ProductAssetWrapper::HasCacheProduct() const
    {
        return m_cacheProduct;
    }

    bool ProductAssetWrapper::HasIntermediateProduct() const
    {
        return m_intermediateProduct;
    }

    bool ProductAssetWrapper::DeleteFiles(bool sendNotification) const
    {
        bool success = true;

        // Use a manual loop here since we want to be sure we attempt to delete every file even if one doesn't exist
        for(const auto& product : m_products)
        {
            success = product->DeleteFile(sendNotification) && success;
        }

        return success;
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
