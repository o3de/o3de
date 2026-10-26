/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PlatformSettings_Ios.h"

#include "PlatformSettings_common.h"
#include "Validators.h"

#include <AzFramework/Translation/TranslationDef.h>

namespace ProjectSettingsTool
{
    namespace Icons
    {
        static const char* appStore = "iOSAppStoreIcon1024x1024";
        static const char* iphoneApp120 = "iPhoneAppIcon120x120";
        static const char* iphoneApp180 = "iPhoneAppIcon180x180";
        static const char* iphoneNotification40 = "iPhoneNotificationIcon40x40";
        static const char* iphoneNotification60 = "iPhoneNotificationIcon60x60";
        static const char* iphoneSettings58 = "iPhoneSettingsIcon58x58";
        static const char* iphoneSettings87 = "iPhoneSettingsIcon87x87";
        static const char* iphoneSpotlight80 = "iPhoneSpotlightIcon80x80";
        static const char* iphoneSpotlight120 = "iPhoneSpotlightIcon120x120";
        static const char* ipadApp76 = "iPadAppIcon76x76";
        static const char* ipadApp152 = "iPadAppIcon152x152";
        static const char* ipadProApp = "iPadProAppIcon167x167";
        static const char* ipadNotification20 = "iPadNotificationIcon20x20";
        static const char* ipadNotification40 = "iPadNotificationIcon40x40";
        static const char* ipadSettings29 = "iPadSettingsIcon29x29";
        static const char* ipadSettings58 = "iPadSettingsIcon58x58";
        static const char* ipadSpotlight40 = "iPadSpotlightIcon40x40";
        static const char* ipadSpotlight80 = "iPadSpotlightIcon80x80";
    };

