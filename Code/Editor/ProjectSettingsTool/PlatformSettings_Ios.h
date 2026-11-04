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
    class IosIcons
    {
    public:
        AZ_TYPE_INFO(IosIcons, "{A57F9F23-36F5-4425-B40A-56D60119E5C9}");
        AZ_CLASS_ALLOCATOR(IosIcons, AZ::SystemAllocator);

        IosIcons()
            : m_appStore("")
            , m_iphoneApp120("")
            , m_iphoneApp180("")
            , m_iphoneNotification40("")
            , m_iphoneNotification60("")
            , m_iphoneSettings58("")
            , m_iphoneSettings87("")
            , m_iphoneSpotlight80("")
            , m_iphoneSpotlight120("")
            , m_ipadApp76("")
            , m_ipadApp152("")
            , m_ipadProApp("")
            , m_ipadNotification20("")
            , m_ipadNotification40("")
            , m_ipadSettings29("")
            , m_ipadSettings58("")
            , m_ipadSpotlight40("")
            , m_ipadSpotlight80("")
        {}

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_appStore;
        AZStd::string m_iphoneApp120;
        AZStd::string m_iphoneApp180;
        AZStd::string m_iphoneNotification40;
        AZStd::string m_iphoneNotification60;
        AZStd::string m_iphoneSettings58;
        AZStd::string m_iphoneSettings87;
        AZStd::string m_iphoneSpotlight80;
        AZStd::string m_iphoneSpotlight120;
        AZStd::string m_ipadApp76;
        AZStd::string m_ipadApp152;
        AZStd::string m_ipadProApp;
        AZStd::string m_ipadNotification20;
        AZStd::string m_ipadNotification40;
        AZStd::string m_ipadSettings29;
        AZStd::string m_ipadSettings58;
        AZStd::string m_ipadSpotlight40;
        AZStd::string m_ipadSpotlight80;
    };

    class IosLaunchscreens
    {
    public:
        AZ_TYPE_INFO(IosLaunchscreens, "{1A34706F-9558-4081-9898-33758B026629}");
        AZ_CLASS_ALLOCATOR(IosLaunchscreens, AZ::SystemAllocator);

        IosLaunchscreens()
            : m_iphone640x960("")
            , m_iphone640x1136("")
            , m_iphone750x1334("")
            , m_iphone1125x2436("")
            , m_iphone2436x1125("")
            , m_iphone1242x2208("")
            , m_iphone2208x1242("")
            , m_ipad768x1024("")
            , m_ipad1024x768("")
            , m_ipad1536x2048("")
            , m_ipad2048x1536("")
        {}

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_iphone640x960;
        AZStd::string m_iphone640x1136;
        AZStd::string m_iphone750x1334;
        AZStd::string m_iphone1125x2436;
        AZStd::string m_iphone2436x1125;
        AZStd::string m_iphone1242x2208;
        AZStd::string m_iphone2208x1242;
        AZStd::string m_ipad768x1024;
        AZStd::string m_ipad1024x768;
        AZStd::string m_ipad1536x2048;
        AZStd::string m_ipad2048x1536;
    };

    class IosOrientations
    {
    public:
        AZ_TYPE_INFO(IosOrientations, "{A42CDF2E-CCE1-4D93-9D4E-2270CFC0F2ED}");
        AZ_CLASS_ALLOCATOR(IosOrientations, AZ::SystemAllocator);

        IosOrientations()
            : m_landscapeRight(false)
            , m_landscapeLeft(false)
            , m_portraitBottom(false)
            , m_portraitTop(false)
        {}

        static void Reflect(AZ::ReflectContext* context);

        bool m_landscapeRight;
        bool m_landscapeLeft;
        bool m_portraitBottom;
        bool m_portraitTop;
    };

    class IosSettings
    {
    public:
        AZ_TYPE_INFO(IosSettings, "{9EDF051E-0158-4ADE-92A3-B7AC230E0114}");
        AZ_CLASS_ALLOCATOR(IosSettings, AZ::SystemAllocator);

        IosSettings()
            : m_bundleName("")
            , m_bundleDisplayName("")
            , m_executableName("")
            , m_bundleIdentifier("com.amazon.o3de.UnknownProject")
            , m_versionName("1.0.0")
            , m_versionNumber("1.0.0")
            , m_developmentRegion("en_US")
            , m_requiresFullscreen(false)
            , m_hideStatusBar(false)
            , m_iphoneOrientations()
            , m_ipadOrientations()
            , m_icons()
            , m_launchscreens()
        {}

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_bundleName;
        AZStd::string m_bundleDisplayName;
        AZStd::string m_executableName;
        AZStd::string m_bundleIdentifier;
        AZStd::string m_versionName;
        AZStd::string m_versionNumber;
        AZStd::string m_developmentRegion;
        bool m_requiresFullscreen;
        bool m_hideStatusBar;
        IosOrientations m_iphoneOrientations;
        IosOrientations m_ipadOrientations;
        IosIcons m_icons;
        IosLaunchscreens m_launchscreens;
    };
} // namespace ProjectSettingsTool
