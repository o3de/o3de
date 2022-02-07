/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BuilderSettings/ImageProcessingDefines.h>
#include <BuilderSettings/BuilderSettings.h>
#include <AzCore/base.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/containers/set.h>
#include <Atom/ImageProcessing/ImageObject.h>

#include <QDateTime>
#include <QFileSystemWatcher>
#include <QScopedPointer>

class QSettings;
class QString;

namespace AZ
{
    template<class T>
    class EnvironmentVariable;
    class SerializeContext;
}

namespace ImageProcessingAtom
{
    /**
     * BuilderSettingManager is a singleton which responses to manage preset settings and some global settings for image builder.
     * It loads all presets from *.settings file. The settings file need to be loaded before builder or editor starting process any image.
     * Preset is a collection of some parameters which used for convert and export image to formats used in runtime. It's used as template to provide
     * default settings when process certain type of texture
     * When process an image, a texture setting will be loaded (from legacy .exportsettings or new .imagesettings) or
     * generated for this image. The texture setting will need to reference to a preset which is used to initialize the texture setting.
     * Each preset setting may have different values on different platform, but they are using same uuid.
     */
    class BuilderSettingManager
        : public QObject // required for using QFileSystemWatcher
    {
        friend class ImageProcessingTest;

    public:
        AZ_TYPE_INFO(BuilderSettingManager, "{8E95726D-4E3A-446C-99A6-D02863640EAC}");
        AZ_CLASS_ALLOCATOR(BuilderSettingManager, AZ::SystemAllocator, 0);
        
        static BuilderSettingManager* Instance();
        // life cycle management:
        static void CreateInstance();
        static void DestroyInstance();
        static void Reflect(AZ::ReflectContext* context);
        
        const PresetSettings* GetPreset(const PresetName& presetName, const PlatformName& platform = "", AZStd::string_view* settingsFilePathOut = nullptr) const;

        AZStd::vector<AZStd::string> GetFileMasksForPreset(const PresetName& presetName) const;

        const BuilderSettings* GetBuilderSetting(const PlatformName& platform) const;
                
        //! Return A list of platform supported
        const PlatformNameList GetPlatformList() const;

        //! Return A map of preset settings based on their filemasks.
        //!      @key filemask string, empty string means no filemask
        //!      @value set of preset setting names supporting the specified filemask
        const AZStd::map<FileMask, AZStd::unordered_set<PresetName>>& GetPresetFilterMap() const;

        const AZStd::unordered_set<PresetName>& GetFullPresetList() const;

        //! Find preset name based on the preset id.
        const PresetName GetPresetNameFromId(const AZ::Uuid& presetId);

        //! Load configurations files from both project and gem default config folders
        StringOutcome LoadConfig();
        
        //! Load configurations files from a folder which includes builder settings and presets
        //! Note: this is only used for unit test. Use LoadConfig() for editor or game launcher
         StringOutcome LoadConfigFromFolder(AZStd::string_view configFolder);

        //! Reload preset from config folders
        void ReloadPreset(const PresetName& presetName);
                
        const AZStd::string& GetAnalysisFingerprint() const;

        //! Provides a full path to the adjacent metafile of a given texture/image file.
        //! @param imagePath File path string to the texture/image file.
        //! @param Output metafilePath File path string to the adjacent texture/image metafile.
        void MetafilePathFromImagePath(AZStd::string_view imagePath, AZStd::string& metafilePath);

        //! Find a suitable preset a given image file.
        //! @param imageFilePath: Filepath string of the image file. The function may load the image from the path for better detection
        //! @param image: an optional image object which can be used for preset selection if there is no match based file mask.
        //! @return suggested preset name.
        PresetName GetSuggestedPreset(AZStd::string_view imageFilePath) const;

        //! Get the possible preset config's full file paths
        //! This function is only used for setting up image's source dependency if a preset file is missing
        //! Otherwise, the preset's file path can be retrieved in GetPreset() function
        AZStd::vector<AZStd::string> GetPossiblePresetPaths(const PresetName& presetName) const;

        bool IsValidPreset(PresetName presetName) const;

        bool DoesSupportPlatform(AZStd::string_view platformId);

        static const char* s_environmentVariableName;
        static AZ::EnvironmentVariable<BuilderSettingManager*> s_globalInstance;
        static AZStd::mutex s_instanceMutex;
        static const PlatformName s_defaultPlatform;

        // The relative folder where the default image builder configuration files (builder setting, presets) are. 
        static const char* s_defaultConfigRelativeFolder;
        // The relative folder where project's image builder configuration files are
        static const char* s_projectConfigRelativeFolder;
        // Builder setting file name
        static const char* s_builderSettingFileName;
        static const char* s_presetFileExtension;

        BuilderSettingManager() = default;

    private: // functions
        AZ_DISABLE_COPY_MOVE(BuilderSettingManager);

        // Write image builder setting to the file specified by filepath
        StringOutcome WriteSettings(AZStd::string_view filepath);
        // Load image builder settings from the file specified by filepath
        StringOutcome LoadSettings(AZStd::string_view filepath);

        // Load merge image builder settings (project and default)
        StringOutcome LoadSettings();

        // report warnings for the deprecated properties in image builder setting data
        void ReportDeprecatedSettings();

        // Clear Builder Settings and any cached maps/lists
        void ClearSettings();

        // collect file masks 
        void CollectFileMasksFromPresets();

        // Functions to save/load preset from a folder
        void SavePresets(AZStd::string_view outputFolder);
        void LoadPresets(AZStd::string_view presetFolder);

        // Load a preset to m_presets and return true if success
        bool LoadPreset(const AZStd::string& filePath);

        // handle preset files changes
        void OnFileChanged(const QString &path);
        void OnFolderChanged(const QString &path);

    private: // variables

        struct PresetEntry
        {
            MultiplatformPresetSettings m_multiPreset;
            AZStd::string m_presetFilePath; // Can be used for debug output
            QDateTime m_lastModifiedTime;
        };

        // Builder settings for each platform
        AZStd::map <PlatformName, BuilderSettings> m_builderSettings;

        AZStd::unordered_map<PresetName, PresetEntry> m_presets;

        // a list of presets mapped by their file masks.
        // @Key file mask, use empty string to indicate all presets without filtering
        // @Value set of preset names that matches the file mask
        AZStd::map <FileMask, AZStd::unordered_set<PresetName>> m_presetFilterMap;

        // A mutex to protect when modifying any map in this manager
        mutable AZStd::recursive_mutex m_presetMapLock;

        // Default presets for certain file masks
        AZStd::map <FileMask, PresetName > m_defaultPresetByFileMask;

        // Default preset for none power of two image
        PresetName m_defaultPresetNonePOT;

        // Default preset for power of two
        PresetName m_defaultPreset;

        // Default preset for power of two with alpha
        PresetName m_defaultPresetAlpha;

        // Image builder's version
        AZStd::string m_analysisFingerprint;

        // default config folder
        AZ::IO::FixedMaxPath m_defaultConfigFolder;

        // project config folder
        AZ::IO::FixedMaxPath m_projectConfigFolder;

        // File system watcher to detect preset file changes
        QScopedPointer<QFileSystemWatcher> m_fileWatcher;
    };
} // namespace ImageProcessingAtom
