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
            //! @param jobDependencies Dependencies may be added to this list
            //! @param sourceDependencies Dependencies may be added to this list
            //! @param forceOrderOnce If true, any job dependencies will use JobDependencyType::OrderOnce. Use this if the builder will only ever need to get the AssetId
            //!                       of the referenced file but will not need to load the asset.
            void AddPossibleDependencies(
                const AZStd::string& currentFilePath,
                const AZStd::string& referencedParentPath,
                const char* jobKey,
                AZStd::vector<AssetBuilderSDK::JobDependency>& jobDependencies,
                AZStd::vector<AssetBuilderSDK::SourceFileDependency>& sourceDependencies,
                bool forceOrderOnce = false,
                AZStd::optional<AZ::u32> productSubId = AZStd::nullopt);


            void AddPossibleImageDependencies(
                const AZStd::string& currentFilePath,
                const AZStd::string& imageFilePath,
                AZStd::vector<AssetBuilderSDK::JobDependency>& jobDependencies,
                AZStd::vector<AssetBuilderSDK::SourceFileDependency>& sourceDependencies);
        }

    } // namespace RPI
} // namespace AZ
