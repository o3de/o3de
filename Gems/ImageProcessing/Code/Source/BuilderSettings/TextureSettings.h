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

#include <BuilderSettings/ImageProcessingDefines.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/DataPatch.h>

namespace ImageProcessing
{
    class TextureSettings;
    typedef AZStd::map<PlatformName, TextureSettings> MultiplatformTextureSettings;

    class TextureSettings
    {
    public:
        AZ_TYPE_INFO(TextureSettings, "{CC3ED018-7FF7-4233-AAD8-6D3115FD844A}");
        AZ_CLASS_ALLOCATOR(TextureSettings, AZ::SystemAllocator, 0);

        TextureSettings();
        float ComputeMIPAlphaOffset(AZ::u32 mip) const;
        void ApplyPreset(AZ::Uuid presetId);
        
        /**
        * Performs a comprehensive comparison between two TextureSettings instances.
        * @param Reference to the settings which will be compared.
        * @param Optional. Serialize context. Will use global context if none is provided.
        * @return True, is both instances are equivalent.
        */
        bool Equals(const TextureSettings& other, AZ::SerializeContext* serializeContext = nullptr);

        /**
        * Applies texture settings to the instance (including overrides). Common settings are applied, unless specific platform is specified.
        * @param Reference to the settings which will be applied.
        * @param Optional. Applies settings as a platform override if a platform is specified.
        * @param Optional. Serialize context. Will use global context if none is provided.
        * @return Status outcome result.
        */
        StringOutcome ApplySettings(const TextureSettings& settings, const PlatformName& overridePlatform = PlatformName(), AZ::SerializeContext* serializeContext = nullptr);
        
        /**
        * Gets platform-specific texture settings obtained from the base settings version of a pre-loaded TextureSettings instance.
        * @param Name of platform to get the settings from.
        * @param Base TextureSettings which we will get overrides from.
        * @param Output TextureSettings which will contain the result of the function.
        * @param Optional. Serialize context. Will use global context if none is provided.
        * @return Status outcome result.
        */
        static StringOutcome GetPlatformSpecificTextureSetting(const PlatformName& platformName, const TextureSettings& baseTextureSettings, TextureSettings& textureSettingsOut, AZ::SerializeContext* serializeContext = nullptr);

        static void Reflect(AZ::ReflectContext* context);


        /**
        * Loads base texture settings obtained from ".exportsettings" file (legacy setting)
        * @param ImagePath absolute/relative path of the image file.
        * @param FilePath absolute/relative path of the ".exportsettings" file.
        * @param Output TextureSettings which contain the result of the function, it may contains platform-specific overrides.
        * @param Optional. Serialize context. Will use global context if none is provided.
        * @return Status outcome result.
        */
        static StringOutcome LoadLegacyTextureSettingFromFile(const AZStd::string& imagePath, const AZStd::string& filepath, TextureSettings& textureSettingOut, AZ::SerializeContext* serializeContext = nullptr);
        
        /**
        * Loads base texture settings obtained from a legacy setting string
        * @param ImagePath absolute/relative path of the image file.
        * @param Content string of the legacy setting to be read.
        * @param Output TextureSettings which contain the result of the function, it may contains platform-specific overrides.
        * @param Optional. Serialize context. Will use global context if none is provided.
        * @return Status outcome result.
        */
        static StringOutcome LoadLegacyTextureSetting(const AZStd::string& imagePath, const AZStd::string& contentString, TextureSettings& textureSettingOut, AZ::SerializeContext* serializeContext = nullptr);
        
        /**
        * Loads base texture settings obtained from ".imagesettings" file (modern setting)
        * @param FilePath absolute/relative path of the ".imagesettings" file.
        * @param Output TextureSettings which contain the result of the function, it may contains platform-specific overrides.
        * @param Optional. Serialize context. Will use global context if none is provided.
        * @return Status outcome result.
        */
        static StringOutcome LoadTextureSetting(const AZStd::string& filepath, TextureSettings& textureSettingPtrOut, AZ::SerializeContext* serializeContext = nullptr);
        
