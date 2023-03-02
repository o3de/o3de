/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/utilities/ProductOutputUtil.h>
#include <QString>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/IO/FileOperations.h>
#include <native/utilities/UuidManager.h>

namespace AssetProcessor
{
    AZStd::string ProductOutputUtil::GetPrefix(AZ::s64 scanfolderId)
    {
        return AZStd::string::format("(%" PRIu64 ")", scanfolderId);
    }

    void ProductOutputUtil::ModifyProductPath(QString& outputFilename, AZ::s64 sourceScanfolderId)
    {
        auto prefix = GetPrefix(sourceScanfolderId);
        outputFilename = QStringLiteral("%1%2").arg(prefix.c_str()).arg(outputFilename);
    }

    void ProductOutputUtil::FinalizeProduct(AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection> db, const PlatformConfiguration* platformConfig, const SourceAssetReference& sourceAsset, AZStd::vector<AssetBuilderSDK::JobProduct>& products, AZStd::string_view platformIdentifier)
    {
        auto* uuidInterface = AZ::Interface<IUuidRequests>::Get();
        AZ_Assert(uuidInterface, "Programmer Error - IUuidRequests interface is not available.");

        if (uuidInterface->IsGenerationEnabledForFile(sourceAsset.AbsolutePath()))
        {
            QString overrider =
                platformConfig->GetOverridingFile(sourceAsset.RelativePath().c_str(), sourceAsset.ScanFolderPath().c_str());

            if (overrider.isEmpty())
            {
                // There is no other file or this is the highest priority
                for (auto& product : products)
                {
                    QString scanfolder, relativePath;

                    if (!AZ::IO::PathView(product.m_productFileName).IsRelative())
                    {
                        platformConfig->ConvertToRelativePath(product.m_productFileName.c_str(), scanfolder, relativePath);
                    }
                    else
                    {
                        relativePath = product.m_productFileName.c_str();
                    }

                    AZStd::string prefix = GetPrefix(sourceAsset.ScanFolderId());
                    int prefixPos = relativePath.indexOf(prefix.c_str());

                    if (prefixPos < 0)
                    {
                        AZ_Error(
                            "OutputManager",
                            false,
                            "Product " AZ_STRING_FORMAT " is expected to be prefixed but was not",
                            AZ_STRING_ARG(product.m_productFileName));
                        continue;
                    }

                    // Remove the prefix and update
                    QStringRef unprefixedString = relativePath.midRef(prefixPos + prefix.size());
                    AZStd::string newName =
                        (AZ::IO::FixedMaxPath(scanfolder.toUtf8().constData()) / unprefixedString.toUtf8().constData()).c_str();

                    AssetUtilities::ProductPath oldProductPath(product.m_productFileName, platformIdentifier);
                    AssetUtilities::ProductPath newProductPath(newName, platformIdentifier);
                    AssetProcessor::ProductAssetWrapper wrapper{ product, oldProductPath };

                    auto oldAbsolutePath =
                        wrapper.HasCacheProduct() ? oldProductPath.GetCachePath() : oldProductPath.GetIntermediatePath();
                    auto newAbsolutePath =
                        wrapper.HasCacheProduct() ? newProductPath.GetCachePath() : newProductPath.GetIntermediatePath();

                    product.m_productFileName = newName;

                    // Find any other sources which output the non-prefixed product
                    AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer existingProducts;
                    if (db->GetProductsByProductName(newProductPath.GetDatabasePath().c_str(), existingProducts))
                    {
                        for (const auto& existingProduct : existingProducts)
                        {
                            AzToolsFramework::AssetDatabase::SourceDatabaseEntry existingSource;
                            if (db->GetSourceByProductID(existingProduct.m_productID, existingSource))
                            {
                                if (existingSource.m_scanFolderPK != sourceAsset.ScanFolderId() ||
                                    AZ::IO::PathView(existingSource.m_sourceName) != sourceAsset.RelativePath())
                                {
                                    // Found a different source already using this product name
                                    // This should be a previously-higher priority override
                                    // Rename the existing file/product entries from non-prefixed to prefixed version
                                    RenameProduct(db, existingProduct, existingSource, platformIdentifier);
                                }
                            }
                        }
                    }

                    auto result = AZ::IO::SmartMove(oldAbsolutePath.c_str(), newAbsolutePath.c_str());

                    if (!result)
                    {
                        AZ_Error(
                            "OutputManager",
                            false,
                            "Failed to move product from " AZ_STRING_FORMAT " to " AZ_STRING_FORMAT,
                            AZ_STRING_ARG(oldAbsolutePath),
                            AZ_STRING_ARG(newAbsolutePath));
                    }
                }
            }
        }
    }

    void ProductOutputUtil::RenameProduct(AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection> db, AzToolsFramework::AssetDatabase::ProductDatabaseEntry existingProduct, const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry, AZStd::string_view platformIdentifier)
    {
        auto oldProductPath = AssetUtilities::ProductPath::FromDatabasePath(existingProduct.m_productName);
        QString newName = oldProductPath.GetRelativePath().c_str();
        ModifyProductPath(newName, sourceEntry.m_scanFolderPK);

        AssetUtilities::ProductPath newProductPath(newName.toUtf8().constData(), platformIdentifier);
        AssetProcessor::ProductAssetWrapper wrapper{ existingProduct, oldProductPath };

        auto oldAbsolutePath = wrapper.HasCacheProduct() ? oldProductPath.GetCachePath() : oldProductPath.GetIntermediatePath();
        auto newAbsolutePath = wrapper.HasCacheProduct() ? newProductPath.GetCachePath() : newProductPath.GetIntermediatePath();

        auto result = AZ::IO::SmartMove(oldAbsolutePath.c_str(), newAbsolutePath.c_str());

        if (!result)
        {
            AZ_Error(
                "OutputManager",
                false,
                "Failed to move product from " AZ_STRING_FORMAT " to " AZ_STRING_FORMAT,
                AZ_STRING_ARG(oldAbsolutePath),
                AZ_STRING_ARG(newAbsolutePath));
        }

        existingProduct.m_productName = newProductPath.GetDatabasePath();
        db->SetProduct(existingProduct);
    }
} // namespace AssetProcessor
