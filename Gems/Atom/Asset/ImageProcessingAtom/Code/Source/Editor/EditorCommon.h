/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BuilderSettings/BuilderSettingManager.h>
#include <BuilderSettings/TextureSettings.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <Processing/PixelFormatInfo.h>
#include <Atom/ImageProcessing/ImageObject.h>
#include <AzCore/std/smart_ptr/make_shared.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QImage>
#include <QWidget>
#include <QMainWindow>
#include <QApplication>
AZ_POP_DISABLE_WARNING

namespace ImageProcessingAtomEditor
{
    class EditorHelper
    {
    public:
        static const char* s_PixelFormatString[ImageProcessingAtom::EPixelFormat::ePixelFormat_Count];
        static void InitPixelFormatString();
        static const AZStd::string GetFileSizeString(size_t fileSizeInBytes);
        static const AZStd::string ToReadablePlatformString(const AZStd::string& platformRawStr);

    private:
        static bool s_IsPixelFormatStringInited;
    };

    struct ResolutionInfo
    {
        AZ::u32 width = 0;
        AZ::u32 height = 0;
        AZ::u32 arrayCount = 1;
        AZ::u32 reduce = 0;
        AZ::u32 mipCount = 0;
    };

    struct EditorTextureSetting
    {
        AZStd::string m_textureName = "";
        AZStd::string m_fullPath = "";
        ImageProcessingAtom::MultiplatformTextureSettings m_settingsMap;
        bool m_overrideFromPreset = false;
        bool m_modified = false;
        ImageProcessingAtom::IImageObjectPtr m_img;

        EditorTextureSetting(const AZ::Uuid& sourceTextureId);
        EditorTextureSetting(const AZStd::string& texturePath);
        ~EditorTextureSetting() = default;

        void InitFromPath(const AZStd::string& texturePath);
        
        void SetIsOverrided();
        
        void SetToPreset(const ImageProcessingAtom::PresetName& presetName);
        
        //Get the texture setting on certain platform
        ImageProcessingAtom::TextureSettings& GetMultiplatformTextureSetting(const AZStd::string& platform = "");

        //Gets the final resolution/reduce/mip count for a texture on a certain platform
        //@param wantedReduce indicates the reduce level that's preferred
        //@return successfully get the value or not
        bool GetFinalInfoForTextureOnPlatform(const AZStd::string& platform, AZ::u32 wantedReduce, ResolutionInfo& outResolutionInfo);

        //Refresh the mip setting when the mip map setting is enabled/disabled.
        //@return whether the mipmap is enabled or not.
        bool RefreshMipSetting(bool enableMip);

        //Propagate non platform specific settings from the first setting to all the settings stored in m_settingsMap
        void PropagateCommonSettings();

        //Returns a list of calculated final resolution info based on different base reduce levels
        AZStd::list<ResolutionInfo> GetResolutionInfo(AZStd::string platform, AZ::u32& minReduce, AZ::u32& maxReduce);

        //Returns a list of calculated final resolution info based on different mipmap levels
        AZStd::list<ResolutionInfo> GetResolutionInfoForMipmap(AZStd::string platform);
    };
    

    class ImageProcessingEditorInteralNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        /////////////////////////////////////////////////////////////////////////

        //! Used to inform the settings changed across widgets
        virtual void OnEditorSettingsChanged(bool needRefresh, const AZStd::string& platform) = 0;
    };

    using EditorInternalNotificationBus = AZ::EBus<ImageProcessingEditorInteralNotifications>;

} //namespace ImageProcessingAtomEditor