        /**
        * Writes base texture settings to a ".imagesettings" file (modern setting)
        * @param FilePath absolute/relative path of the ".imagesettings" file.
        * @param TextureSetting to be written on the disk, it may contains platform-specific overrides.
        * @param Optional. Serialize context. Will use global context if none is provided.
        * @return Status outcome result.
        */
        static StringOutcome WriteTextureSetting(const AZStd::string& filepath, TextureSettings& textureSetting, AZ::SerializeContext* serializeContext = nullptr);

        // Generates a MultiplatformTextureSettings collection with default texture settings for all
        static MultiplatformTextureSettings GenerateDefaultMultiplatformTextureSettings(const AZStd::string& imageFilepath);

        /**
        * Generates a TextureSetting instance of a particular image file for each supported platform.
        * @param filepath - A path to the texture file.
        * @param canOverridePreset - Returns whether the preset can be overriden. Will return false if the preset was selecting from a settings file created by the user.
        * @param serializeContext - Optional. Serialize context used for reflection/serialization
        * @return - The collection of TextureSetting instances. If error occurs, a default MultiplatformTextureSettings is returned (see GenerateDefaultMultiplatformTextureSettings()).
        */
        static const MultiplatformTextureSettings GetMultiplatformTextureSetting(const AZStd::string& filepath, bool& canOverridePreset, AZ::SerializeContext* serializeContext = nullptr);
        
        /**
        * Generates a TextureSetting instance of a particular image file for each supported platform.
        * @param textureSettings - A reference to an already-loaded texture settings.
        * @param serializeContext - Optional. Serialize context used for reflection/serialization
        * @return - The collection of TextureSetting instances. If error occurs, a default MultiplatformTextureSettings is returned (see GenerateDefaultMultiplatformTextureSettings()).
        */
        static const MultiplatformTextureSettings GetMultiplatformTextureSetting(const TextureSettings& textureSettings, AZ::SerializeContext* serializeContext = nullptr);

        static const char* legacyExtensionName;
        static const char* modernExtensionName;
        static const size_t s_MaxMipMaps = 6;

        // uuid of selected preset for this texture
        AZ::Uuid m_preset;

        // texture size reduce level. the value of this variable will override the same variable in PresetSettings
        unsigned int m_sizeReduceLevel;

        // "ser". Whether to enable supress reduce resolution (m_sizeReduceLevel) during loading, 0(default)
        // the value of this variable will override the same variable in PresetSettings
        bool m_suppressEngineReduce;

        //enable generate mipmap or not
        bool m_enableMipmap;

        //"mc". not used in rc.ini. experiemental
        //maybe relate to http://the-witness.net/news/2010/09/computing-alpha-mipmaps/ 
        bool m_maintainAlphaCoverage;

        // "M", adjust mipalpha, 0..50=normal..100. associate with ComputeMIPAlphaOffset function
        // only useful if m_maintainAlphaCoverage set to true. 
        // This data type MUST be an AZStd::vector, even though we treat is as a fixed array. This is due to a limitation
        // during AZ::DataPatch serialization, where an element is allocated one by one while extending the container..
        AZStd::vector<AZ::u32> m_mipAlphaAdjust;

        MipGenEvalType m_mipGenEval;

        MipGenType m_mipGenType;

    private:
        // Platform overrides in form of DataPatch. Each entry is a patch for a specified platform.
        // This map is used to generate TextureSettings with overridden values. The map is empty if
        // the instance is for platform-specific settings.
        AZStd::map<PlatformName, AZ::DataPatch> m_platfromOverrides;

        // The platform which these settings override. 
        // Blank if the instance is for common settings.
        PlatformName m_overridingPlatform;

        // Comparison operators only compare the base settings, they do not compare overrides.
        // For a comprehensive equality comparison, use Equals() function.
        bool operator==(const TextureSettings& other) const;
        bool operator!=(const TextureSettings& other) const;

    };

} // namespace ImageProcessing