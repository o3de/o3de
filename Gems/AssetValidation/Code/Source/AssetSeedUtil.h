/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/FileIO.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Module/ModuleManager.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzFramework/Gem/GemInfo.h>
#include <AzFramework/Platform/PlatformDefaults.h>

namespace AssetValidation::AssetSeed
{
    constexpr char SeedFileExtension[] = "seed";
    constexpr char PlatformsDirectoryName[] = "Platforms";
    constexpr char GemsDirectoryName[] = "Gems";
    constexpr char GemsSeedFileName[] = "seedList";
    constexpr char EngineSeedFileName[] = "SeedAssetList";
    constexpr auto EngineDirectoryName = AZ::IO::FixedMaxPath("Assets") / "Engine";

    void AddPlatformSeeds(const AZStd::string& rootFolder, AZStd::vector<AZStd::string>& defaultSeedLists, AzFramework::PlatformFlags platformFlags);

    void AddPlatformsDirectorySeeds(const AZStd::string& rootFolder, AZStd::vector<AZStd::string>& defaultSeedLists, AzFramework::PlatformFlags platformFlags);


    AZStd::vector<AZStd::string> GetGemSeedListFiles(const AZStd::vector<AzFramework::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlags);

    AZStd::vector<AZStd::string> GetDefaultSeedListFiles(const AZStd::vector<AzFramework::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlag);
}
