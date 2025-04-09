/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <BuilderSettings/TextureSettings.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/Utils.h>

#include <BuilderSettings/BuilderSettingManager.h>
#include <ImageLoader/ImageLoaders.h>
#include <Editor/EditorCommon.h>


namespace ImageProcessingAtom
{
    const char* TextureSettings::ExtensionName = ".assetinfo";

    TextureSettings::TextureSettings()
        : m_sizeReduceLevel(0)
        , m_suppressEngineReduce(false)
        , m_enableMipmap(true)
        , m_maintainAlphaCoverage(false)
        , m_mipGenEval(MipGenEvalType::sum)
        , m_mipGenType(MipGenType::blackmanHarris)
    {
        const int defaultMipMapValue = 50;

        for (int i = 0; i < s_MaxMipMaps; ++i)
        {
            m_mipAlphaAdjust.push_back(defaultMipMapValue);
        }
    }

    void TextureSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TextureSettings>()
                ->Version(2)
                ->Field("PresetID", &TextureSettings::m_presetId)
                ->Field("Preset", &TextureSettings::m_preset)
                ->Field("SizeReduceLevel", &TextureSettings::m_sizeReduceLevel)
                ->Field("EngineReduce", &TextureSettings::m_suppressEngineReduce)
                ->Field("EnableMipmap", &TextureSettings::m_enableMipmap)
                ->Field("MipMapGenEval", &TextureSettings::m_mipGenEval)
                ->Field("MipMapGenType", &TextureSettings::m_mipGenType)
                ->Field("MaintainAlphaCoverage", &TextureSettings::m_maintainAlphaCoverage)
                ->Field("MipMapAlphaAdjustments", &TextureSettings::m_mipAlphaAdjust)
                ->Field("PlatformSpecificOverrides", &TextureSettings::m_platfromOverrides)
                ->Field("OverridingPlatform", &TextureSettings::m_overridingPlatform)
                ->Field("Tags", &TextureSettings::m_tags);

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TextureSettings>("Texture Setting", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TextureSettings::m_mipAlphaAdjust, "Alpha Test Bias", "Multiplies the mipmap's alpha channel by a scale value that is based on alpha coverage. \
                                            Specify a value from 0 to 100 for each mipmap to offset the alpha test values and ensure the mipmap's alpha coverage matches the original image.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->ElementAttribute(AZ::Edit::UIHandlers::Handler, AZ::Edit::UIHandlers::Slider)
                        ->ElementAttribute(AZ::Edit::Attributes::Min, 0)
                        ->ElementAttribute(AZ::Edit::Attributes::Max, 100)
                        ->ElementAttribute(AZ::Edit::Attributes::Step, 1)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &TextureSettings::m_mipGenType, "Filter Type", "Filter Types specify sample sizes and algorithms \
                                            for determining the color of each pixel as the texture resolution is reduced for each mipmap.")
                        ->EnumAttribute(MipGenType::point, "Point")
                        ->EnumAttribute(MipGenType::box, "Average")
                        ->EnumAttribute(MipGenType::triangle, "Linear")
                        ->EnumAttribute(MipGenType::quadratic, "Bilinear")
                        ->EnumAttribute(MipGenType::gaussian, "Gaussian")
                        ->EnumAttribute(MipGenType::blackmanHarris, "BlackmanHarris")
                        ->EnumAttribute(MipGenType::kaiserSinc, "KaiserSinc")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &TextureSettings::m_mipGenEval, "Pixel Sampler", "The Pixel Sampler specifies how the final pixel value is calculated when mipmaps are generated.")
                        ->EnumAttribute(MipGenEvalType::max, "Max")
                        ->EnumAttribute(MipGenEvalType::min, "Min")
                        ->EnumAttribute(MipGenEvalType::sum, "Sum")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &TextureSettings::m_maintainAlphaCoverage, "Adjust Alpha", "Enable to manually adjust the alpha channel of the mipmaps with the Alpha Test Bias values.")
                ;
            }
        }
    }

    bool TextureSettings::operator!=(const TextureSettings& other) const
    {
        return !(*this == other);
    }

    bool TextureSettings::operator==(const TextureSettings& other) const
    {
        /////////////////////////////
        // Compare Alpha Adjust
        /////////////////////////////
        bool matchingAlphaTestAdjust = true;
        for (AZ::u8 curIndex = 0; curIndex < s_MaxMipMaps; ++curIndex)
        {
            if (m_mipAlphaAdjust[curIndex] != other.m_mipAlphaAdjust[curIndex])
            {
                matchingAlphaTestAdjust = false;
                break;
            }
        }
        return
            matchingAlphaTestAdjust &&
            m_preset == other.m_preset &&
            m_sizeReduceLevel == other.m_sizeReduceLevel &&
            m_suppressEngineReduce == other.m_suppressEngineReduce &&
            m_maintainAlphaCoverage == other.m_maintainAlphaCoverage &&
            m_mipGenEval == other.m_mipGenEval &&
            m_mipGenType == other.m_mipGenType &&
            m_tags == other.m_tags;
    }

    bool TextureSettings::Equals(const TextureSettings& other, AZ::SerializeContext* serializeContext)
    {
        /////////////////////////////
        // Compare Common Settings
        /////////////////////////////
        if (*this != other)
        {
            return false;
        }

        /////////////////////////////
        // Compare Overrides
        /////////////////////////////
        const MultiplatformTextureSettings selfOverrides = GetMultiplatformTextureSetting(*this, serializeContext);
        const MultiplatformTextureSettings otherOverrides = GetMultiplatformTextureSetting(other, serializeContext);
        auto selfOverridesIter = selfOverrides.begin();
        auto otherOverridesIter = otherOverrides.begin();

        while (selfOverridesIter != selfOverrides.end() && otherOverridesIter != otherOverrides.end())
        {
            if (selfOverridesIter->second != otherOverridesIter->second)
            {
                return false;
            }
            otherOverridesIter++;
            selfOverridesIter++;
        }

        AZ_Assert(selfOverridesIter == selfOverrides.end() && otherOverridesIter == otherOverrides.end(), "Both iterators must be at the end by now.")
        return true;
    }

    float TextureSettings::ComputeMIPAlphaOffset(AZ::u32 mip) const
    {
        if (mip / 2 + 1 >= s_MaxMipMaps)
        {
            return 0;
        }

        float fVal = static_cast<float>(m_mipAlphaAdjust[s_MaxMipMaps - 1]);
        if (mip / 2 + 1 < s_MaxMipMaps)
        {
            float fInterpolationSlider1 = static_cast<float>(m_mipAlphaAdjust[mip / 2]);
            float fInterpolationSlider2 = static_cast<float>(m_mipAlphaAdjust[mip / 2 + 1]);
            fVal = fInterpolationSlider1 + (fInterpolationSlider2 - fInterpolationSlider1) * (mip & 1) * 0.5f;
        }

        return 0.5f - fVal / 100.0f;
    }

    void TextureSettings::ApplyPreset(PresetName presetName)
    {
        const PresetSettings* presetSetting = BuilderSettingManager::Instance()->GetPreset(presetName);
        if (presetSetting != nullptr)
        {
            m_sizeReduceLevel = presetSetting->m_sizeReduceLevel;
            m_suppressEngineReduce = presetSetting->m_suppressEngineReduce;
            if (presetSetting->m_mipmapSetting)
            {
                m_mipGenType = presetSetting->m_mipmapSetting->m_type;
            }

            m_preset = presetName;
        }
        else
        {
            AZ_Error("Image Processing", false, "Cannot set an invalid preset %s!", presetName.GetCStr());
        }
    }

    StringOutcome TextureSettings::LoadTextureSetting(const AZStd::string& filepath, TextureSettings& textureSettingPtrOut, AZ::SerializeContext* serializeContext /*= nullptr*/)
    {
        auto loadedTextureSettingPtr = AZStd::unique_ptr<TextureSettings>(AZ::Utils::LoadObjectFromFile<TextureSettings>(filepath, serializeContext));

        if (!loadedTextureSettingPtr)
        {
            return AZ::Failure(AZStd::string());
        }

        textureSettingPtrOut = *loadedTextureSettingPtr;

        // In old format, the preset name doesn't exist. Using preset id to get preset name
        // We can remove this when we fully deprecate the preset uuid
        if (textureSettingPtrOut.m_preset.IsEmpty())
        {
            textureSettingPtrOut.m_preset = BuilderSettingManager::Instance()->GetPresetNameFromId(textureSettingPtrOut.m_presetId);
        }

        return AZ::Success(AZStd::string());
    }

    StringOutcome TextureSettings::WriteTextureSetting(const AZStd::string& filepath, TextureSettings& textureSetting, AZ::SerializeContext* serializeContext)
    {
        if (false == AZ::Utils::SaveObjectToFile<TextureSettings>(filepath, AZ::DataStream::StreamType::ST_XML, &textureSetting, serializeContext))
        {
            return STRING_OUTCOME_ERROR("Failed to write to file: " + filepath);
        }

        return STRING_OUTCOME_SUCCESS;
    }

    MultiplatformTextureSettings TextureSettings::GenerateDefaultMultiplatformTextureSettings(const AZStd::string& imageFilepath)
    {
        MultiplatformTextureSettings settings;
        PlatformNameList platformsList = BuilderSettingManager::Instance()->GetPlatformList();
        PresetName suggestedPreset = BuilderSettingManager::Instance()->GetSuggestedPreset(imageFilepath);

        // If the suggested preset doesn't exist (or was failed to be loaded), return empty texture settings
        if (BuilderSettingManager::Instance()->GetPreset(suggestedPreset) == nullptr)
        {
            AZ_Error("Image Processing", false, "Failed to find suggested preset [%s]", suggestedPreset.GetCStr());
            return settings;
        }

        for (PlatformName& platform : platformsList)
        {
            TextureSettings textureSettings;
            textureSettings.ApplyPreset(suggestedPreset);
            settings.insert(AZStd::pair<PlatformName, TextureSettings>(platform, textureSettings));
        }
        return settings;
    }

    StringOutcome TextureSettings::GetPlatformSpecificTextureSetting(const PlatformName& platformName, const TextureSettings& baseTextureSettings,
        TextureSettings& textureSettingsOut, AZ::SerializeContext* serializeContext)
    {
        // Obtain the DataPatch (if platform exists)
        auto overrideIter = baseTextureSettings.m_platfromOverrides.find(platformName);
        if (overrideIter == baseTextureSettings.m_platfromOverrides.end())
        {
            return STRING_OUTCOME_ERROR(AZStd::string::format("TextureSettings preset [%s] does not have override for platform [%s]",
                baseTextureSettings.m_preset.GetCStr(), platformName.c_str()));
        }
        AZ::DataPatch& platformOverride = const_cast<AZ::DataPatch&>(overrideIter->second);

        // Update settings instance with override values.
        if (platformOverride.IsData())
        {
            // Apply the AZ::DataPatch to obtain a platform-overridden version of the TextureSettings.
            AZStd::unique_ptr<TextureSettings> platformSpecificTextureSettings(platformOverride.Apply(&baseTextureSettings, serializeContext));
            AZ_Assert(platformSpecificTextureSettings->m_mipAlphaAdjust.size() == s_MaxMipMaps, "Unexpected m_mipAlphaAdjust size.");

            // Adjust overrides data to imply 'platformSpecificTextureSettings' *IS* the override.
            platformSpecificTextureSettings->m_platfromOverrides.clear();
            platformSpecificTextureSettings->m_overridingPlatform = platformName;
            textureSettingsOut = *platformSpecificTextureSettings;
        }
        else
        {
            textureSettingsOut = baseTextureSettings;
        }

        return STRING_OUTCOME_SUCCESS;
    }

    const MultiplatformTextureSettings TextureSettings::GetMultiplatformTextureSetting(const TextureSettings& textureSettings, AZ::SerializeContext* serializeContext)
    {
        MultiplatformTextureSettings loadedSettingsReturn;
        PlatformNameList platformsList = BuilderSettingManager::Instance()->GetPlatformList();
        // Generate MultiplatformTextureSettings based on existing available overrides.
        for (const PlatformName& curPlatformName : platformsList)
        {
            // Start with a copy of the base settings
            TextureSettings curPlatformOverride = textureSettings;
            if (!GetPlatformSpecificTextureSetting(curPlatformName, textureSettings, curPlatformOverride, serializeContext).IsSuccess())
            {
                // We have failed to obtain an override. Maintain base settings to indicate zero overrides.
                // We still want to designate these TextureSettings as an (empty) override.
                curPlatformOverride.m_platfromOverrides.clear();
                curPlatformOverride.m_overridingPlatform = curPlatformName;
            }

            // Add as an entry to the multiplatform texture settings
            loadedSettingsReturn.insert(AZStd::pair<PlatformName, TextureSettings>(curPlatformName, curPlatformOverride));
        }

        // return a copy of the results
        return loadedSettingsReturn;
    }

    const MultiplatformTextureSettings TextureSettings::GetMultiplatformTextureSetting(const AZStd::string& imageFilepath, bool& canOverridePreset, AZ::SerializeContext* serializeContext)
    {
        TextureSettings loadedTextureSetting;

        // Attempt to get metadata file path from image path.
        AZStd::string metadataFilepath = imageFilepath + TextureSettings::ExtensionName;
        bool hasMetafile = AZ::IO::SystemFile::Exists(metadataFilepath.c_str());

        canOverridePreset = true;

        // If the image has an accompanying metadata...
        if(hasMetafile)
        {
            // Parse the metadata file.
            if (LoadTextureSetting(metadataFilepath, loadedTextureSetting, serializeContext).IsSuccess())
            {
                canOverridePreset = false;
                return GetMultiplatformTextureSetting(loadedTextureSetting, serializeContext);
            }
            else
            {
                AZ_Error("Image processing", false, "Failed to load the image's meta file %s", metadataFilepath.c_str());
            }
        }

        return GenerateDefaultMultiplatformTextureSettings(imageFilepath);
    }

    StringOutcome TextureSettings::ApplySettings(const TextureSettings& settings, const PlatformName& overridePlatform, AZ::SerializeContext* serializeContext)
    {
        if (overridePlatform.empty())
        {
            *this = settings;
        }
        else
        {
            AZ::DataPatch newOverride;
            if (false == newOverride.Create<TextureSettings, TextureSettings>(this, &settings, AZ::DataPatch::FlagsMap(), AZ::DataPatch::FlagsMap(), serializeContext))
            {
                return STRING_OUTCOME_ERROR("Failed to create TextureSettings platform override data. See AZ_Error log for details.");
            }

            m_platfromOverrides[overridePlatform] = newOverride;
        }

        return STRING_OUTCOME_SUCCESS;
    }
}
