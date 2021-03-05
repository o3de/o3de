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

#include "ImageProcessing_precompiled.h"
#include <BuilderSettings/TextureSettings.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/Utils.h>

#include <BuilderSettings/BuilderSettingManager.h>
#include <ImageLoader/ImageLoaders.h>


namespace ImageProcessing
{
    const char* TextureSettings::legacyExtensionName = ".exportsettings";
    const char* TextureSettings::modernExtensionName = ".imagesettings";

    StringOutcome ParseLegacyTextureSettingString(AZStd::string& key, AZStd::string& value, TextureSettings& textureSettingOut)
    {
        // Parse only the settings we support for TextureSetting
        if ("reduce" == key) // Example: reduce=0
        {
            int reduce = AzFramework::StringFunc::ToInt(value.c_str());
            if (reduce >= 0)
            {
                textureSettingOut.m_sizeReduceLevel = reduce;
            }
        }
        else if ("M" == key) // Example: M=50,50,0,50,50,50
        {
            textureSettingOut.m_enableMipmap = true;

            AZStd::vector<AZStd::string> mipStringValues;
            AzFramework::StringFunc::Tokenize(value.c_str(), mipStringValues, ',');

            for (size_t mipIndex = 0; mipIndex < mipStringValues.size(); ++mipIndex)
            {
                AZ::u32 mipValue = AzFramework::StringFunc::ToInt(mipStringValues[mipIndex].c_str());
                textureSettingOut.m_mipAlphaAdjust[mipIndex] = mipValue;
            }
        }
        else if ("ser" == key) // Example: ser=1
        {
            textureSettingOut.m_suppressEngineReduce = (value == "1");
        }
        else if ("preset" == key) // Example: preset=NormalsWithSmoothness
        {
            AZ::Uuid presetUuid = BuilderSettingManager::Instance()->GetPresetIdFromName(value);

            // There's a chance the preset name still adheres to legacy preset naming convention (from CryEngine).
            const PresetName translation = BuilderSettingManager::Instance()->TranslateLegacyPresetName(value);
            AZ::Uuid translatedPresetUuid = BuilderSettingManager::Instance()->GetPresetIdFromName(translation);

            if (!presetUuid.IsNull())
            {
                textureSettingOut.m_preset = presetUuid;
            }
            else if (!translatedPresetUuid.IsNull())
            {
                textureSettingOut.m_preset = translatedPresetUuid;
            }
            else
            {
                AZ_Error("Image processing", false, "Can't find preset %s", value.c_str());
            }
        }
        else if ("mipgentype" == key) // Example: mipgentype=box
        {
            if ("box" == value || "average" == value)
            {
                textureSettingOut.m_mipGenType = MipGenType::box;
            }
            else if ("gauss" == value)
            {
                textureSettingOut.m_mipGenType = MipGenType::gaussian;
            }
            else if ("blackman-harris" == value)
            {
                textureSettingOut.m_mipGenType = MipGenType::blackmanHarris;
            }
            else if ("kaiser" == value)
            {
                textureSettingOut.m_mipGenType = MipGenType::kaiserSinc;
            }
            else if ("point" == value)
            {
                textureSettingOut.m_mipGenType = MipGenType::point;
            }
            else if ("quadric" == value)
            {
                textureSettingOut.m_mipGenType = MipGenType::quadratic;
            }
            else if ("triangle" == value)
            {
                textureSettingOut.m_mipGenType = MipGenType::triangle;
            }
        }
        return STRING_OUTCOME_SUCCESS;
    }

