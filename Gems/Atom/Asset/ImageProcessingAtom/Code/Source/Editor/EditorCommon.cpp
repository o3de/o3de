/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImageProcessing_Traits_Platform.h>
#include <Editor/EditorCommon.h>
#include <Processing/ImageToProcess.h>
#include <ImageLoader/ImageLoaders.h>
#include <BuilderSettings/TextureSettings.h>
#include <BuilderSettings/PresetSettings.h>
#include <Atom/ImageProcessing/PixelFormats.h>
#include <Converters/Cubemap.h>
#include <Processing/ImageConvert.h>
#include <AzCore/std/string/conversions.h>

#include <AzFramework/StringFunc/StringFunc.h>

namespace ImageProcessingAtomEditor
{
    using namespace ImageProcessingAtom;

    bool EditorHelper::s_IsPixelFormatStringInited = false;
    const char* EditorHelper::s_PixelFormatString[ImageProcessingAtom::EPixelFormat::ePixelFormat_Count];
    
    void EditorHelper::InitPixelFormatString()
    {
        if (!s_IsPixelFormatStringInited)
        {
            s_IsPixelFormatStringInited = true;
        }

        CPixelFormats& pixelFormats = CPixelFormats::GetInstance();
        for (int format = 0; format < EPixelFormat::ePixelFormat_Count; format ++)
        {
            const PixelFormatInfo* info = pixelFormats.GetPixelFormatInfo((EPixelFormat)format);
            s_PixelFormatString[(EPixelFormat)format] = "";
            if (info)
            {
                s_PixelFormatString[(EPixelFormat)format] = info->szName;
            }
            else
            {
                AZ_Error("Texture Editor", false, "Cannot find name of EPixelFormat %i", format);
            }
        }
    }

    const AZStd::string EditorHelper::GetFileSizeString(size_t fileSizeInBytes)
    {
        AZStd::string fileSizeStr;

        static double kb = 1024.0f;
        static double mb = kb * 1024.0;
        static double gb = mb * 1024.0;

        static AZStd::fixed_string<2> byteStr = "B";
        static AZStd::fixed_string<3> kbStr = "KB";
        static AZStd::fixed_string<3> mbStr = "MB";
        static AZStd::fixed_string<3> gbStr = "GB";

#if AZ_TRAIT_IMAGEPROCESSING_USE_BASE10_BYTE_PREFIX
        kb = 1000.0;
        mb = kb * 1000.0;
        gb = mb * 1000.0;

        kbStr = "kB";
        mbStr = "mB";
        gbStr = "gB";
#endif // AZ_TRAIT_IMAGEPROCESSING_USE_BASE10_BYTE_PREFIX

        if (fileSizeInBytes < kb)
        {
            fileSizeStr = AZStd::string::format("%zu %s", fileSizeInBytes, byteStr.c_str());
        }
        else if (fileSizeInBytes < mb)
        {
            double size = fileSizeInBytes / kb;
            fileSizeStr = AZStd::string::format("%.2f %s", size, kbStr.c_str());
        }
        else if (fileSizeInBytes < gb)
        {
            double size = fileSizeInBytes / mb;
            fileSizeStr = AZStd::string::format("%.2f %s", size, mbStr.c_str());
        }
        else
        {
            double size = fileSizeInBytes / gb;
            fileSizeStr = AZStd::string::format("%.2f %s", size, gbStr.c_str());
        }
        return fileSizeStr;
    }

    const AZStd::string EditorHelper::ToReadablePlatformString(const AZStd::string& platformRawStr)
    {
        AZStd::string readableString;
        AZStd::string platformStrLowerCase = platformRawStr;
        AZStd::to_lower(platformStrLowerCase.begin(), platformStrLowerCase.end());
        if (platformStrLowerCase == "pc")
        {
            readableString = "PC";
        }
        else if (platformStrLowerCase == "linux")
        {
            readableString = "Linux";
        }
        else if (platformStrLowerCase == "android")
        {
            readableString = "Android";
        }
        else if (platformStrLowerCase == "mac")
        {
            readableString = "macOS";
        }
        else if (platformStrLowerCase == "provo")
        {
            readableString = "Provo";
        }
        else if (platformStrLowerCase == "ios")
        {
            readableString = "iOS";
        }
        else if (platformStrLowerCase == "salem")
        {
            readableString = "Salem";
        }
        else if (platformStrLowerCase == "jasper")
        {
            readableString = "Jasper";
        }
        else
        {
            return platformRawStr;
        }

        return readableString;
    }


    EditorTextureSetting::EditorTextureSetting(const AZ::Uuid& sourceTextureId)
    {
        const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* fullDetails = AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry::GetSourceByUuid(sourceTextureId);
        if (fullDetails)
        {
            InitFromPath(fullDetails->GetFullPath());
        }
    }

