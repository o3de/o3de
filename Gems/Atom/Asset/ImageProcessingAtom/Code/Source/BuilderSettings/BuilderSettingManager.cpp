/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "BuilderSettingManager.h"
#include <QCoreApplication>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QString>

#include <Atom/ImageProcessing/PixelFormats.h>
#include <BuilderSettings/CubemapSettings.h>
#include <BuilderSettings/TextureSettings.h>
#include <Converters/Cubemap.h>
#include <Processing/ImageToProcess.h>
#include <Processing/PixelFormatInfo.h>
#include <Processing/Utils.h>
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
    const char* BuilderSettingManager::s_defaultConfigRelativeFolder = "Gems/Atom/Asset/ImageProcessingAtom/Assets/Config/";
    const char* BuilderSettingManager::s_projectConfigRelativeFolder = "Config/AtomImageBuilder/";
    const char* BuilderSettingManager::s_builderSettingFileName = "ImageBuilder.settings";
    const char* BuilderSettingManager::s_presetFileExtension = "preset";

    const char FileMaskDelimiter = '_';

    namespace
    {
        [[maybe_unused]] static constexpr const char* const LogWindow = "Image Processing";
    }

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
                ->Version(2)
                ->Field("BuildSettings", &BuilderSettingManager::m_builderSettings)
                ->Field("PresetsByFileMask", &BuilderSettingManager::m_presetFilterMap)
                ->Field("DefaultPreset", &BuilderSettingManager::m_defaultPreset)
                ->Field("DefaultPresetAlpha", &BuilderSettingManager::m_defaultPresetAlpha)
                ->Field("DefaultPresetNonePOT", &BuilderSettingManager::m_defaultPresetNonePOT)
                // deprecated properties
                ->Field("DefaultPresetsByFileMask", &BuilderSettingManager::m_defaultPresetByFileMask)
                ->Field("AnalysisFingerprint", &BuilderSettingManager::m_analysisFingerprint);
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
    
    const PresetSettings* BuilderSettingManager::GetPreset(const PresetName& presetName, const PlatformName& platform, AZStd::string_view* settingsFilePathOut) const
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

    AZStd::vector<AZStd::string> BuilderSettingManager::GetFileMasksForPreset(const PresetName& presetName) const
    {
        AZStd::vector<AZStd::string> fileMasks;
        
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);
        for (const auto& mapping:m_presetFilterMap)
        {
            for (const auto& preset : mapping.second)
            {
                if (preset == presetName)
                {
                    fileMasks.push_back(mapping.first);
                    break;
                }
            }
        }
        return fileMasks;
    }

    const BuilderSettings* BuilderSettingManager::GetBuilderSetting(const PlatformName& platform) const
    {
        auto itr = m_builderSettings.find(platform);
        if (itr != m_builderSettings.end())
        {
            return &itr->second;
        }
        return nullptr;
    }

    const PlatformNameList BuilderSettingManager::GetPlatformList() const
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

    const AZStd::map <FileMask, AZStd::unordered_set<PresetName>>& BuilderSettingManager::GetPresetFilterMap() const
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);
        return m_presetFilterMap;
    }

    const AZStd::unordered_set<PresetName>& BuilderSettingManager::GetFullPresetList() const
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);
        AZStd::string noFilter = AZStd::string();
        return m_presetFilterMap.find(noFilter)->second;
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
    }

    StringOutcome BuilderSettingManager::LoadConfig()
    {
        StringOutcome outcome = STRING_OUTCOME_ERROR("");

        auto fileIoBase = AZ::IO::FileIOBase::GetInstance();
        if (fileIoBase == nullptr)
        {
            return AZ::Failure(
                AZStd::string("File IO instance needs to be initialized to resolve ImageProcessing builder file aliases"));
        }

        if (auto engineRoot = fileIoBase->ResolvePath("@engroot@"); engineRoot.has_value())
        {
            m_defaultConfigFolder = *engineRoot;
            m_defaultConfigFolder /= s_defaultConfigRelativeFolder;
        }

        if (auto sourceGameRoot = fileIoBase->ResolvePath("@projectroot@"); sourceGameRoot.has_value())
        {
            m_projectConfigFolder = *sourceGameRoot;
            m_projectConfigFolder /= s_projectConfigRelativeFolder;
        }

        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);
        ClearSettings();
        
        outcome = LoadSettings();
        
        if (outcome.IsSuccess())
        {
            // Load presets in default folder first, then load from project folder. 
            // The same presets which loaded last will overwrite previous loaded one.
            LoadPresets(m_defaultConfigFolder.Native());
            LoadPresets(m_projectConfigFolder.Native());
        }

        // Collect extra file masks from preset files
        CollectFileMasksFromPresets();

        
        if (QCoreApplication::instance())
        {
            m_fileWatcher.reset(new QFileSystemWatcher);
            // track preset files
            // Note, the QT signal would only works for AP but not AssetBuilder
            // We use file time stamp to track preset file change in builder's CreateJob
            for (auto& preset : m_presets)
            {
                m_fileWatcher.data()->addPath(QString(preset.second.m_presetFilePath.c_str()));
            }
            m_fileWatcher.data()->addPath(QString(m_defaultConfigFolder.c_str()));
            m_fileWatcher.data()->addPath(QString(m_projectConfigFolder.c_str()));
            QObject::connect(m_fileWatcher.data(), &QFileSystemWatcher::fileChanged, this, &BuilderSettingManager::OnFileChanged);
            QObject::connect(m_fileWatcher.data(), &QFileSystemWatcher::directoryChanged, this, &BuilderSettingManager::OnFolderChanged);
        }

        return outcome;
    }

    void BuilderSettingManager::LoadPresets(AZStd::string_view presetFolder)
    {
        QDirIterator it(presetFolder.data(), QStringList() << "*.preset", QDir::Files, QDirIterator::NoIteratorFlags);
        while (it.hasNext())
        {
            QString filePath = it.next();
            LoadPreset(filePath.toUtf8().data());
        }
    }

    bool BuilderSettingManager::LoadPreset(const AZStd::string& filePath)
    {
        QFileInfo fileInfo (filePath.c_str());

        if (!fileInfo.exists())
        {
            return false;
        }

        MultiplatformPresetSettings preset;
        auto result = AZ::JsonSerializationUtils::LoadObjectFromFile(preset, filePath);
        if (!result.IsSuccess())
        {
            AZ_Warning(LogWindow, false, "Failed to load preset file %s. Error: %s",
                filePath.c_str(), result.GetError().c_str());
            return false;
        }

        PresetName presetName(fileInfo.baseName().toUtf8().data());

        AZ_Warning(LogWindow, presetName == preset.GetPresetName(), "Preset file name '%s' is not"
            " same as preset name '%s'. Using preset file name as preset name",
            filePath.c_str(), preset.GetPresetName().GetCStr());

        preset.SetPresetName(presetName);

        m_presets[presetName] = PresetEntry{preset, filePath.c_str(), fileInfo.lastModified()};
        return true;
    }

    void BuilderSettingManager::ReloadPreset(const PresetName& presetName)
    {
        // Find the preset file from project or default config folder
        AZStd::string presetFileName = AZStd::string::format("%s.%s", presetName.GetCStr(), s_presetFileExtension);
        AZ::IO::FixedMaxPath filePath = m_projectConfigFolder/presetFileName;
        QFileInfo fileInfo (filePath.c_str());
        if (!fileInfo.exists())
        {
            filePath = (m_defaultConfigFolder/presetFileName).c_str();
            fileInfo = QFileInfo(filePath.c_str());
        }
        
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);

        //Skip the loading if the file wasn't chagned
        if (fileInfo.exists())
        {
            if (m_presets.find(presetName) != m_presets.end())
            {
                if (m_presets[presetName].m_lastModifiedTime == fileInfo.lastModified()
                    && m_presets[presetName].m_presetFilePath == filePath.c_str())
                {
                    return;
                }
            }
        }

        // remove preset
        m_presets.erase(presetName);

        if (fileInfo.exists())
        {
            LoadPreset(filePath.c_str());
        }
    }

    StringOutcome BuilderSettingManager::LoadConfigFromFolder(AZStd::string_view configFolder)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);

        // Load builder settings
        AZStd::string settingFilePath = AZStd::string::format("%.*s%s",  aznumeric_cast<int>(configFolder.size()), 
            configFolder.data(), s_builderSettingFileName);
        auto result = LoadSettings(settingFilePath);

        // Load presets
        if (result.IsSuccess())
        {
            LoadPresets(configFolder);
        }

        return result;
    }

    void BuilderSettingManager::ReportDeprecatedSettings()
    {
        // reported deprecated attributes in image builder settings
        if (!m_analysisFingerprint.empty())
        {
            AZ_Warning(LogWindow, false, "'AnalysisFingerprint' is deprecated and it should be removed from file [%s]", s_builderSettingFileName);
        }
        if (!m_defaultPresetByFileMask.empty())
        {
            AZ_Warning(LogWindow, false, "'DefaultPresetsByFileMask' is deprecated and it should be removed from file [%s]. Use PresetsByFileMask instead", s_builderSettingFileName);
        }
    }

    StringOutcome BuilderSettingManager::LoadSettings()
    {
        // If the project image build setting file exist, it will merge image builder settings from project folder to the settings from default config folder.
        bool needMerge = false;
        AZStd::string projectSettingFile{ (m_projectConfigFolder / s_builderSettingFileName).Native() };

        if (AZ::IO::SystemFile::Exists(projectSettingFile.c_str()))
        {
            needMerge = true;
        }

        AZ::Outcome<void, AZStd::string> outcome;
        AZStd::string defaultSettingFile{ (m_defaultConfigFolder / s_builderSettingFileName).Native() };
        if (needMerge)
        {
            auto outcome1 = AZ::JsonSerializationUtils::ReadJsonFile(defaultSettingFile);
            auto outcome2 = AZ::JsonSerializationUtils::ReadJsonFile(projectSettingFile);

            // return error if it failed to load default settings
            if (!outcome1.IsSuccess())
            {
                return STRING_OUTCOME_ERROR(outcome1.GetError());
            }

            // if project config was loaded successfully, apply merge patch
            rapidjson::Document& originDoc = outcome1.GetValue();
            if (outcome2.IsSuccess())
            {
                const rapidjson::Document& patchDoc = outcome2.GetValue();
                AZ::JsonSerializationResult::ResultCode result =
                    AZ::JsonSerialization::ApplyPatch(originDoc, originDoc.GetAllocator(), patchDoc, AZ::JsonMergeApproach::JsonMergePatch);

                if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed)
                {
                    AZStd::vector<char> outBuffer;
                    AZ::IO::ByteContainerStream<AZStd::vector<char>> outStream{ &outBuffer };
                    AZ::JsonSerializationUtils::WriteJsonStream(originDoc, outStream);

                    outStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

                    outcome = AZ::JsonSerializationUtils::LoadObjectFromStream(*this, outStream);
                    if (!outcome.IsSuccess())
                    {
                        return STRING_OUTCOME_ERROR(outcome.GetError());
                    }
                    
                    ReportDeprecatedSettings();


                    // Generate config file fingerprint
                    outStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
                    AZ::u64 hash = AssetBuilderSDK::GetHashFromIOStream(outStream);
                    m_analysisFingerprint = AZStd::string::format("%llX", hash);
                }
                else
                {
                    needMerge = false;
                    AZ_Warning(LogWindow, false, "Failed to fully merge data into image builder settings. Skipping project build setting file [%s]", projectSettingFile.c_str());
                }
            }
            else
            {
                AZ_Warning(LogWindow, false, "Failed to load project setting file [%s]. Skipping", projectSettingFile.c_str());
            }
        }

        if (!needMerge)
        {
            outcome = AZ::JsonSerializationUtils::LoadObjectFromFile(*this, defaultSettingFile);
            if (!outcome.IsSuccess())
            {
                return STRING_OUTCOME_ERROR(outcome.GetError());
            }

            ReportDeprecatedSettings();

            // Generate config file fingerprint
            AZ::u64 hash = AssetBuilderSDK::GetFileHash(defaultSettingFile.c_str());
            m_analysisFingerprint = AZStd::string::format("%llX", hash);
        }

        return STRING_OUTCOME_SUCCESS;
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

    void BuilderSettingManager::CollectFileMasksFromPresets()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);

        AZStd::string noFilter = AZStd::string();
        
        AZStd::string extraString;

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
                    AZ_Warning(LogWindow, false, "File mask '%s' is invalid. It must start with '%c'.", filemask.c_str(), FileMaskDelimiter);
                    continue;
                }
                else if (filemask.size() < 2)
                {
                    AZ_Warning(LogWindow, false, "File mask '%s' is invalid. The '%c' must be followed by at least one other character.", filemask.c_str());
                    continue;
                }
                else if (filemask.find(FileMaskDelimiter, 1) != AZStd::string::npos)
                {
                    AZ_Warning(LogWindow, false, "File mask '%s' is invalid. It must contain only a single '%c' character.", filemask.c_str(), FileMaskDelimiter);
                    continue;
                }

                extraString += (filemask + preset.m_name.GetCStr());

                m_presetFilterMap[filemask].insert(preset.m_name);
            }
        }

        if (!extraString.empty())
        {
            AZ::u64 hash = AZStd::hash<AZStd::string>{}(extraString);
            m_analysisFingerprint += AZStd::string::format("%llX", hash);
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

    PresetName BuilderSettingManager::GetSuggestedPreset(AZStd::string_view imageFilePath) const
    {
        PresetName emptyPreset;

        //get file mask of this image file
        AZStd::string fileMask = GetFileMask(imageFilePath);

        PresetName outPreset = emptyPreset;

        //use the preset filter map to find
        if (outPreset.IsEmpty() && !fileMask.empty())
        {
            auto& presetFilterMap = GetPresetFilterMap();
            if (presetFilterMap.find(fileMask) != presetFilterMap.end())
            {
                outPreset = *(presetFilterMap.find(fileMask)->second.begin());
            }
        }

        if (outPreset == emptyPreset)
        {
            outPreset = m_defaultPreset;
        }

        return outPreset;
    }

    AZStd::vector<AZStd::string> BuilderSettingManager::GetPossiblePresetPaths(const PresetName& presetName) const
    {
        AZStd::vector<AZStd::string> paths;
        AZStd::string presetFile = AZStd::string::format("%s.preset", presetName.GetCStr());
        paths.push_back((m_defaultConfigFolder / presetFile).c_str());
        paths.push_back((m_projectConfigFolder / presetFile).c_str());
        return paths;
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
                AZ_Warning(LogWindow, false, "Failed to construct path with folder '%.*s' and file: '%s' to save preset",
                    aznumeric_cast<int>(outputFolder.size()), outputFolder.data(), filePath.c_str());
                continue;
            }
            auto result = AZ::JsonSerializationUtils::SaveObjectToFile(&presetEntry.m_multiPreset, filePath);
            if (!result.IsSuccess())
            {
                AZ_Warning(LogWindow, false, "Failed to save preset '%s' to file '%s'. Error: %s", 
                    presetEntry.m_multiPreset.GetDefaultPreset().m_name.GetCStr(), filePath.c_str(), result.GetError().c_str());
            }
        }
    }

    void BuilderSettingManager::OnFileChanged(const QString &path)
    {
        // handles preset file change
        // Note: this signal only works with AP but not AssetBuilder
        AZ_TracePrintf(LogWindow, "File changed %s\n", path.toUtf8().data());
        QFileInfo info(path);

        // skip if the file is not a preset file
        // Note: for .settings file change it's handled when restart AP. 
        if (info.suffix() != s_presetFileExtension)
        {
            return;
        }
        
        ReloadPreset(PresetName(info.baseName().toUtf8().data()));
    }
    
    void BuilderSettingManager::OnFolderChanged([[maybe_unused]] const QString &path)
    {
        // handles new file added or removed
        // Note: this signal only works with AP but not AssetBuilder
        AZ_TracePrintf(LogWindow, "folder changed %s\n", path.toUtf8().data());

        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_presetMapLock);
        m_presets.clear();
        LoadPresets(m_defaultConfigFolder.Native());
        LoadPresets(m_projectConfigFolder.Native());

        for (auto& preset : m_presets)
        {
            m_fileWatcher.data()->addPath(QString(preset.second.m_presetFilePath.c_str()));
        }
    }
} // namespace ImageProcessingAtom
