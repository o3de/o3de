/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "BuilderSettingManager.h"
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QString>

#include <Atom/ImageProcessing/PixelFormats.h>
#include <BuilderSettings/CubemapSettings.h>
#include <BuilderSettings/TextureSettings.h>
#include <Converters/Cubemap.h>
#include <Processing/PixelFormatInfo.h>
#include <Processing/ImageToProcess.h>
#include <ImageLoader/ImageLoaders.h>
#include <ImageProcessing_Traits_Platform.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/Sha1.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Application/Application.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

namespace ImageProcessingAtom
{
    const char* BuilderSettingManager::s_defaultConfigRelativeFolder = "Gems/Atom/Asset/ImageProcessingAtom/Config/";
    const char* BuilderSettingManager::s_projectConfigRelativeFolder = "Config/AtomImageBuilder/";
    const char* BuilderSettingManager::s_builderSettingFileName = "ImageBuilder.settings";
    const char* BuilderSettingManager::s_presetFileExtension = ".preset";

    const char FileMaskDelimiter = '_';

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3) \
    namespace ImageProcess##PrivateName                                                                                                                                                           \
    {                                                                                                                                                                                             \
        bool DoesSupport(AZStd::string);                                                                                                                                                          \
    }
    AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif //AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS

    const char* BuilderSettingManager::s_environmentVariableName = "ImageBuilderSettingManager_Atom";
    AZ::EnvironmentVariable<BuilderSettingManager*> BuilderSettingManager::s_globalInstance = nullptr;
    AZStd::mutex BuilderSettingManager::s_instanceMutex;
    const PlatformName BuilderSettingManager::s_defaultPlatform = AZ_TRAIT_IMAGEPROCESSING_DEFAULT_PLATFORM;
    
    void BuilderSettingManager::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<BuilderSettingManager>()
                ->Version(1)
                ->Field("AnalysisFingerprint", &BuilderSettingManager::m_analysisFingerprint)
                ->Field("BuildSettings", &BuilderSettingManager::m_builderSettings)
                ->Field("DefaultPresetsByFileMask", &BuilderSettingManager::m_defaultPresetByFileMask)
                ->Field("DefaultPreset", &BuilderSettingManager::m_defaultPreset)
                ->Field("DefaultPresetAlpha", &BuilderSettingManager::m_defaultPresetAlpha)
                ->Field("DefaultPresetNonePOT", &BuilderSettingManager::m_defaultPresetNonePOT);
        }
    }

    BuilderSettingManager* BuilderSettingManager::Instance()
    {
        AZStd::lock_guard<AZStd::mutex> lock(s_instanceMutex);

        if (!s_globalInstance)
        {
            s_globalInstance = AZ::Environment::FindVariable<BuilderSettingManager*>(s_environmentVariableName);
        }
        AZ_Assert(s_globalInstance, "BuilderSettingManager not created!");

        return s_globalInstance.Get();
    }

    void BuilderSettingManager::CreateInstance()
    {
        AZStd::lock_guard<AZStd::mutex> lock(s_instanceMutex);

        if (s_globalInstance)
        {
            AZ_Assert(false, "BuilderSettingManager already created!");
            return;
        }

        if (!s_globalInstance)
        {
            s_globalInstance = AZ::Environment::CreateVariable<BuilderSettingManager*>(s_environmentVariableName);
        }
        if (!s_globalInstance.Get())
        {
            s_globalInstance.Set(aznew BuilderSettingManager());
        }
    }
    
    void BuilderSettingManager::DestroyInstance()
    {
        AZStd::lock_guard<AZStd::mutex> lock(s_instanceMutex);
        AZ_Assert(s_globalInstance, "Invalid call to DestroyInstance - no instance exists.");
        AZ_Assert(s_globalInstance.Get(), "You can only call DestroyInstance if you have called CreateInstance.");

        delete s_globalInstance.Get();
        s_globalInstance.Reset();
    }
    
    const PresetSettings* BuilderSettingManager::GetPreset(const PresetName& presetName, const PlatformName& platform, AZStd::string_view* settingsFilePathOut)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);
        auto itr = m_presets.find(presetName);
        if (itr != m_presets.end())
        {
            if (settingsFilePathOut)
            {
                *settingsFilePathOut = itr->second.m_presetFilePath;
            }
            return itr->second.m_multiPreset.GetPreset(platform);
        }
        return nullptr;
    }

    const BuilderSettings* BuilderSettingManager::GetBuilderSetting(const PlatformName& platform)
    {
        if (m_builderSettings.find(platform) != m_builderSettings.end())
        {
            return &m_builderSettings[platform];
        }
        return nullptr;
    }

    const PlatformNameList BuilderSettingManager::GetPlatformList()
    {
        PlatformNameList platforms;

        for (auto& builderSetting : m_builderSettings)
        {
            if (builderSetting.second.m_enablePlatform)
            {
                platforms.push_back(builderSetting.first);
            }
        }

        return platforms;
    }

    const AZStd::map <FileMask, AZStd::unordered_set<PresetName>>& BuilderSettingManager::GetPresetFilterMap()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);
        return m_presetFilterMap;
    }

    const PresetName BuilderSettingManager::GetPresetNameFromId(const AZ::Uuid& presetId)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);

        for (const auto& itr : m_presets)
        {
            if (itr.second.m_multiPreset.GetPresetId() == presetId)
            {
                return itr.second.m_multiPreset.GetPresetName();
            }
        }

        return {};
    }

    void BuilderSettingManager::ClearSettings()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);
        m_presetFilterMap.clear();
        m_builderSettings.clear();
        m_presets.clear();
        m_defaultPresetByFileMask.clear();
    }

    StringOutcome BuilderSettingManager::LoadConfig()
    {
        StringOutcome outcome = STRING_OUTCOME_ERROR("");

        auto fileIoBase = AZ::IO::FileIOBase::GetInstance();
        if (fileIoBase == nullptr)
        {
            return AZ::Failure(AZStd::string("File IO instance needs to be initialized to resolve ImageProcessing builder file aliases"));
        }

        // Construct the default setting path

        AZ::IO::FixedMaxPath defaultConfigFolder;
        if (auto engineRoot = fileIoBase->ResolvePath("@engroot@"); engineRoot.has_value())
        {
            defaultConfigFolder = *engineRoot;
            defaultConfigFolder /= s_defaultConfigRelativeFolder;
        }

        AZ::IO::FixedMaxPath projectConfigFolder;
        if (auto sourceGameRoot = fileIoBase->ResolvePath("@projectroot@"); sourceGameRoot.has_value())
        {
            projectConfigFolder = *sourceGameRoot;
            projectConfigFolder /= s_projectConfigRelativeFolder;
        }

        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);
        ClearSettings();
        
        outcome = LoadSettings((projectConfigFolder / s_builderSettingFileName).Native());

        if (!outcome.IsSuccess())
        {
            outcome = LoadSettings((defaultConfigFolder / s_builderSettingFileName).Native());
        }
        
        if (outcome.IsSuccess())
        {
            // Load presets in default folder first, then load from project folder. 
            // The same presets which loaded last will overwrite previous loaded one.
            LoadPresets(defaultConfigFolder.Native());
            LoadPresets(projectConfigFolder.Native());

            // Regenerate file mask mapping after all presets loaded
            RegenerateMappings();
        }

        return outcome;
    }

    void BuilderSettingManager::LoadPresets(AZStd::string_view presetFolder)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);

        QDirIterator it(presetFolder.data(), QStringList() << "*.preset", QDir::Files, QDirIterator::NoIteratorFlags);
        while (it.hasNext())
        {
            QString filePath = it.next();
            QFileInfo fileInfo = it.fileInfo();

            MultiplatformPresetSettings preset;
            auto result = AZ::JsonSerializationUtils::LoadObjectFromFile(preset, filePath.toUtf8().data());
            if (!result.IsSuccess())
            {
                AZ_Warning("Image Processing", false, "Failed to load preset file %s. Error: %s",
                    filePath.toUtf8().data(), result.GetError().c_str());
            }

            PresetName presetName(fileInfo.baseName().toUtf8().data());

            AZ_Warning("Image Processing", presetName == preset.GetPresetName(), "Preset file name '%s' is not"
                " same as preset name '%s'. Using preset file name as preset name",
                filePath.toUtf8().data(), preset.GetPresetName().GetCStr());

            preset.SetPresetName(presetName);

            m_presets[presetName] = PresetEntry{preset, filePath.toUtf8().data()};
        }
    }

    StringOutcome BuilderSettingManager::LoadConfigFromFolder(AZStd::string_view configFolder)
    {
        // Load builder settings
        AZStd::string settingFilePath = AZStd::string::format("%.*s%s",  aznumeric_cast<int>(configFolder.size()), 
            configFolder.data(), s_builderSettingFileName);
        auto result = LoadSettings(settingFilePath);

        // Load presets
        if (result.IsSuccess())
        {
            LoadPresets(configFolder);
            RegenerateMappings();
        }

        return result;
    }

    StringOutcome BuilderSettingManager::LoadSettings(AZStd::string_view filepath)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);

        auto result = AZ::JsonSerializationUtils::LoadObjectFromFile(*this, filepath);

        if (!result.IsSuccess())
        {
            return STRING_OUTCOME_ERROR(result.GetError());
        }

        //enable builder settings for enabled restricted platforms. These settings should be disabled by default in the setting file