    EditorTextureSetting::EditorTextureSetting(const AZStd::string& texturePath)
    {
        InitFromPath(texturePath);
    }
    
    void EditorTextureSetting::InitFromPath(const AZStd::string& texturePath)
    {
        m_fullPath = texturePath;
        AzFramework::StringFunc::Path::GetFullFileName(texturePath.c_str(), m_textureName);

        m_img = IImageObjectPtr(LoadImageFromFile(m_fullPath));

        if (m_img == nullptr)
        {
            AZ_Warning("Texture Editor", false, "%s is not a valid texture image.", texturePath.c_str());
            return;
        }

        bool generatedDefaults = false;
        m_settingsMap = TextureSettings::GetMultiplatformTextureSetting(m_fullPath, generatedDefaults);

        // Get the preset id from one platform. The preset id for each platform should always be same
        AZ_Assert(m_settingsMap.size() > 0, "There is no platform information");
        PresetName presetName = m_settingsMap.begin()->second.m_preset;
        const PresetSettings* preset = BuilderSettingManager::Instance()->GetPreset(presetName);

        if (!preset)
        {
            AZ_Warning("Texture Editor", false, "Cannot find preset %s! Will assign a suggested one for the texture.", presetName.GetCStr());
            presetName = BuilderSettingManager::Instance()->GetSuggestedPreset(m_fullPath);

            for (auto& settingIter : m_settingsMap)
            {
                settingIter.second.ApplyPreset(presetName);
            }
        }
    }
    
    void EditorTextureSetting::SetIsOverrided()
    {
        for (auto& it : m_settingsMap)
        {
            m_overrideFromPreset = false;
            TextureSettings& textureSetting = it.second;
            const PresetSettings* presetSetting = BuilderSettingManager::Instance()->GetPreset(textureSetting.m_preset);
            if (presetSetting != nullptr)
            {
                if ((textureSetting.m_sizeReduceLevel != presetSetting->m_sizeReduceLevel) ||
                    (textureSetting.m_suppressEngineReduce != presetSetting->m_suppressEngineReduce) ||
                    (presetSetting->m_mipmapSetting != nullptr && textureSetting.m_mipGenType != presetSetting->m_mipmapSetting->m_type))
                {
                    m_overrideFromPreset = true;
                }
            }
            else
            {
                AZ_Error("Texture Editor", false, "Texture Preset %s is not found!", textureSetting.m_preset.GetCStr());
            }
        }
    }

    void EditorTextureSetting::SetToPreset(const PresetName& presetName)
    {
        m_overrideFromPreset = false;

        for (auto& settingIter : m_settingsMap)
        {
            settingIter.second.ApplyPreset(presetName);
        }
    }
    
    //Get the texture setting on certain platform
    TextureSettings& EditorTextureSetting::GetMultiplatformTextureSetting(const AZStd::string& platform)
    {
        AZ_Assert(m_settingsMap.size() > 0, "Texture Editor", "There is no texture settings for texture %s", m_fullPath.c_str());
        PlatformName platformName = platform;
        if (platform.empty())
        {
            platformName = BuilderSettingManager::s_defaultPlatform;
        }
        if (m_settingsMap.find(platformName) != m_settingsMap.end())
        {
            return m_settingsMap[platformName];
        }
        else
        {
            AZ_Error("Texture Editor", false, "Cannot find texture setting on platform %s", platformName.c_str());
        }
        return m_settingsMap.begin()->second;
    }

    bool EditorTextureSetting::GetFinalInfoForTextureOnPlatform(const AZStd::string& platform, AZ::u32 wantedReduce, ResolutionInfo& outResolutionInfo)
    {
        if (m_settingsMap.find(platform) == m_settingsMap.end())
        {
            return false;
        }

        // Copy current texture setting and set to desired reduce
        TextureSettings textureSetting = m_settingsMap[platform];
        wantedReduce = AZStd::min<int>(AZStd::max<int>(s_MinReduceLevel, wantedReduce), s_MaxReduceLevel);
        textureSetting.m_sizeReduceLevel = wantedReduce;

        const PresetSettings* presetSetting = BuilderSettingManager::Instance()->GetPreset(textureSetting.m_preset, platform);
        if (presetSetting)
        {
            EPixelFormat pixelFormat = presetSetting->m_pixelFormat;
            CPixelFormats& pixelFormats = CPixelFormats::GetInstance();

            AZ::u32 inputWidth = m_img->GetWidth(0);
            AZ::u32 inputHeight = m_img->GetHeight(0);

            // Update input width and height if it's a cubemap
            if (presetSetting->m_cubemapSetting != nullptr)
            {
                if (IsValidLatLongMap(m_img))
                {
                    inputWidth = inputWidth/4;
                }
                else
                {
                    CubemapLayout *srcCubemap = CubemapLayout::CreateCubemapLayout(m_img);
                    if (srcCubemap == nullptr)
                    {
                        return false;
                    }
                    inputWidth = srcCubemap->GetFaceSize();
                    delete srcCubemap;
                }
                inputHeight = inputWidth;
                outResolutionInfo.arrayCount = 6;
            }

            GetOutputExtent(inputWidth, inputHeight, outResolutionInfo.width, outResolutionInfo.height, outResolutionInfo.reduce, &textureSetting, presetSetting);

            AZ::u32 mipMapCount = pixelFormats.ComputeMaxMipCount(pixelFormat, outResolutionInfo.width, outResolutionInfo.height);
            outResolutionInfo.mipCount = presetSetting->m_mipmapSetting != nullptr && textureSetting.m_enableMipmap ? mipMapCount : 1;

            return true;
        }
        else
        {
            return false;
        }
    }

