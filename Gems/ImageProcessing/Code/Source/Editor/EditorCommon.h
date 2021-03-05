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

#include <BuilderSettings/BuilderSettingManager.h>
#include <BuilderSettings/TextureSettings.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <Processing/PixelFormatInfo.h>
#include <ImageProcessing/ImageObject.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <QImage>
namespace ImageProcessingEditor
{
    class EditorHelper
    {
    public:
        static const char* s_PixelFormatString[ImageProcessing::EPixelFormat::ePixelFormat_Count];
        static void InitPixelFormatString();
        static const AZStd::string GetFileSizeString(AZ::u32 fileSizeInBytes);
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
        ImageProcessing::MultiplatformTextureSettings m_settingsMap;
        bool m_overrideFromPreset = false;
        bool m_modified = false;
        ImageProcessing::IImageObjectPtr m_img;

        EditorTextureSetting(const AZ::Uuid& sourceTextureId);
        EditorTextureSetting(const AZStd::string& texturePath);
        ~EditorTextureSetting() = default;

        void InitFromPath(const AZStd::string& texturePath);
        
        void SetIsOverrided();
        
        void SetToPreset(const AZStd::string& presetName);
        
        //Get the texture setting on certain platform
        ImageProcessing::TextureSettings& GetMultiplatformTextureSetting(const AZStd::string& platform = "");

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

} //namespace ImageProcessingEditor

