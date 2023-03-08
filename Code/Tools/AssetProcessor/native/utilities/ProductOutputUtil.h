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
        static AZStd::string GetPrefix(AZ::s64 scanfolderId);

        // Modifies product paths to have the scanfolder prepended to the filename
        // This needs to be done before the files are copied into the cache to avoid overwriting the legacy non-prepended version
        static void ModifyProductPath(QString& outputFilename, AZ::s64 sourceScanfolderId);

        // For meta types, does an override check to see if the provided source is the highest priority
        // If so, the products are renamed back to the non-prefixed version and any existing un-prefixed products
        // are prefixed
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
    };
} // namespace AssetProcessor