    TextureSettings::TextureSettings()
        : m_preset(0)
        , m_sizeReduceLevel(0)
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
                ->Version(1)
                ->Field("PresetID", &TextureSettings::m_preset)
                ->Field("SizeReduceLevel", &TextureSettings::m_sizeReduceLevel)
                ->Field("EngineReduce", &TextureSettings::m_suppressEngineReduce)
                ->Field("EnableMipmap", &TextureSettings::m_enableMipmap)
                ->Field("MaintainAlphaCoverage", &TextureSettings::m_maintainAlphaCoverage)
                ->Field("MipMapAlphaAdjustments", &TextureSettings::m_mipAlphaAdjust)
                ->Field("MipMapGenEval", &TextureSettings::m_mipGenEval)
                ->Field("MipMapGenType", &TextureSettings::m_mipGenType)
                ->Field("PlatformSpecificOverrides", &TextureSettings::m_platfromOverrides)
                ->Field("OverridingPlatform", &TextureSettings::m_overridingPlatform);

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TextureSettings>("Texture Setting", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TextureSettings::m_mipAlphaAdjust, "Alpha Test Bias", "Multiplies the mipmap's alpha with a scale value that is based on alpha coverage. \
                                            Set the mip 0 to mip 5 values to offset the alpha test values and ensure the mipmap's alpha coverage matches the original image. Specify a value from 0 to 100.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->ElementAttribute(AZ::Edit::UIHandlers::Handler, AZ::Edit::UIHandlers::Slider)
                        ->ElementAttribute(AZ::Edit::Attributes::Min, 0)
                        ->ElementAttribute(AZ::Edit::Attributes::Max, 100)
                        ->ElementAttribute(AZ::Edit::Attributes::Step, 1)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &TextureSettings::m_mipGenType, "Filter Method", "")
                        ->EnumAttribute(MipGenType::point, "Point")
                        ->EnumAttribute(MipGenType::box, "Average")
                        ->EnumAttribute(MipGenType::triangle, "Linear")
                        ->EnumAttribute(MipGenType::quadratic, "Bilinear")
                        ->EnumAttribute(MipGenType::gaussian, "Gaussian")
                        ->EnumAttribute(MipGenType::blackmanHarris, "BlackmanHarris")
                        ->EnumAttribute(MipGenType::kaiserSinc, "KaiserSinc")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &TextureSettings::m_mipGenEval, "Pixel Sampling Type", "")
                        ->EnumAttribute(MipGenEvalType::max, "Max")
                        ->EnumAttribute(MipGenEvalType::min, "Min")
                        ->EnumAttribute(MipGenEvalType::sum, "Sum")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &TextureSettings::m_maintainAlphaCoverage, "Maintain Alpha Coverage", "Select this option to manually adjust Alpha channel mipmaps.")
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
            m_mipGenType == other.m_mipGenType;
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

    void TextureSettings::ApplyPreset(AZ::Uuid presetId)
    {
        const PresetSettings* presetSetting = BuilderSettingManager::Instance()->GetPreset(presetId);
        if (presetSetting != nullptr)
        {
            m_sizeReduceLevel = presetSetting->m_sizeReduceLevel;
            m_suppressEngineReduce = presetSetting->m_suppressEngineReduce;
            if (presetSetting->m_mipmapSetting)
            {
                m_mipGenType = presetSetting->m_mipmapSetting->m_type;
            }

            m_preset = presetId;
        }
        else
        {
            AZ_Error("Image Processing", false, "Cannot set an invalid preset %s!", presetId.ToString<AZStd::string>().c_str());
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
        return AZ::Success(AZStd::string());
    }

    StringOutcome TextureSettings::WriteTextureSetting(const AZStd::string& filepath, TextureSettings& textureSetting, AZ::SerializeContext* serializeContext)
    {
        if(false == AZ::Utils::SaveObjectToFile<TextureSettings>(filepath, AZ::DataStream::StreamType::ST_XML, &textureSetting, serializeContext))
        {
            return STRING_OUTCOME_ERROR("Failed to write to file: " + filepath);
        }

        return STRING_OUTCOME_SUCCESS;
    }

    StringOutcome TextureSettings::LoadLegacyTextureSetting(const AZStd::string& imagePath, const AZStd::string& contentString, TextureSettings& textureSettingOut, AZ::SerializeContext* serializeContext)
    {
        AZStd::string trimmedContent = contentString;
        AzFramework::StringFunc::TrimWhiteSpace(trimmedContent, true /* leading */, true /* trailing */);
        if (trimmedContent.empty())
        {
            return STRING_OUTCOME_ERROR("Empty legacy texture setting!");
        }

        AZStd::vector<AZStd::string> settings;
        AZStd::vector<AZStd::string*> overrideSettings;

        // Each setting begins with a forward-slash.
        AzFramework::StringFunc::Tokenize(trimmedContent.c_str(), settings, " /"); // requires space character.
        
        // For each setting pair (field & value)...
        for (AZStd::string& settingPair : settings)
        {            
            // Split setting pair into key & value.
            AZStd::string key, value;
            {
                AZStd::vector<AZStd::string> key_value;
                AzFramework::StringFunc::Tokenize(settingPair.c_str(), key_value, '=');
                if (key_value.size() == 2)
                {
                    key = key_value[0];
                    value = key_value[1];
                }
                else
                {
                    return STRING_OUTCOME_ERROR("Invalid format found in legacy texture setting: " + settingPair);
                }
            }

            bool containsPlatformOverrides = !value.empty() && (value[0] == '"' && value[value.length() - 1] == '"');
            if (containsPlatformOverrides)
            {
                // Process platform-specific overrides on the next loop.
                overrideSettings.push_back(&settingPair);
                continue;
            }

            // Parse the common settings.
            ParseLegacyTextureSettingString(key, value, textureSettingOut);
        }

        // Some setting file won't assign a proper preset for the image, need to assign a suggested one here
        if (textureSettingOut.m_preset.IsNull())
        {
            textureSettingOut.m_preset = BuilderSettingManager::Instance()->GetSuggestedPreset(imagePath);
        }

        // Store a temporary settings of all platforms intended to have overrides within this preset.
        // We will collate all overrides per-platform to generate PatchData at the end.
        MultiplatformTextureSettings overrideCache;

        // For each platform-specific override setting pair
        for (AZStd::string* overrideSettingPair : overrideSettings)
        {
            // Split setting pair into key & value.
            AZStd::string key, value;
            {
                AZStd::vector<AZStd::string> key_value;
                AzFramework::StringFunc::Tokenize(overrideSettingPair->c_str(), key_value, '=');

                if (key_value.size() == 2)
                {
                    key = key_value[0];
                    value = key_value[1];
                }
                else
                {
                    return STRING_OUTCOME_ERROR("Invalid format found in legacy texture setting: " + *overrideSettingPair);
                }
            }

            // Chop the surrounding quotation marks.
            AzFramework::StringFunc::LChop(value, 1);
            AzFramework::StringFunc::RChop(value, 1);

            // Split the collection of platforms overrides into entries in a vector.
            // Layout: { [platform0],[value0],[platform1],[value1],[platform2],[value2] }
            AZStd::vector<AZStd::string> override_platform_value;
            AzFramework::StringFunc::Tokenize(value.c_str(), override_platform_value, ",:");

            if (override_platform_value.size() % 2 != 0)
            {
                return STRING_OUTCOME_ERROR("Invalid format found in legacy texture setting: " + value);
            }

            for (int platformIdx = 0, valueIdx = 1;
                valueIdx < override_platform_value.size();
                platformIdx += 2, valueIdx = platformIdx + 1)
            {
                PlatformName& overridePlatform = override_platform_value[platformIdx];
                AZStd::string& overrideValue = override_platform_value[valueIdx];

                // Insert a copy of the base settings we've parsed from the legacy metafile.
                overrideCache.insert(AZStd::pair<PlatformName, TextureSettings>(overridePlatform, textureSettingOut));

                ParseLegacyTextureSettingString(key, overrideValue, overrideCache[overridePlatform]);
                overrideCache[overridePlatform].m_overridingPlatform = overridePlatform;
                overrideCache[overridePlatform].m_platfromOverrides.clear();
            }
        }

        // Store the final result in a temp variable to do a whole-sale copy at the end.
        TextureSettings finalResult = textureSettingOut;

        // Use the override cache to generate a DataPatch per-platform.
        for (auto& overridePair : overrideCache)
        {
            // Every DataPatch is only a diff between a vanilla common-settings (with no platform-specific overrides) and the specified platform's override.
            AZ::DataPatch platformOverridePatch;
            platformOverridePatch.Create<TextureSettings, TextureSettings>(&textureSettingOut, &overridePair.second,AZ::DataPatch::FlagsMap(), AZ::DataPatch::FlagsMap(), serializeContext);
            finalResult.m_platfromOverrides.insert(AZStd::pair<PlatformName, AZ::DataPatch>(overridePair.first, platformOverridePatch));
        }

        // Fully overwrite output variable. The only difference should be properly filled-out overrides.
        textureSettingOut = finalResult;

        return STRING_OUTCOME_SUCCESS;
    }

    StringOutcome TextureSettings::LoadLegacyTextureSettingFromFile(const AZStd::string& imagePath, const AZStd::string& filepath, TextureSettings& textureSettingOut, AZ::SerializeContext* serializeContext)
    {
        AZStd::string fileContents;

        // Perform file I/O to read the contents of the metafile into the string above.
        auto fileIoPtr = AZ::IO::FileIOBase::GetInstance();
        AZ::IO::HandleType fileHandle;
        fileIoPtr->Open(filepath.c_str(), AZ::IO::OpenMode::ModeRead, fileHandle);
        AZ::u64 fileSize = 0;
        AZ::u64 bytesRead = 0;
        fileIoPtr->Size(fileHandle, fileSize);
        char* fileString = new char[fileSize + 1];
        fileIoPtr->Read(fileHandle, fileString, fileSize, false, &bytesRead);
        fileIoPtr->Close(fileHandle);
        fileString[bytesRead] = 0;
        fileContents = fileString;
        delete[] fileString;
        

        return LoadLegacyTextureSetting(imagePath, fileContents, textureSettingOut, serializeContext);
    }

    MultiplatformTextureSettings TextureSettings::GenerateDefaultMultiplatformTextureSettings(const AZStd::string& imageFilepath)
    {
        MultiplatformTextureSettings settings;
        PlatformNameList platformsList = BuilderSettingManager::Instance()->GetPlatformList();
        AZ::Uuid suggestedPreset = BuilderSettingManager::Instance()->GetSuggestedPreset(imageFilepath);
        if (!suggestedPreset.IsNull())
        {
            for (PlatformName& platform : platformsList)
            {
                TextureSettings textureSettings;
                textureSettings.ApplyPreset(suggestedPreset);
                settings.insert(AZStd::pair<PlatformName, TextureSettings>(platform, textureSettings));
            }
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
                baseTextureSettings.m_preset.ToString<AZStd::string>().c_str(), platformName.c_str()));
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

        // Attempt to get metadata filepath from image path.
        AZStd::string legacyMetadataFilepath = imageFilepath + legacyExtensionName;
        AZStd::string modernMetadataFilepath = imageFilepath + modernExtensionName;
        bool hasLegacyMetafile = AZ::IO::SystemFile::Exists(legacyMetadataFilepath.c_str());
        bool hasModernMetafile = AZ::IO::SystemFile::Exists(modernMetadataFilepath.c_str());

        // If the image has an accompanying metadata...
        if(hasModernMetafile)
        {
            // Parse the metadata file.
            if (!LoadTextureSetting(modernMetadataFilepath, loadedTextureSetting, serializeContext).IsSuccess())
            {
                canOverridePreset = true;
                return GenerateDefaultMultiplatformTextureSettings(imageFilepath);
            }
        }
        else if (hasLegacyMetafile)
        {
            if (!LoadLegacyTextureSettingFromFile(imageFilepath, legacyMetadataFilepath, loadedTextureSetting, serializeContext).IsSuccess())
            {
                canOverridePreset = true;
                return GenerateDefaultMultiplatformTextureSettings(imageFilepath);
            }
        }
        else
        {
            canOverridePreset = true; // RC could override settings if it was loaded from the image so this is set to
                                      // true regardless of whether settings existed in the texture for compatibility.
            // Try to load from image file if it has embedded setting file
            AZStd::string embeddedString = LoadEmbeddedSettingFromFile(imageFilepath);
            if (!LoadLegacyTextureSetting(imageFilepath, embeddedString, loadedTextureSetting, serializeContext).IsSuccess())
            {
                // If the texture has neither of legacy/modern meta file nor embedded setting, generate data for a new metadata file.
                return GenerateDefaultMultiplatformTextureSettings(imageFilepath);
            }
        }

        // Generate MultiplatformTextureSettings based on the loaded texture setting.
        return GetMultiplatformTextureSetting(loadedTextureSetting, serializeContext);
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
