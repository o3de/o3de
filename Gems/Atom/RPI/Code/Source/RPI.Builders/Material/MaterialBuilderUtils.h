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
            //! Resolves paths to source files, registers job dependencies, and updates the job fingerprint
            //! @param originatingSourceFilePath The path of the .material or .materialtype file being processed
            //! @param referencedSourceFilePath The path to the referenced file as it appears in the current file
            //! @param response Used to update source dependencies
            //! @param jobDescriptor Used to update job dependencies
            //! @param jobKey The job key for the job that is expected to process the referenced file
            //! @param jobDependencyType Assigns the job dependency type for any added dependencies.
            //! @param productSubIds Sub ids used to filter which product dependencies cause the job to be reprocessed. This should default
            //! to an empty container so that all nested dependencies cause the job to be reprocessed. Leaving it empty results in the ideal
            //! propagation and ordering for shaders and materials, where materials always get reprocessed last. However, this causes issues
            //! if the platform ID is something other than PC. For example, if platform ID is set to server the asset system or streamer
            //! will fail to correctly resolve products generated from intermediate assets with a correct asset id.
            //! @param platformId Specific platform identifier for any added job dependencies
            void AddPossibleDependencies(
                const AZStd::string& originatingSourceFilePath,
                const AZStd::string& referencedSourceFilePath,
                AssetBuilderSDK::CreateJobsResponse& response,
                AssetBuilderSDK::JobDescriptor& jobDescriptor,
                const AZStd::string& jobKey,
                const AssetBuilderSDK::JobDependencyType jobDependencyType = AssetBuilderSDK::JobDependencyType::Order,
                const AZStd::vector<AZ::u32>& productSubIds = { 0 },
                const AZStd::string& platformId = {});

            //! Resolve potential paths and add source and job dependencies for image assets
            //! @param originatingSourceFilePath The path of the .material or .materialtype file being processed
            //! @param referencedSourceFilePath The path to the referenced file as it appears in the current file
            //! @param response Used to update source dependencies
            //! @param jobDescriptor Used to update job dependencies
            void AddPossibleImageDependencies(
                const AZStd::string& originatingSourceFilePath,
                const AZStd::string& referencedSourceFilePath,
                AssetBuilderSDK::CreateJobsResponse& response,
                AssetBuilderSDK::JobDescriptor& jobDescriptor);

            //! Append a fingerprint value to the job descriptor using the file modification time of the specified file path
            void AddFingerprintForDependency(const AZStd::string& path, AssetBuilderSDK::JobDescriptor& jobDescriptor);

            //! Generate a relative path from an absolute path
            AZStd::string GetRelativeSourcePath(const AZStd::string& path);
        } // namespace MaterialBuilderUtils
    } // namespace RPI
} // namespace AZ