    void IosIcons::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<IosIcons>()
                ->Version(1)
                ->Field(Icons::appStore, &IosIcons::m_appStore)
                ->Field(Icons::iphoneApp120, &IosIcons::m_iphoneApp120)
                ->Field(Icons::iphoneApp180, &IosIcons::m_iphoneApp180)
                ->Field(Icons::iphoneNotification40, &IosIcons::m_iphoneNotification40)
                ->Field(Icons::iphoneNotification60, &IosIcons::m_iphoneNotification60)
                ->Field(Icons::iphoneSettings58, &IosIcons::m_iphoneSettings58)
                ->Field(Icons::iphoneSettings87, &IosIcons::m_iphoneSettings87)
                ->Field(Icons::iphoneSpotlight80, &IosIcons::m_iphoneSpotlight80)
                ->Field(Icons::iphoneSpotlight120, &IosIcons::m_iphoneSpotlight120)
                ->Field(Icons::ipadApp76, &IosIcons::m_ipadApp76)
                ->Field(Icons::ipadApp152, &IosIcons::m_ipadApp152)
                ->Field(Icons::ipadProApp, &IosIcons::m_ipadProApp)
                ->Field(Icons::ipadNotification20, &IosIcons::m_ipadNotification20)
                ->Field(Icons::ipadNotification40, &IosIcons::m_ipadNotification40)
                ->Field(Icons::ipadSettings29, &IosIcons::m_ipadSettings29)
                ->Field(Icons::ipadSettings58, &IosIcons::m_ipadSettings58)
                ->Field(Icons::ipadSpotlight40, &IosIcons::m_ipadSpotlight40)
                ->Field(Icons::ipadSpotlight80, &IosIcons::m_ipadSpotlight80)
            ;

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<IosIcons>(
                    QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Icons"),
                    QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "All png icon overrides for iOS."))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_appStore,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "App Store (1024px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<1024>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::appStore))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_iphoneApp120,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone App (120px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<120>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::iphoneApp120))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_iphoneApp180,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone App (180px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<180>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::iphoneApp180))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_iphoneNotification40,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone Notification (40px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<40>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::iphoneNotification40))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_iphoneNotification60,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone Notification (60px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<60>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::iphoneNotification60))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_iphoneSettings58,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone Settings (58px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<58>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::iphoneSettings58))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_iphoneSettings87,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone Settings (87px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<87>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::iphoneSettings87))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_iphoneSpotlight80,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone Spotlight (80px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<80>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::iphoneSpotlight80))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_iphoneSpotlight120,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone Spotlight (120px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<120>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::iphoneSpotlight120))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_ipadApp76,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPad App (76px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<76>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::ipadApp76))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_ipadApp152,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPad App (152px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<152>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::ipadApp152))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_ipadProApp,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPad Pro App (167px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<167>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::ipadProApp))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_ipadNotification20,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPad Notification (20px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<20>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::ipadNotification20))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_ipadNotification40,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPad Notification (40px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<40>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::ipadNotification40))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_ipadSettings29,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPad Settings (29px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<29>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::ipadSettings29))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_ipadSettings58,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPad Settings (58px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<58>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::ipadSettings58))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_ipadSpotlight40,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPad Spotlight (40px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<40>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::ipadSpotlight40))
                    ->DataElement(Handlers::ImagePreview, &IosIcons::m_ipadSpotlight80,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPad Spotlight (80px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<80>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosIcons, Icons::ipadSpotlight80))
                ;
            }
        }
    }

    namespace Launchscreens
    {
        static const char* iphone640x960 = "iPhoneLaunchImage640x960";
        static const char* iphone640x1136 = "iPhoneLaunchImage640x1136";
        static const char* iphone750x1334 = "iPhoneLaunchImage750x1334";
        static const char* iphone1125x2436 = "iPhoneLaunchImage1125x2436";
        static const char* iphone2436x1125 = "iPhoneLaunchImage2436x1125";
        static const char* iphone1242x2208 = "iPhoneLaunchImage1242x2208";
        static const char* iphone2208x1242 = "iPhoneLaunchImage2208x1242";
        static const char* ipad768x1024 = "iPadLaunchImage768x1024";
        static const char* ipad1024x768 = "iPadLaunchImage1024x768";
        static const char* ipad1536x2048 = "iPadLaunchImage1536x2048";
        static const char* ipad2048x1536 = "iPadLaunchImage2048x1536";
    } // namespace Launchscreens

    void IosLaunchscreens::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<IosLaunchscreens>()
                ->Version(1)
                ->Field(Launchscreens::iphone640x960, &IosLaunchscreens::m_iphone640x960)
                ->Field(Launchscreens::iphone640x1136, &IosLaunchscreens::m_iphone640x1136)
                ->Field(Launchscreens::iphone750x1334, &IosLaunchscreens::m_iphone750x1334)
                ->Field(Launchscreens::iphone1125x2436, &IosLaunchscreens::m_iphone1125x2436)
                ->Field(Launchscreens::iphone2436x1125, &IosLaunchscreens::m_iphone2436x1125)
                ->Field(Launchscreens::iphone1242x2208, &IosLaunchscreens::m_iphone1242x2208)
                ->Field(Launchscreens::iphone2208x1242, &IosLaunchscreens::m_iphone2208x1242)
                ->Field(Launchscreens::ipad768x1024, &IosLaunchscreens::m_ipad768x1024)
                ->Field(Launchscreens::ipad1024x768, &IosLaunchscreens::m_ipad1024x768)
                ->Field(Launchscreens::ipad1536x2048, &IosLaunchscreens::m_ipad1536x2048)
                ->Field(Launchscreens::ipad2048x1536, &IosLaunchscreens::m_ipad2048x1536)
            ;

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<IosLaunchscreens>(
                    QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Launchscreens"),
                    QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "All png launchscreen overrides for iOS."))
                    ->DataElement(Handlers::ImagePreview, &IosLaunchscreens::m_iphone640x960,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone (640x960px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<640, 960>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosLaunchScreens, Launchscreens::iphone640x960))
                    ->DataElement(Handlers::ImagePreview, &IosLaunchscreens::m_iphone640x1136,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone (640x1136px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<640, 1136>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosLaunchScreens, Launchscreens::iphone640x1136))
                    ->DataElement(Handlers::ImagePreview, &IosLaunchscreens::m_iphone750x1334,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone (750x1334px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<750, 1334>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosLaunchScreens, Launchscreens::iphone750x1334))
                    ->DataElement(Handlers::ImagePreview, &IosLaunchscreens::m_iphone1125x2436,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone (1125x2436px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<1125, 2436>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosLaunchScreens, Launchscreens::iphone1125x2436))
                    ->DataElement(Handlers::ImagePreview, &IosLaunchscreens::m_iphone2436x1125,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone (2436x1125px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<2436, 1125>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosLaunchScreens, Launchscreens::iphone2436x1125))
                    ->DataElement(Handlers::ImagePreview, &IosLaunchscreens::m_iphone1242x2208,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone (1242x2208px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<1242, 2208>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosLaunchScreens, Launchscreens::iphone1242x2208))
                    ->DataElement(Handlers::ImagePreview, &IosLaunchscreens::m_iphone2208x1242,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone (2208x1242px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<2208, 1242>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosLaunchScreens, Launchscreens::iphone2208x1242))
                    ->DataElement(Handlers::ImagePreview, &IosLaunchscreens::m_ipad768x1024,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPad (768x1024px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<768, 1024>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosLaunchScreens, Launchscreens::ipad768x1024))
                    ->DataElement(Handlers::ImagePreview, &IosLaunchscreens::m_ipad1024x768,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPad (1024x768px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<1024, 768>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosLaunchScreens, Launchscreens::ipad1024x768))
                    ->DataElement(Handlers::ImagePreview, &IosLaunchscreens::m_ipad1536x2048,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPad (1536x2048px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<1536, 2048>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosLaunchScreens, Launchscreens::ipad1536x2048))
                    ->DataElement(Handlers::ImagePreview, &IosLaunchscreens::m_ipad2048x1536,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPad (2048x1536px)"), "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<2048, 1536>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::IosLaunchScreens, Launchscreens::ipad2048x1536))
                ;
            }
        }
    }

    void IosOrientations::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<IosOrientations>()
                ->Version(1)
                ->Field("UIInterfaceOrientationLandscapeRight", &IosOrientations::m_landscapeRight)
                ->Field("UIInterfaceOrientationLandscapeLeft", &IosOrientations::m_landscapeLeft)
                ->Field("UIInterfaceOrientationPortrait", &IosOrientations::m_portraitBottom)
                ->Field("UIInterfaceOrientationPortraitUpsideDown", &IosOrientations::m_portraitTop)
            ;

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<IosOrientations>(
                    QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Orientations"),
                    QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "All supported orientations for iOS."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &IosOrientations::m_landscapeRight,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Landscape (right home button)"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Enable landscape orientation with home button on right side of device."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &IosOrientations::m_landscapeLeft,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Landscape (left home button)"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Enable landscape orientation with home button on left side of device."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &IosOrientations::m_portraitBottom,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Portrait (bottom home button)"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Enable portrait orientation with home button on bottom of device."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &IosOrientations::m_portraitTop,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Portrait (top home button)"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Enable portrait orientation with home button on top of device."))
                ;
            }
        }
    }

    void IosSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            IosOrientations::Reflect(context);
            IosIcons::Reflect(context);
            IosLaunchscreens::Reflect(context);

            serialize->Class<IosSettings>()
                ->Version(1)
                ->Field("CFBundleName", &IosSettings::m_bundleName)
                ->Field("CFBundleDisplayName", &IosSettings::m_bundleDisplayName)
                ->Field("CFBundleExecutable", &IosSettings::m_executableName)
                ->Field("CFBundleIdentifier", &IosSettings::m_bundleIdentifier)
                ->Field("CFBundleShortVersionString", &IosSettings::m_versionName)
                ->Field("CFBundleVersion", &IosSettings::m_versionNumber)
                ->Field("CFBundleDevelopmentRegion", &IosSettings::m_developmentRegion)
                ->Field("UIRequiresFullScreen", &IosSettings::m_requiresFullscreen)
                ->Field("UIStatusBarHidden", &IosSettings::m_hideStatusBar)
                ->Field("UISupportedInterfaceOrientations", &IosSettings::m_iphoneOrientations)
                ->Field("UISupportedInterfaceOrientations~ipad", &IosSettings::m_ipadOrientations)
                ->Field("icons", &IosSettings::m_icons)
                ->Field("launchscreens", &IosSettings::m_launchscreens)
            ;

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<IosSettings>(
                    QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iOS Settings"),
                    QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "All iOS settings."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(Handlers::LinkedLineEdit, &IosSettings::m_bundleName,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Bundle Name"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "The name of the bundle."))
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::IOSFileName))
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::IosBundleName)
                    ->DataElement(Handlers::LinkedLineEdit, &IosSettings::m_bundleDisplayName,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Display Name"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "The user visible name of the bundle."))
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::IsNotEmpty))
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::IosDisplayName)
                    ->DataElement(Handlers::LinkedLineEdit, &IosSettings::m_executableName,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Executable Name"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Name of the bundle's executable file."))
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::IOSFileName))
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::IosExecutableName)
                    ->DataElement(Handlers::LinkedLineEdit, &IosSettings::m_bundleIdentifier,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Bundle Identifier"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Uniquely identifies the bundle. Should be in reverse-DNS format."))
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PackageName))
                        ->Attribute(Attributes::LinkOptional, true)
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::IosBundleIdentifer)
                    ->DataElement(Handlers::LinkedLineEdit, &IosSettings::m_versionName,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Version Name"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "The release version number string for the app. Displayed in the app store."))
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::IOSVersionNumber))
                        ->Attribute(Attributes::LinkOptional, true)
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::IosVersionName)
                    ->DataElement(Handlers::QValidatedLineEdit, &IosSettings::m_versionNumber,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Version Number"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "The build version number string for the bundle."))
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::IOSVersionNumber))
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &IosSettings::m_developmentRegion,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Development Region"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "The default language and region for the app."))
                        ->Attribute(AZ::Edit::Attributes::StringList, AZStd::vector<AZStd::string>
                        {
                            "en_US",
                            "en_CA",
                            "fr_CA",
                            "zh_CN",
                            "fr_FR",
                            "de_DE",
                            "it_IT",
                            "ja_JP",
                            "ko_KR",
                            "zh_TW",
                            "en_GB"
                        })
                    ->DataElement(AZ::Edit::UIHandlers::Default, &IosSettings::m_requiresFullscreen,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Requires Fullscreen"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Specifies whether the app is required to run in fullscreen mode."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &IosSettings::m_hideStatusBar,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Hide Status Bar"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Specifies whether the status bar is initially hidden when the app launches."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &IosSettings::m_iphoneOrientations,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPhone Orientations"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Enable support for iPhone orientations."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &IosSettings::m_ipadOrientations,
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "iPad Orientations"),
                        QT_TRANSLATE_NOOP("ReflectedPropertyEditor", "Enable support for iPad orientations."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &IosSettings::m_icons)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &IosSettings::m_launchscreens)
                ;
            }
        }
    }
} // namespace ProjectSettingsTool
