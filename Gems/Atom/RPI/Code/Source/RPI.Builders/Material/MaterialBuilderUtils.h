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
            //! @param jobKey The job key for the job that is expected to process the referenced file
            //! @param outputJobDescriptor Used to update job dependencies
            //! @param jobDependencyType Assigns the job dependency type for any added dependencies.
            void AddPossibleDependencies(
                const AZStd::string& originatingSourceFilePath,
                const AZStd::string& referencedSourceFilePath,
                const AZStd::string& jobKey,
                AssetBuilderSDK::JobDescriptor& outputJobDescriptor,
                AssetBuilderSDK::JobDependencyType jobDependencyType,
                const AZStd::vector<AZ::u32>& productSubIds = {},
                const AZStd::string& platformId = {});

            void AddPossibleImageDependencies(
                const AZStd::string& originatingSourceFilePath,
                const AZStd::string& imageFilePath,
                AssetBuilderSDK::JobDescriptor& outputJobDescriptor);

            void AddFingerprintForDependency(const AZStd::string& filePath, AssetBuilderSDK::JobDescriptor& outputJobDescriptor);

            AZStd::string GetRelativeSourcePath(const AZStd::string& path);
        } // namespace MaterialBuilderUtils
    } // namespace RPI
} // namespace AZ
