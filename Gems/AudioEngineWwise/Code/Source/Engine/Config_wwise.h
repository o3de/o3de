/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>

namespace Audio::Wwise
{
    static constexpr const char DefaultBanksPath[] = "sounds/wwise/";
    static constexpr const char ExternalSourcesPath[] = "external";
    static constexpr const char ConfigFile[] = "wwise_config.json";
    static constexpr const char BankExtension[] = ".bnk";
    static constexpr const char MediaExtension[] = ".wem";
    static constexpr const char InitBank[] = "init.bnk";

    //! Banks path that's set after reading the configuration settings.
    //! This might be different than the DefaultBanksPath.
    const AZStd::string_view GetBanksRootPath();
    void SetBanksRootPath(const AZStd::string_view path);

    /**
     * ConfigurationSettings
     */
    struct ConfigurationSettings
    {
        AZ_TYPE_INFO(ConfigurationSettings, "{6BEEC05E-C5AE-4270-AAAD-08E27A6B5341}");
        AZ_CLASS_ALLOCATOR(ConfigurationSettings, AZ::SystemAllocator, 0);

        struct PlatformMapping
        {
            AZ_TYPE_INFO(PlatformMapping, "{9D444546-784B-4509-A8A5-8E174E345097}");
            AZ_CLASS_ALLOCATOR(PlatformMapping, AZ::SystemAllocator, 0);

            PlatformMapping() = default;
            ~PlatformMapping() = default;

            // Serialized Data...
            AZStd::string m_assetPlatform;      // LY Asset Platform name (i.e. "pc", "mac", "android", ...)
            AZStd::string m_altAssetPlatform;   // Some platforms can be run using a different asset platform.  Useful for builder worker.
            AZStd::string m_enginePlatform;     // LY Engine Platform name (i.e. "Windows", "Mac", "Android", ...)
            AZStd::string m_wwisePlatform;      // Wwise Platform name (i.e. "Windows", "Mac", "Android", ...)
            AZStd::string m_bankSubPath;        // Wwise Banks Sub-Path (i.e. "windows", "mac", "android", ...)
        };

        ConfigurationSettings() = default;
        ~ConfigurationSettings() = default;

        static void Reflect(AZ::ReflectContext* context);

        bool Load(const AZStd::string& filePath);
        bool Save(const AZStd::string& filePath);

        // Serialized Data...
        AZStd::vector<PlatformMapping> m_platformMappings;
    };

} // namespace Audio::Wwise
