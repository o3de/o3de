/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/utilities/ProductOutputUtil.h>
#include <QString>
#include <QFile>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzToolsFramework/Metadata/MetadataManager.h>
#include <native/utilities/UuidManager.h>
#include <native/utilities/IMetadataUpdates.h>

namespace AssetProcessor
{
    AZStd::string ProductOutputUtil::GetInterimPrefix(AZ::s64 scanfolderId)
    {
        return AZStd::string::format("(TMP%" PRId64 "___)", int64_t(scanfolderId));
    }

    AZStd::string ProductOutputUtil::GetFinalPrefix(AZ::s64 scanfolderId)
    {
        return AZStd::string::format("(%" PRId64 ")", int64_t(scanfolderId));
    }

    void ProductOutputUtil::GetInterimProductPath(QString& outputFilename, AZ::s64 sourceScanfolderId)
    {
        auto prefix = GetInterimPrefix(sourceScanfolderId);
        outputFilename = QStringLiteral("%1%2").arg(prefix.c_str()).arg(outputFilename);
    }

    void ProductOutputUtil::GetFinalProductPath(QString& outputFilename, AZ::s64 sourceScanfolderId)
    {
        auto prefix = GetFinalPrefix(sourceScanfolderId);
        outputFilename = QStringLiteral("%1%2").arg(prefix.c_str()).arg(outputFilename);
    }

    auto ProductOutputUtil::GetPaths(
        const AssetBuilderSDK::JobProduct& product,
        AZStd::string_view platformIdentifier,
        const AssetUtilities::ProductPath& newProductPath)
    {
        AssetUtilities::ProductPath oldProductPath(product.m_productFileName, platformIdentifier);
        AssetProcessor::ProductAssetWrapper wrapper{ product, oldProductPath };

        auto oldAbsolutePath = wrapper.HasCacheProduct() ? oldProductPath.GetCachePath() : oldProductPath.GetIntermediatePath();
        auto newAbsolutePath = wrapper.HasCacheProduct() ? newProductPath.GetCachePath() : newProductPath.GetIntermediatePath();

        return AZStd::make_tuple(oldAbsolutePath, newAbsolutePath);
    }

    void ProductOutputUtil::FinalizeProduct(
        AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection> db,
        const PlatformConfiguration* platformConfig,
        const SourceAssetReference& sourceAsset,
        AZStd::vector<AssetBuilderSDK::JobProduct>& products,
        AZStd::string_view platformIdentifier)
    {
        auto* uuidInterface = AZ::Interface<IUuidRequests>::Get();
        AZ_Assert(uuidInterface, "Programmer Error - IUuidRequests interface is not available.");

        if (uuidInterface->IsGenerationEnabledForFile(sourceAsset.AbsolutePath()))
        {
            QString overrider = platformConfig->GetOverridingFile(sourceAsset.RelativePath().c_str(), sourceAsset.ScanFolderPath().c_str());

            if (overrider.isEmpty())
            {
                // There is no other file or this is the highest priority

                // Sort the products by filename first
                // This prevents an edge case where there are multiple outputs like
                // a.png
                // (2)a.png
                // from the builder which have been renamed to
                // (2)a.png
                // (2)(2)a.png
                // By sorting, (2)a.png will be renamed first, avoiding the case where (2)(2)a.png is trying to be renamed to (2)a.png which already exists
                std::stable_sort(
                    products.begin(),
                    products.end(),
                    [](const AssetBuilderSDK::JobProduct& a, const AssetBuilderSDK::JobProduct& b)
                    {
                        return a.m_productFileName.compare(b.m_productFileName) < 0;
                    });

                for (auto& product : products)
                {
                    AZStd::string newName;
                    if (!ComputeFinalProductName(GetInterimPrefix(sourceAsset.ScanFolderId()), "", product, newName))
                    {
                        // Error handling is done by function
                        continue;
                    }

                    AssetUtilities::ProductPath newProductPath(newName, platformIdentifier);
                    auto [oldAbsolutePath, newAbsolutePath] = GetPaths(product, platformIdentifier, newProductPath);

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
                                    RenameProduct(db, existingProduct, existingSource);
                                }
                            }
                        }
                    }

