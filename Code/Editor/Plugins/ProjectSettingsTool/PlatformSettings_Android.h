/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/EditContext.h>

namespace ProjectSettingsTool
{
    class AndroidIcons
    {
    public:
        AZ_TYPE_INFO(AndroidIcons, "{D807ABEA-9C79-4EDD-B418-9A823E812C9A}");
        AZ_CLASS_ALLOCATOR(AndroidIcons, AZ::SystemAllocator);

        AndroidIcons()
            : m_default("")
            , m_mdpi("")
            , m_hdpi("")
            , m_xhdpi("")
            , m_xxhdpi("")
            , m_xxxhdpi("")
        {}

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_default;
        AZStd::string m_mdpi;
        AZStd::string m_hdpi;
        AZStd::string m_xhdpi;
        AZStd::string m_xxhdpi;
        AZStd::string m_xxxhdpi;
    };

    class AndroidLandscapeSplashscreens
    {
    public:
        AZ_TYPE_INFO(AndroidLandscapeSplashscreens, "{37888881-5050-47B6-9EB4-A408FD27D397}");
        AZ_CLASS_ALLOCATOR(AndroidIcons, AZ::SystemAllocator);

        AndroidLandscapeSplashscreens()
            : m_default("")
            , m_mdpi("")
            , m_hdpi("")
            , m_xhdpi("")
            , m_xxhdpi("")
        {}

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_default;
        AZStd::string m_mdpi;
        AZStd::string m_hdpi;
        AZStd::string m_xhdpi;
        AZStd::string m_xxhdpi;
    };

    class AndroidPortraitSplashscreens
    {
    public:
        AZ_TYPE_INFO(AndroidPortraitSplashscreens, "{2AADA22F-B5A3-440C-A592-FB923BC66878}");
        AZ_CLASS_ALLOCATOR(AndroidPortraitSplashscreens, AZ::SystemAllocator);

        AndroidPortraitSplashscreens()
            : m_default("")
            , m_mdpi("")
            , m_hdpi("")
            , m_xhdpi("")
            , m_xxhdpi("")
        {}

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_default;
        AZStd::string m_mdpi;
        AZStd::string m_hdpi;
        AZStd::string m_xhdpi;
        AZStd::string m_xxhdpi;
    };

    class AndroidSplashscreens
    {
    public:
        AZ_TYPE_INFO(AndroidSplashscreens, "{95985732-F45B-436A-86BC-7AE5249FF520}");
        AZ_CLASS_ALLOCATOR(AndroidSplashscreens, AZ::SystemAllocator);

        AndroidSplashscreens()
            : m_landscapeSplashscreens()
            , m_portraitSplashscreens()
        {}

        AndroidLandscapeSplashscreens m_landscapeSplashscreens;
        AndroidPortraitSplashscreens m_portraitSplashscreens;

        static void Reflect(AZ::ReflectContext* context);
    };

    class AndroidSettings
    {
    public:
        AZ_TYPE_INFO(AndroidSettings, "{5D57D014-4939-4D86-B862-AF35BAC705DC}");
        AZ_CLASS_ALLOCATOR(AndroidSettings, AZ::SystemAllocator);

        AndroidSettings()
            : m_packageName("")
            , m_versionName("")
            , m_versionNumber(1)
            , m_orientation("landscape")
            , m_appPublicKey("")
            , m_appObfuscatorSalt("")
            , m_rcPakJob("")
            , m_rcObbJob("")
            , m_useMainObb(false)
            , m_usePatchObb(false)
            , m_enableKeyScreenOn(false)
            , m_disableImmersiveMode(false)
            , m_icons()
            , m_splashscreens()
        {}

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_packageName;
        AZStd::string m_versionName;
        int m_versionNumber;
        AZStd::string m_orientation;
        AZStd::string m_appPublicKey;
        AZStd::string m_appObfuscatorSalt;
        AZStd::string m_rcPakJob;
        AZStd::string m_rcObbJob;
        bool m_useMainObb;
        bool m_usePatchObb;
        bool m_enableKeyScreenOn;
        bool m_disableImmersiveMode;
        AndroidIcons m_icons;
        AndroidSplashscreens m_splashscreens;
    };
} // namespace ProjectSettingsTool
