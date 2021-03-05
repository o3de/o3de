/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/IO/FileIO.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Module/ModuleManager.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <GemRegistry/IGemRegistry.h>

namespace AssetValidation::AssetSeed
{
    constexpr char SeedFileExtension[] = "seed";
    constexpr char PlatformsDirectoryName[] = "Platforms";
    constexpr char GemsDirectoryName[] = "Gems";
    constexpr char GemsSeedFileName[] = "seedList";
    constexpr char EngineSeedFileName[] = "SeedAssetList";
    constexpr char EngineDirectoryName[] = "Engine";

    //! This struct stores gem related information
    struct GemInfo
    {
        AZ_CLASS_ALLOCATOR(GemInfo, AZ::SystemAllocator, 0);
        GemInfo(AZStd::string name, AZStd::string relativeFilePath, AZStd::string absoluteFilePath, AZStd::string identifier, bool isGameGem, bool assetOnlyGem);
        GemInfo() = default;
        AZStd::string m_gemName; ///< A friendly display name, not to be used for any pathing stuff.
        AZStd::string m_relativeFilePath; ///< Where the gem's folder is (relative to the gems search path(s))
        AZStd::string m_absoluteFilePath; ///< Where the gem's folder is (as an absolute path)
        AZStd::string m_identifier; ///< The UUID of the gem.

        bool m_isGameGem = false; //< True if its a 'game project' gem. Only one such gem can exist for any game project.
        bool m_assetOnly = false; ///< True if it is an asset only gems.

        static constexpr AZStd::string_view GetGemAssetFolder() { return AZStd::string_view("Assets"); }

    };

    void AddPlatformSeeds(const AZStd::string& rootFolder, AZStd::vector<AZStd::string>& defaultSeedLists, AzFramework::PlatformFlags platformFlags);

    void AddPlatformsDirectorySeeds(const AZStd::string& rootFolder, AZStd::vector<AZStd::string>& defaultSeedLists, AzFramework::PlatformFlags platformFlags);

    bool GetGemsInfo(const char* root, const char* assetRoot, const char* gameName, AZStd::vector<GemInfo>& gemInfoList);

    AZStd::vector<AZStd::string> GetGemSeedListFiles(const AZStd::vector<GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlags);

    AZStd::vector<AZStd::string> GetDefaultSeedListFiles(const AZStd::vector<GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlag);
}