#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3) \
    for (auto& buildSetting : m_builderSettings)                                                                                                                                                  \
    {                                                                                                                                                                                             \
        if (ImageProcess##PrivateName::DoesSupport(buildSetting.first))                                                                                                                           \
        {                                                                                                                                                                                         \
            buildSetting.second.m_enablePlatform = true;                                                                                                                                          \
            break;                                                                                                                                                                                \
        }                                                                                                                                                                                         \
    }
    AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif //AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
        
        return STRING_OUTCOME_SUCCESS;
    }

    StringOutcome BuilderSettingManager::WriteSettings(AZStd::string_view filepath)
    {
        AZ::JsonSerializerSettings saveSettings;
        saveSettings.m_keepDefaults = true;
        auto result = AZ::JsonSerializationUtils::SaveObjectToFile(this, filepath,
            (BuilderSettingManager*)nullptr, &saveSettings);
        if (!result.IsSuccess())
        {
            return STRING_OUTCOME_ERROR(result.GetError());
        }

        return STRING_OUTCOME_SUCCESS;
    }

    const AZStd::string& BuilderSettingManager::GetAnalysisFingerprint() const
    {
        return m_analysisFingerprint;
    }

    void BuilderSettingManager::RegenerateMappings()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);

        AZStd::string noFilter = AZStd::string();

        m_presetFilterMap.clear();

        for (const auto& presetIter : m_presets)
        {
            const MultiplatformPresetSettings& multiPreset = presetIter.second.m_multiPreset;
            const PresetSettings& preset = multiPreset.GetDefaultPreset();
            
            //Put into no filter preset list
            m_presetFilterMap[noFilter].insert(preset.m_name);

            //Put into file mask preset list if any
            for (const PlatformName& filemask : preset.m_fileMasks)
            {
                if (filemask.empty() || filemask[0] != FileMaskDelimiter)
                {
                    AZ_Warning("Image Processing", false, "File mask '%s' is invalid. It must start with '%c'.", filemask.c_str(), FileMaskDelimiter);
                    continue;
                }
                else if (filemask.size() < 2)
                {
                    AZ_Warning("Image Processing", false, "File mask '%s' is invalid. The '%c' must be followed by at least one other character.", filemask.c_str());
                    continue;
                }
                else if (filemask.find(FileMaskDelimiter, 1) != AZStd::string::npos)
                {
                    AZ_Warning("Image Processing", false, "File mask '%s' is invalid. It must contain only a single '%c' character.", filemask.c_str(), FileMaskDelimiter);
                    continue;
                }
                m_presetFilterMap[filemask].insert(preset.m_name);
            }
        }
    }

    void BuilderSettingManager::MetafilePathFromImagePath(AZStd::string_view imagePath, AZStd::string& metafilePath)
    {
        // Determine if we have a meta file
        AZ::IO::LocalFileIO fileIO;

        AZStd::string settingFilePath = AZStd::string::format("%.*s%s", aznumeric_cast<int>(imagePath.size()),
            imagePath.data(), TextureSettings::ExtensionName);
        if (fileIO.Exists(settingFilePath.c_str()))
        {
            metafilePath = settingFilePath;
            return;
        }

        metafilePath = AZStd::string();
    }

    AZStd::string GetFileMask(AZStd::string_view imageFilePath)
    {
        //get file name
        AZStd::string fileName;
        QString lowerFileName = imageFilePath.data();
        lowerFileName = lowerFileName.toLower();
        AzFramework::StringFunc::Path::GetFileName(lowerFileName.toUtf8().constData(), fileName);

        //get the substring from last '_'
        size_t lastUnderScore = fileName.find_last_of(FileMaskDelimiter);
        if (lastUnderScore != AZStd::string::npos)
        {
            return fileName.substr(lastUnderScore);
        }

        return AZStd::string();
    }

    bool BuilderSettingManager::IsValidPreset(PresetName presetName) const
    {
        if (presetName.IsEmpty())
        {
            return false;
        }

        return m_presets.find(presetName) != m_presets.end();
    }

    PresetName BuilderSettingManager::GetSuggestedPreset(AZStd::string_view imageFilePath, IImageObjectPtr imageFromFile)
    {
        PresetName emptyPreset;

        //load the image to get its size for later use
        IImageObjectPtr image = imageFromFile;
        //if the input image is empty we will try to load it from the path
        if (imageFromFile == nullptr)
        {
            image = IImageObjectPtr(LoadImageFromFile(imageFilePath));
        }

        if (image == nullptr)
        {
            return emptyPreset;
        }

        //get file mask of this image file
        AZStd::string fileMask = GetFileMask(imageFilePath);

        PresetName outPreset = emptyPreset;

        //check default presets for some file masks
        if (m_defaultPresetByFileMask.find(fileMask) != m_defaultPresetByFileMask.end())
        {
            outPreset = m_defaultPresetByFileMask[fileMask];
            if (!IsValidPreset(outPreset))
            {
                outPreset = emptyPreset;
            }
        }

        //use the preset filter map to find
        if (outPreset.IsEmpty() && !fileMask.empty())
        {
            auto& presetFilterMap = GetPresetFilterMap();
            if (presetFilterMap.find(fileMask) != presetFilterMap.end())
            {
                outPreset = *(presetFilterMap.find(fileMask)->second.begin());
            }
        }

        const PresetSettings* presetInfo = nullptr;

        if (!outPreset.IsEmpty())
        {
            presetInfo = GetPreset(outPreset);

            //special case for cubemap
            if (presetInfo && presetInfo->m_cubemapSetting)
            {
                // If it's not a latitude-longitude map or it doesn't match any cubemap layouts then reset its preset
                if (!IsValidLatLongMap(image) && CubemapLayout::GetCubemapLayoutInfo(image) == nullptr)
                {
                    outPreset = emptyPreset;
                }
            }
        }

        if (outPreset == emptyPreset)
        {
            if (image->GetAlphaContent() == EAlphaContent::eAlphaContent_Absent)
            {
                outPreset = m_defaultPreset;
            }
            else
            {
                outPreset = m_defaultPresetAlpha;
            }
        }

        //get the pixel format for selected preset
        presetInfo = GetPreset(outPreset);

        if (presetInfo)
        {
            //valid whether image size work with pixel format
            if (CPixelFormats::GetInstance().IsImageSizeValid(presetInfo->m_pixelFormat,
                image->GetWidth(0), image->GetHeight(0), false))
            {
                return outPreset;
            }
            else
            {
                AZ_Warning("Image Processing", false, "Image dimensions are not compatible with preset '%s'. The default preset will be used.", presetInfo->m_name.GetCStr());
            }
        }

        //uncompressed one which could be used for almost everything
        return m_defaultPresetNonePOT;
    }

    bool BuilderSettingManager::DoesSupportPlatform(AZStd::string_view platformId)
    {
        bool rv = m_builderSettings.find(platformId) != m_builderSettings.end();
        return rv;
    }
    
    void BuilderSettingManager::SavePresets(AZStd::string_view outputFolder)
    {
        for (const auto& element : m_presets)
        {
            const PresetEntry& presetEntry = element.second;
            AZStd::string fileName = AZStd::string::format("%s.preset", presetEntry.m_multiPreset.GetDefaultPreset().m_name.GetCStr());
            AZStd::string filePath;
            if (!AzFramework::StringFunc::Path::Join(outputFolder.data(), fileName.c_str(), filePath))
            {
                AZ_Warning("Image Processing", false, "Failed to construct path with folder '%.*s' and file: '%s' to save preset",
                    aznumeric_cast<int>(outputFolder.size()), outputFolder.data(), filePath.c_str());
                continue;
            }
            auto result = AZ::JsonSerializationUtils::SaveObjectToFile(&presetEntry.m_multiPreset, filePath);
            if (!result.IsSuccess())
            {
                AZ_Warning("Image Processing", false, "Failed to save preset '%s' to file '%s'. Error: %s", 
                    presetEntry.m_multiPreset.GetDefaultPreset().m_name.GetCStr(), filePath.c_str(), result.GetError().c_str());
            }
        }
    }


} // namespace ImageProcessingAtom
