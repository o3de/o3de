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
                const bool updateFingerprint = true);

            //! Resolve potential paths and add source and job dependencies for image assets
            //! @param originatingSourceFilePath The path of the .material or .materialtype file being processed
            //! @param referencedSourceFilePath The path to the referenced file as it appears in the current file
            //! @param jobDescriptor Used to update job dependencies
            void AddPossibleImageDependencies(
                const AZStd::string& originatingSourceFilePath,
                const AZStd::string& referencedSourceFilePath,
                AssetBuilderSDK::JobDescriptor& jobDescriptor);

            //! Append a fingerprint value to the job descriptor using the file modification time of the specified file path
            void AddFingerprintForDependency(const AZStd::string& path, AssetBuilderSDK::JobDescriptor& jobDescriptor);
        } // namespace MaterialBuilderUtils
    } // namespace RPI
} // namespace AZ