    bool EditorTextureSetting::RefreshMipSetting(bool enableMip)
    {
        bool enabled = true;
        for (auto& it : m_settingsMap)
        {
            const PresetSettings* preset = BuilderSettingManager::Instance()->GetPreset(it.second.m_preset);
            if (enableMip)
            {
                if (preset && preset->m_mipmapSetting)
                {
                    it.second.m_enableMipmap = true;
                    it.second.m_mipGenType = preset->m_mipmapSetting->m_type;
                }
                else
                {
                    it.second.m_enableMipmap = false;
                    enabled = false;
                    AZ_Error("Texture Editor", false, "Preset %s does not support mipmap!", preset->m_name.GetCStr());
                }
            }
            else
            {
                it.second.m_enableMipmap = false;
                enabled = false;
            }
        }
        return enabled;
    }

    void EditorTextureSetting::PropagateCommonSettings()
    {
        if (m_settingsMap.size() <= 1)
        {
            //Only one setting available, no need to propagate
            return;
        }

        TextureSettings& texSetting = GetMultiplatformTextureSetting();
        for (auto& it = ++ m_settingsMap.begin(); it != m_settingsMap.end(); ++it)
        {
            const PlatformName defaultPlatform = BuilderSettingManager::s_defaultPlatform;
            if (it->first != defaultPlatform)
            {
                it->second.m_enableMipmap = texSetting.m_enableMipmap;
                it->second.m_maintainAlphaCoverage = texSetting.m_maintainAlphaCoverage;
                it->second.m_mipGenEval = texSetting.m_mipGenEval;
                it->second.m_mipGenType = texSetting.m_mipGenType;
                for (size_t i = 0; i < TextureSettings::s_MaxMipMaps; i++)
                {
                    it->second.m_mipAlphaAdjust[i] = texSetting.m_mipAlphaAdjust[i];
                }
            }
        }
    }

    AZStd::list<ResolutionInfo> EditorTextureSetting::GetResolutionInfo(AZStd::string platform, AZ::u32& minReduce, AZ::u32& maxReduce)
    {
        AZStd::list<ResolutionInfo> resolutionInfos;
        // Set the min/max reduce to the global value range first
        minReduce = s_MaxReduceLevel;
        maxReduce = s_MinReduceLevel;
        for (AZ::u32 i = s_MinReduceLevel; i <= s_MaxReduceLevel; i++)
        {
            ResolutionInfo resolutionInfo;
            GetFinalInfoForTextureOnPlatform(platform, i, resolutionInfo);
            // If actual reduce is lower than desired reduce, it reaches the limit and we can stop try lower resolution
            if (i > resolutionInfo.reduce)
            {
                break;
            }
            // Finds out the final min/max reduce based on range in different platforms
            minReduce = AZStd::min<AZ::u32>(resolutionInfo.reduce, minReduce);
            maxReduce = AZStd::max<AZ::u32>(resolutionInfo.reduce, maxReduce);
            resolutionInfos.push_back(resolutionInfo);
        }
        return resolutionInfos;
    }

    AZStd::list<ResolutionInfo> EditorTextureSetting::GetResolutionInfoForMipmap(AZStd::string platform)
    {
        AZStd::list<ResolutionInfo> resolutionInfos;
        unsigned int baseReduce = m_settingsMap[platform].m_sizeReduceLevel;
        ResolutionInfo baseInfo;
        GetFinalInfoForTextureOnPlatform(platform, baseReduce, baseInfo);
        resolutionInfos.push_back(baseInfo);
        for (AZ::u32 i = 1; i < baseInfo.mipCount; i++)
        { 
            ResolutionInfo resolutionInfo = baseInfo;
            resolutionInfo.width = AZStd::max<AZ::u32>(baseInfo.width >> i, 1);
            resolutionInfo.height = AZStd::max<AZ::u32>(baseInfo.height >> i, 1);
            resolutionInfo.reduce = baseInfo.reduce + i;
            resolutionInfo.mipCount = 1;
            resolutionInfos.push_back(resolutionInfo);
        }
        return resolutionInfos;
    }

} //namespace ImageProcessingAtomEditor

