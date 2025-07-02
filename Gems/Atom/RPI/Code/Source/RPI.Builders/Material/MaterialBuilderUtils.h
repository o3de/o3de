/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AZ
{
    namespace RPI
    {
        class MaterialAsset;
        class MaterialTypeAsset;

        namespace MaterialBuilderUtils
        {
            //! @brief configure and register a job dependency with the job descriptor
            //! @param jobDescriptor job descriptor to which dependency will be added
            //! @param path path to the source file for the dependency
            //! @param jobKey job key for the builder processing the dependency
            //! @param platformId list of platform IDs to monitor for the job dependency
            //! @param subIds list of sub IDs that should be monitored for assets created by the job dependency
            //! @param updateFingerprint flag specifying if the job descriptor fingerprint should be updated with information from the
            //! dependency file
            //! @return reference to the new job dependency added to the job descriptor dependency container
            AssetBuilderSDK::JobDependency& AddJobDependency(
                AssetBuilderSDK::JobDescriptor& jobDescriptor,
                const AZStd::string& path,
                const AZStd::string& jobKey,
                const AZStd::string& platformId = {},
                const AZStd::vector<AZ::u32>& subIds = {},
                const bool updateFingerprint = true,
                const bool orderOnce = false);

            //! Given a material asset that has been fully built and prepared,
            //! add any image dependencies as pre-load dependencies, to the job being emitted.
            //! This will cause them to auto preload as part of loading the material, as well as make sure
            //! they are included in any pak files shipped with the product.
            void AddImageAssetDependenciesToProduct(const AZ::RPI::MaterialAsset* materialAsset, AssetBuilderSDK::JobProduct& product);

            //! Same as the above overload, but for material TYPE assets.
            void AddImageAssetDependenciesToProduct(const AZ::RPI::MaterialTypeAsset* materialTypeAsset, AssetBuilderSDK::JobProduct& product);

            //! Append a fingerprint value to the job descriptor using the file modification time of the specified file path
            void AddFingerprintForDependency(const AZStd::string& path, AssetBuilderSDK::JobDescriptor& jobDescriptor);
        } // namespace MaterialBuilderUtils
    } // namespace RPI
} // namespace AZ
