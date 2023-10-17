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

            //! Adds all relevant dependencies for a referenced source file, considering that the path might be relative to the original file location or a full asset path.
            //! This can include both source dependencies and a single job dependency, but will include only source dependencies if the file is not found.
            //! Note the AssetBuilderSDK::JobDependency::m_platformIdentifier will not be set by this function. The calling code must set this value before passing back
            //! to the AssetBuilderSDK::CreateJobsResponse.
            //! @param currentFilePath The path of the .material or .materialtype file being processed
            //! @param referencedParentPath The path to the referenced file as it appears in the current file
            //! @param jobKey The job key for the job that is expected to process the referenced file
            //! @param outputJobDescriptor Used to update job dependencies
            //! @param response Used to update source dependencies
            //! @param jobDependencyType Assigns the job dependency type for any added dependencies.
            void AddPossibleDependencies(
                const AZStd::string& currentFilePath,
                const AZStd::string& referencedParentPath,
                const char* jobKey,
                AssetBuilderSDK::JobDescriptor& outputJobDescriptor,
                AssetBuilderSDK::CreateJobsResponse& response,
                AssetBuilderSDK::JobDependencyType jobDependencyType,
                AZStd::optional<AZ::u32> productSubId = AZStd::nullopt);

            void AddPossibleImageDependencies(
                const AZStd::string& currentFilePath,
                const AZStd::string& imageFilePath,
                AssetBuilderSDK::JobDescriptor& outputJobDescriptor,
                AssetBuilderSDK::CreateJobsResponse& response);

            void AddFingerprintForDependency(
                const AZStd::string& filePath,
                AssetBuilderSDK::JobDescriptor& outputJobDescriptor);
        } // namespace MaterialBuilderUtils
    } // namespace RPI
} // namespace AZ
