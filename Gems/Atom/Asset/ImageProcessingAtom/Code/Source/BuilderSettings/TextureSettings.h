/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/ImageProcessing/ImageProcessingDefines.h>
#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/std/containers/set.h>

namespace AZ
{
    class ReflectContext;
    class SerializeContext;
}

namespace ImageProcessingAtom
{
    class TextureSettings;
    typedef AZStd::map<PlatformName, TextureSettings> MultiplatformTextureSettings;
    /**
     * TextureSettings is the configuration for processing one image. It contains a reference of preset and other parameters.
     * Some parameters are come from preset but overwrite them.
     * The texture settings may be different for each platform, so the different is saved as data patch per platform.
     * When automatically generate a new texture settings for an image file, use BuilderSettingManager::GetSuggestedPreset function to find the best preset
     * may fit this image, then use ApplyPreset to propagate values from preset settings to texture settings.
     * TextureSettings is intended to be editable for user to modify its value through texture editor tool.
     */
    class TextureSettings
    {
    public:
        AZ_TYPE_INFO(TextureSettings, "{980132FF-C450-425D-8AE0-BD96A8486177}");
        AZ_CLASS_ALLOCATOR(TextureSettings, AZ::SystemAllocator);

        TextureSettings();

        /**
         * Returns an alpha offset value for certain mip. The alpha offset is interpolated from m_mipAlphaAdjust[]
         * and used for TransferAlphaCoverage only.
         * Check the comment of m_maintainAlphaCoverage for more detail on how it may work.
         */
        float ComputeMIPAlphaOffset(AZ::u32 mip) const;

        /**
         * Apply value of some preset settings to this texture settings
         */
        void ApplyPreset(PresetName presetName);

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
        * Loads base texture settings obtained from ".assetinfo" file
        * @param FilePath absolute/relative path of the ".assetinfo" file.
        * @param Output TextureSettings which contain the result of the function, it may contains platform-specific overrides.
        * @param Optional. Serialize context. Will use global context if none is provided.
        * @return Status outcome result.
        */
        static StringOutcome LoadTextureSetting(const AZStd::string& filepath, TextureSettings& textureSettingPtrOut, AZ::SerializeContext* serializeContext = nullptr);

        /**
        * Writes base texture settings to a ".assetinfo" file (modern setting)
        * @param FilePath absolute/relative path of the ".assetinfo" file.
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

        static const char* ExtensionName;
        static const size_t s_MaxMipMaps = 6;

        // uuid of selected preset for this texture
        // We are deprecating preset UUID and switching to preset name as an unique id
        AZ::Uuid m_presetId;

        PresetName m_preset;

        // texture size reduce level. the value of this variable will override the same variable in PresetSettings
        unsigned int m_sizeReduceLevel;

        // "ser". Whether to enable suppress reduce resolution (m_sizeReduceLevel) during loading, 0(default)
        // the value of this variable will override the same variable in PresetSettings
        bool m_suppressEngineReduce;

        //enable generate mipmap or not
        bool m_enableMipmap;

        //"mc". not used in rc.ini. experimental
        //maybe relate to http://the-witness.net/news/2010/09/computing-alpha-mipmaps/
        bool m_maintainAlphaCoverage;

        // "M", adjust mipalpha, 0..50=normal..100. associate with ComputeMIPAlphaOffset function
        // only useful if m_maintainAlphaCoverage set to true.
        // This data type MUST be an AZStd::vector, even though we treat is as a fixed array. This is due to a limitation
        // during AZ::DataPatch serialization, where an element is allocated one by one while extending the container..
        AZStd::vector<AZ::u32> m_mipAlphaAdjust;

        MipGenEvalType m_mipGenEval;

        MipGenType m_mipGenType;

        AZStd::set<AZStd::string> m_tags;

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
} // namespace ImageProcessingAtom
