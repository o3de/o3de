/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
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