                    DoFileRename(oldAbsolutePath, newAbsolutePath);
                }
            }
            else
            {
                for (auto& product : products)
                {
                    AZStd::string newName;

                    if (!ComputeFinalProductName(GetInterimPrefix(sourceAsset.ScanFolderId()), GetFinalPrefix(sourceAsset.ScanFolderId()), product, newName))
                    {
                        continue;
                    }

                    AssetUtilities::ProductPath newProductPath(newName, platformIdentifier);
                    auto [oldAbsolutePath, newAbsolutePath] = GetPaths(product, platformIdentifier, newProductPath);

                    product.m_productFileName = newName;

                    DoFileRename(oldAbsolutePath, newAbsolutePath);
                }
            }
        }
    }

    bool ProductOutputUtil::ComputeFinalProductName(
        const AZStd::string& currentPrefix,
        const AZStd::string& newPrefix,
        const AssetBuilderSDK::JobProduct& product,
        AZStd::string& newName)
    {
        QString filename = AZ::IO::PathView(product.m_productFileName).Filename().FixedMaxPathString().c_str();

        int prefixPos = filename.indexOf(currentPrefix.c_str());

        if (prefixPos < 0)
        {
            AZ_Error(
                "ProductOutputUtil",
                false,
                "Programmer Error - Product " AZ_STRING_FORMAT " is expected to be prefixed but was not",
                AZ_STRING_ARG(product.m_productFileName));
            return false;
        }

        // Remove the prefix and update
        QStringRef unprefixedString = filename.midRef(static_cast<int>(prefixPos + currentPrefix.size()));
        newName = (AZ::IO::FixedMaxPath(AZ::IO::PathView(product.m_productFileName).ParentPath()) / (newPrefix + unprefixedString.toUtf8().constData()))
                      .StringAsPosix();

        return true;
    }

    bool ProductOutputUtil::DoFileRename(const AZStd::string& oldAbsolutePath, const AZStd::string& newAbsolutePath)
    {
        auto* updateInterface = AZ::Interface<IMetadataUpdates>::Get();
        AZ_Assert(updateInterface, "Programmer Error - IMetadataUpdates interface is not available.");
        updateInterface->PrepareForFileMove(oldAbsolutePath.c_str(), newAbsolutePath.c_str());

        bool result = AssetUtilities::MoveFileWithTimeout(oldAbsolutePath.c_str(), newAbsolutePath.c_str(), 1);

        AZ_Error(
            "ProductOutputUtil",
            result,
            "Failed to move product from " AZ_STRING_FORMAT " to " AZ_STRING_FORMAT ".  See previous log messages for details on failure.",
            AZ_STRING_ARG(oldAbsolutePath),
            AZ_STRING_ARG(newAbsolutePath));

        auto oldMetadataFile = AzToolsFramework::MetadataManager::ToMetadataPath(oldAbsolutePath);

        if (QFile(oldMetadataFile.c_str()).exists())
        {
            // Move the metadata file
            result = AssetUtilities::MoveFileWithTimeout(
                oldMetadataFile.c_str(),
                AzToolsFramework::MetadataManager::ToMetadataPath(newAbsolutePath).c_str());

            AZ_Error(
                "ProductOutputUtil",
                result,
                "Failed to move product metadata from " AZ_STRING_FORMAT " to " AZ_STRING_FORMAT
                ".  See previous log messages for details on failure.",
                AZ_STRING_ARG(oldAbsolutePath),
                AZ_STRING_ARG(newAbsolutePath));
        }

        return result;
    }

    void ProductOutputUtil::RenameProduct(
        AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection> db,
        AzToolsFramework::AssetDatabase::ProductDatabaseEntry existingProduct,
        const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
    {
        auto oldProductPath = AssetUtilities::ProductPath::FromDatabasePath(existingProduct.m_productName);
        AZ::IO::FixedMaxPath existingProductName(existingProduct.m_productName);
        QString newName = existingProductName.Filename().StringAsPosix().c_str();
        GetFinalProductPath(newName, sourceEntry.m_scanFolderPK);

        auto newProductPath = AssetUtilities::ProductPath::FromDatabasePath(
            (AZ::IO::FixedMaxPath(existingProductName.ParentPath()) / newName.toUtf8().constData()).Native().c_str());
        AssetProcessor::ProductAssetWrapper wrapper{ existingProduct, oldProductPath };

        auto oldAbsolutePath = wrapper.HasCacheProduct() ? oldProductPath.GetCachePath() : oldProductPath.GetIntermediatePath();
        auto newAbsolutePath = wrapper.HasCacheProduct() ? newProductPath.GetCachePath() : newProductPath.GetIntermediatePath();

        auto* updateInterface = AZ::Interface<IMetadataUpdates>::Get();
        AZ_Assert(updateInterface, "Programmer Error - IMetadataUpdates interface is not available.");
        updateInterface->PrepareForFileMove(oldAbsolutePath.c_str(), newAbsolutePath.c_str());

        [[maybe_unused]] bool result = AssetUtilities::MoveFileWithTimeout(oldAbsolutePath.c_str(), newAbsolutePath.c_str());

        AZ_Error(
            "ProductOutputUtil",
            result,
            "Failed to move product from " AZ_STRING_FORMAT " to " AZ_STRING_FORMAT
            ".  See previous log messages for details on failure.",
            AZ_STRING_ARG(oldAbsolutePath),
            AZ_STRING_ARG(newAbsolutePath));

        auto oldMetadataFile = AzToolsFramework::MetadataManager::ToMetadataPath(oldAbsolutePath);

        if (QFile(oldMetadataFile.c_str()).exists())
        {
            // Move the metadata file
            result = AssetUtilities::MoveFileWithTimeout(
                AzToolsFramework::MetadataManager::ToMetadataPath(oldAbsolutePath).c_str(),
                AzToolsFramework::MetadataManager::ToMetadataPath(newAbsolutePath).c_str());

            AZ_Error(
                "ProductOutputUtil",
                result,
                "Failed to move product metadata from " AZ_STRING_FORMAT " to " AZ_STRING_FORMAT
                ".  See previous log messages for details on failure.",
                AZ_STRING_ARG(oldAbsolutePath),
                AZ_STRING_ARG(newAbsolutePath));
        }

        existingProduct.m_productName = newProductPath.GetDatabasePath();
        db->SetProduct(existingProduct);
    }
} // namespace AssetProcessor
