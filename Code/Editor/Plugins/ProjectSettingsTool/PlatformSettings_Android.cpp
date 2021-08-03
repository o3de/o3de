/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ProjectSettingsTool_precompiled.h"
#include "PlatformSettings_Android.h"

#include "PlatformSettings_common.h"
#include "Validators.h"

namespace ProjectSettingsTool
{
    static const char* defaultImageTooltip = "Default image used if a specific DPI override is not given.";
    static void* xmlFunctor = reinterpret_cast<void*>(&SelectXmlFromFileDialog);

    void AndroidIcons::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AndroidIcons>()
                ->Version(1)
                ->Field("default", &AndroidIcons::m_default)
                ->Field("mdpi", &AndroidIcons::m_mdpi)
                ->Field("hdpi", &AndroidIcons::m_hdpi)
                ->Field("xhdpi", &AndroidIcons::m_xhdpi)
                ->Field("xxhdpi", &AndroidIcons::m_xxhdpi)
                ->Field("xxxhdpi", &AndroidIcons::m_xxxhdpi)
            ;

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<AndroidIcons>("Icons", "All icon overrides for android.")
                    ->DataElement(Handlers::ImagePreview, &AndroidIcons::m_default, "Default", defaultImageTooltip)
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::ValidPngOrEmpty))
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::AndroidIconDefault)
                    ->DataElement(Handlers::ImagePreview, &AndroidIcons::m_mdpi, "Medium Dpi (48px)", "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<48>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::AndroidIcons, "mdpi"))
                        ->Attribute(Attributes::DefaultImagePreview, Identfiers::AndroidIconDefault)
                    ->DataElement(Handlers::ImagePreview, &AndroidIcons::m_hdpi, "High Dpi (72px)", "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<72>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::AndroidIcons, "hdpi"))
                        ->Attribute(Attributes::DefaultImagePreview, Identfiers::AndroidIconDefault)
                    ->DataElement(Handlers::ImagePreview, &AndroidIcons::m_xhdpi, "XHigh Dpi (96px)", "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<96>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::AndroidIcons, "xhdpi"))
                        ->Attribute(Attributes::DefaultImagePreview, Identfiers::AndroidIconDefault)
                    ->DataElement(Handlers::ImagePreview, &AndroidIcons::m_xxhdpi, "XXHigh Dpi (144px)", "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<144>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::AndroidIcons, "xxhdpi"))
                        ->Attribute(Attributes::DefaultImagePreview, Identfiers::AndroidIconDefault)
                    ->DataElement(Handlers::ImagePreview, &AndroidIcons::m_xxxhdpi, "XXXHigh Dpi (192px)", "")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PngImageSetSizeOrEmpty<192>))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::AndroidIcons, "xxxhdpi"))
                        ->Attribute(Attributes::DefaultImagePreview, Identfiers::AndroidIconDefault)
                ;
            }
        }
    }

    void AndroidLandscapeSplashscreens::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AndroidLandscapeSplashscreens>()
                ->Version(1)
                ->Field("default", &AndroidLandscapeSplashscreens::m_default)
                ->Field("mdpi", &AndroidLandscapeSplashscreens::m_mdpi)
                ->Field("hdpi", &AndroidLandscapeSplashscreens::m_hdpi)
                ->Field("xhdpi", &AndroidLandscapeSplashscreens::m_xhdpi)
                ->Field("xxhdpi", &AndroidLandscapeSplashscreens::m_xxhdpi)
            ;

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<AndroidLandscapeSplashscreens>("Landscape", "All landscape splashscreen overrides for Android.")
                    ->DataElement(Handlers::ImagePreview, &AndroidLandscapeSplashscreens::m_default, "Default", defaultImageTooltip)
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::ValidPngOrEmpty))
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::AndroidLandDefault)
                    ->DataElement(Handlers::ImagePreview, &AndroidLandscapeSplashscreens::m_mdpi, "Medium Dpi", "Suggested 1024 x 640 png.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::ValidPngOrEmpty))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::AndroidLandscape, "mdpi"))
                        ->Attribute(Attributes::DefaultImagePreview, Identfiers::AndroidLandDefault)
                    ->DataElement(Handlers::ImagePreview, &AndroidLandscapeSplashscreens::m_hdpi, "High Dpi", "Suggested 1280 x 800 png.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::ValidPngOrEmpty))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::AndroidLandscape, "hdpi"))
                        ->Attribute(Attributes::DefaultImagePreview, Identfiers::AndroidLandDefault)
                    ->DataElement(Handlers::ImagePreview, &AndroidLandscapeSplashscreens::m_xhdpi, "XHigh Dpi", "Suggested 1920 x 1200 png.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::ValidPngOrEmpty))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::AndroidLandscape, "xhdpi"))
                        ->Attribute(Attributes::DefaultImagePreview, Identfiers::AndroidLandDefault)
                    ->DataElement(Handlers::ImagePreview, &AndroidLandscapeSplashscreens::m_xxhdpi, "XXHigh Dpi", "Suggested 2560 x 1600 png.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::ValidPngOrEmpty))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::AndroidLandscape, "xxhdpi"))
                        ->Attribute(Attributes::DefaultImagePreview, Identfiers::AndroidLandDefault)
                ;
            }
        }
    }

    void AndroidPortraitSplashscreens::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AndroidPortraitSplashscreens>()
                ->Version(1)
                ->Field("default", &AndroidPortraitSplashscreens::m_default)
                ->Field("mdpi", &AndroidPortraitSplashscreens::m_mdpi)
                ->Field("hdpi", &AndroidPortraitSplashscreens::m_hdpi)
                ->Field("xhdpi", &AndroidPortraitSplashscreens::m_xhdpi)
                ->Field("xxhdpi", &AndroidPortraitSplashscreens::m_xxhdpi)
            ;

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<AndroidPortraitSplashscreens>("Portrait", "All portrait splashscreen overrides for Android.")
                    ->DataElement(Handlers::ImagePreview, &AndroidPortraitSplashscreens::m_default, "Default", defaultImageTooltip)
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::ValidPngOrEmpty))
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::AndroidPortDefault)
                    ->DataElement(Handlers::ImagePreview, &AndroidPortraitSplashscreens::m_mdpi, "Medium Dpi", "Suggested 640 x 1024 png.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::ValidPngOrEmpty))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::AndroidPortrait, "mdpi"))
                        ->Attribute(Attributes::DefaultImagePreview, Identfiers::AndroidPortDefault)
                    ->DataElement(Handlers::ImagePreview, &AndroidPortraitSplashscreens::m_hdpi, "High Dpi", "Suggested 800 x 1280 png.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::ValidPngOrEmpty))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::AndroidPortrait, "hdpi"))
                        ->Attribute(Attributes::DefaultImagePreview, Identfiers::AndroidPortDefault)
                    ->DataElement(Handlers::ImagePreview, &AndroidPortraitSplashscreens::m_xhdpi, "XHigh Dpi", "Suggested 1200 x 1920 png.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::ValidPngOrEmpty))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::AndroidPortrait, "xhdpi"))
                        ->Attribute(Attributes::DefaultImagePreview, Identfiers::AndroidPortDefault)
                    ->DataElement(Handlers::ImagePreview, &AndroidPortraitSplashscreens::m_xxhdpi, "XXHigh Dpi", "Suggested 1600 x 2560 png.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::ValidPngOrEmpty))
                        ->Attribute(Attributes::DefaultPath, GenDefaultImagePath(ImageGroup::AndroidPortrait, "xxhdpi"))
                        ->Attribute(Attributes::DefaultImagePreview, Identfiers::AndroidPortDefault)
                ;
            }
        }
    }

    void AndroidSplashscreens::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            AndroidLandscapeSplashscreens::Reflect(context);
            AndroidPortraitSplashscreens::Reflect(context);

            serialize->Class<AndroidSplashscreens>()
                ->Version(1)
                ->Field("land", &AndroidSplashscreens::m_landscapeSplashscreens)
                ->Field("port", &AndroidSplashscreens::m_portraitSplashscreens)
            ;

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<AndroidSplashscreens>("Splashscreens", "All splashscreen overrides for Android.")
                    ->DataElement(0, &AndroidSplashscreens::m_landscapeSplashscreens)
                    ->DataElement(0, &AndroidSplashscreens::m_portraitSplashscreens)
                ;
            }
        }
    }

    void AndroidSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            AndroidIcons::Reflect(context);
            AndroidSplashscreens::Reflect(context);

            serialize->Class<AndroidSettings>()
                ->Version(1)
                ->Field("package_name", &AndroidSettings::m_packageName)
                ->Field("version_name", &AndroidSettings::m_versionName)
                ->Field("version_number", &AndroidSettings::m_versionNumber)
                ->Field("orientation", &AndroidSettings::m_orientation)
                ->Field("app_public_key", &AndroidSettings::m_appPublicKey)
                ->Field("app_obfuscator_salt", &AndroidSettings::m_appObfuscatorSalt)
                ->Field("rc_pak_job", &AndroidSettings::m_rcPakJob)
                ->Field("rc_obb_job", &AndroidSettings::m_rcObbJob)
                ->Field("use_main_obb", &AndroidSettings::m_useMainObb)
                ->Field("use_patch_obb", &AndroidSettings::m_usePatchObb)
                ->Field("enable_key_screen_on", &AndroidSettings::m_enableKeyScreenOn)
                ->Field("disable_immersive_mode", &AndroidSettings::m_disableImmersiveMode)
                ->Field("icons", &AndroidSettings::m_icons)
                ->Field("splash_screen", &AndroidSettings::m_splashscreens)
            ;

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<AndroidSettings>("Android Settings", "All settings related to Android not already defined by base settings.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(Handlers::LinkedLineEdit, &AndroidSettings::m_packageName, "Package Name", "Android application package identifier. Used for generating the project specific Java activity class and in the AndroidManifest.xml. Must be in dot separated format.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PackageName))
                        ->Attribute(Attributes::LinkOptional, true)
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::AndroidPackageName)
                        ->Attribute(Attributes::LinkedProperty, Identfiers::IosBundleIdentifer)
                    ->DataElement(Handlers::LinkedLineEdit, &AndroidSettings::m_versionName, "Version Name", "Human readable version number. Used to set the \"android: versionName\" tag in the AndroidManifest.xml and ultimately what will be displayed in the App Store.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::IOSVersionNumber))
                        ->Attribute(Attributes::LinkOptional, true)
                        ->Attribute(Attributes::PropertyIdentfier, Identfiers::AndroidVersionName)
                        ->Attribute(Attributes::LinkedProperty, Identfiers::IosVersionName)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AndroidSettings::m_versionNumber, "Version Number", "Internal application version number. Used to set the \"android:versionCode\" tag in the AndroidManifest.xml.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, Validators::maxAndroidVersion)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AndroidSettings::m_orientation, "Orientation", "Desired orientation of the Android application. Used to set the \"android:screenOrientation\" tag in the AndroidManifest.xml.")
                        ->Attribute(AZ::Edit::Attributes::StringList, AZStd::vector<AZStd::string>
                        {
                            "landscape",
                            "portrait",
                            "reverseLandscape",
                            "reversePortrait",
                            "sensorLandscape",
                            "sensorPortrait",
                            "sensor",
                            "fullSensor",
                            "noSensor",
                            "userLandscape",
                            "userPortrait",
                            "user",
                            "fullUser",
                            "locked",
                            "behind",
                            "unspecified"
                        })
                    ->DataElement(Handlers::LinkedLineEdit, &AndroidSettings::m_appPublicKey, "Public App Key", "The application license key provided by Google Play. Required for using APK expansion files or other Google Play Services.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::PublicAppKeyOrEmpty))
                        ->Attribute(Attributes::Obfuscated, true)
                    ->DataElement(Handlers::LinkedLineEdit, &AndroidSettings::m_appObfuscatorSalt, "App Obfuscation Salt", "Application specific salt value for (un)obfuscation when using APK expansion files.")
                        ->Attribute(Attributes::Obfuscated, true)
                    ->DataElement(Handlers::FileSelect, &AndroidSettings::m_rcPakJob, "Rc Job PAK Override", "Path to the RC job XML file used to override the normal PAK files generation used in release builds. Path must be relative to <build dir>.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::ValidXmlOrEmpty))
                        ->Attribute(Attributes::SelectFunction, xmlFunctor)
                    ->DataElement(Handlers::FileSelect, &AndroidSettings::m_rcObbJob, "Rc Job APK Override", "Path to the RC job XML file used to override the normal APK Expansion file(s) generation used in release builds. Path must be relative to <build dir>.")
                        ->Attribute(Attributes::FuncValidator, ConvertFunctorToVoid(&Validators::ValidXmlOrEmpty))
                        ->Attribute(Attributes::SelectFunction, xmlFunctor)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AndroidSettings::m_useMainObb, "Use Main APK", "Specify if the \"Main\" APK Expansion file should be used.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AndroidSettings::m_usePatchObb, "Use Patch APK", "Specify if the \"Patch\" APK Expansion file should be used.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AndroidSettings::m_enableKeyScreenOn, "Enable Screen Wake Lock", "Enabled or disable the screen wake lock (device won't go to sleep while the application is running).")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AndroidSettings::m_disableImmersiveMode, "Disable Immersive Mode", "Disable hiding of top and bottom system bars.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AndroidSettings::m_icons)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AndroidSettings::m_splashscreens)
                ;
            }
        }
    }
} // namespace ProjectSettingsTool
