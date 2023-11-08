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
