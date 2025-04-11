/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <native/AssetDatabase/AssetDatabase.h>
#include <native/AssetManager/ProductAsset.h>
#include <native/utilities/PlatformConfiguration.h>

namespace AssetProcessor
{
    struct ProductOutputUtil
    {
        //! Gets the prefix to apply to products when copying into the cache before being finalized.
        //! This is different from the final prefix version to avoid conflicts unprefixed assets that may use the same prefix as the final prefix.
        //! The interim prefix shouldn't show up in any UI, so use a longer, less-likely-to-conflict version.
        static AZStd::string GetInterimPrefix(AZ::s64 scanfolderId);
        //! Gets the prefix to apply to products when finalizing.
        static AZStd::string GetFinalPrefix(AZ::s64 scanfolderId);

        //! Modifies product paths to have the interim prefixed version while waiting on final processing.
        //! This needs to be done before the files are copied into the cache to avoid overwriting the legacy non-prepended version.
        static void GetInterimProductPath(QString& outputFilename, AZ::s64 sourceScanfolderId);
        //! Modifies product paths to the final prefixed version for the cache.
        static void GetFinalProductPath(QString& outputFilename, AZ::s64 sourceScanfolderId);

        //! For meta types, does an override check to see if the provided source is the highest priority
        //! If so, the products are renamed back to the non-prefixed version and any existing un-prefixed products
        //! are prefixed
        static void FinalizeProduct(
            AZStd::shared_ptr<AssetDatabaseConnection> db,
            const PlatformConfiguration* platformConfig,
            const SourceAssetReference& sourceAsset,
            AZStd::vector<AssetBuilderSDK::JobProduct>& products,
            AZStd::string_view platformIdentifier);

    protected:
        static void RenameProduct(
            AZStd::shared_ptr<AssetDatabaseConnection> db,
            AzToolsFramework::AssetDatabase::ProductDatabaseEntry existingProduct,
            const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry);

        static bool ComputeFinalProductName(
            const AZStd::string& currentPrefix,
            const AZStd::string& newPrefix,
            const AssetBuilderSDK::JobProduct& product,
            AZStd::string& newName);

        static auto GetPaths(
            const AssetBuilderSDK::JobProduct& product,
            AZStd::string_view platformIdentifier,
            const AssetUtilities::ProductPath& newProductPath);

        static bool DoFileRename(const AZStd::string& oldAbsolutePath, const AZStd::string& newAbsolutePath);
    };
} // namespace AssetProcessor
