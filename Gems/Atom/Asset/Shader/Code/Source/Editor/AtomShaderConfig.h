/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include "AtomShaderCapabilitiesConfigFile.h"

namespace AZ
{
    namespace ShaderBuilder
    {
        namespace AtomShaderConfig
        {
            bool MutateToFirstAbsoluteFolderThatExists(AZStd::string& relativeFolder, AZStd::vector<AZStd::string>& watchFolders);

            AZStd::string GetAssetConfigPath(const char* platformFolder);

            CapabilitiesConfigFile GetMinDescriptorSetsFromConfigFile(const char* platformFolder);

            // @config argument comes from a platform abstracted folder
            AZStd::string FormatSupplementaryArgumentsFromConfigAtomShader(const CapabilitiesConfigFile& config);

            void AddParametersFromConfigFile(AZStd::string& parameters, const AssetBuilderSDK::PlatformInfo& platform);
        } // namespace AtomShaderConfig
    } // namespace ShaderBuilder
} //namespace AZ
