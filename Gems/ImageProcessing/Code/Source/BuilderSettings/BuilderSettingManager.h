/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <BuilderSettings/ImageProcessingDefines.h>
#include <BuilderSettings/BuilderSettings.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/base.h>
#include <ImageProcessing/ImageObject.h>

class QSettings;
class QString;

namespace AZ
{
    template<class T> class EnvironmentVariable;
    class SerializeContext;
}

namespace ImageProcessing
{    
    class BuilderSettingManager
    {
    public:
        AZ_TYPE_INFO(CBuilderSettingManager, "{DAA55241-64FA-4A9B-A37F-C0A36B36D536}");
        AZ_CLASS_ALLOCATOR(BuilderSettingManager, AZ::SystemAllocator, 0);
        //load builder settings for all platform
        //contain builder setting for all platforms
        //this manager should be able to get texture setting for a platform


        static BuilderSettingManager* Instance();
        // life cycle management:
        static void CreateInstance();
        static void DestroyInstance();
        static void Reflect(AZ::ReflectContext* context);

        const PresetSettings* GetPreset(const AZ::Uuid presetId, const PlatformName& platform = "");

        const BuilderSettings* GetBuilderSetting(const PlatformName& platform);

        /**
        * Attempts to translate a legacy preset name into Lumberyard preset name.
        * @param legacy preset name string
        * @return A translated preset name. If no translation is available, returns the same value as input argument.
        */
        const PresetName TranslateLegacyPresetName(const PresetName& legacyName);

        /**
         * @return A list of platform supported
         */
        const PlatformNameList GetPlatformList();

        /**
         * @return A map of preset settings based on their filemasks.
         *      @key filemask string, empty string means no filemask
         *      @value set of preset setting names supporting the specified filemask
         */
        const AZStd::map<FileMask, AZStd::set<PresetName>>& GetPresetFilterMap();

        /**
         * Find preset id list based on the preset name.
         * @param preset name string
         * @return a map of preset ids whose name are specified by the input on different platforms
         *      @key platform name string
         *      @value uuid of the preset setting
         */
        const AZ::Uuid GetPresetIdFromName(const PresetName& presetName);
        
        /**
         * Find preset name based on the preset id.
         * @param uuid of the preset setting
         * @return preset name string
         */
        const PresetName GetPresetNameFromId(const AZ::Uuid& presetId);

        /**
        * Writes preset data to file using AZ::Serialization format.
        * @param filepath string to the build settings xml
        */
        StringOutcome WriteBuilderSettings(AZStd::string filepath, AZ::SerializeContext* context = nullptr);
        
        /**
        * Loads preset data from file using AZ::Serialization format.
        * @param filepath string to the build settings xml
        */
        StringOutcome LoadBuilderSettings(AZStd::string filepath, AZ::SerializeContext* context = nullptr);

        /**
        * Overload function. Loads preset data from project setting file if any
        * Otherwise, the function will load default setting file inside the gem
        */
        StringOutcome LoadBuilderSettings();

        /**
        * Loads preset data from legacy format found in RC.ini.
        * @param filepath string to RC.ini
        */
        StringOutcome LoadBuilderSettingsFromRC(AZStd::string& filePath);

        /**
         * Returns the first u32 generated from a hash of the builder settings
         * file that can be used as a version to detect changes to the file.
         */
        AZ::u32 BuilderSettingsVersion() const;

        /**
        * Provides a full path to the adjacent metafile of a given texture/image file.
        * @param Filepath string to the texture/image file.
        * @param Output filepath string to the adjacent texture/image metafile. 
        *        Will output whichever metafile is present, whether it is legacy or modern format. 
        *        If both are present, modern format is returned.
        *        If none are present, an empty string is returned.
        */
        void MetafilePathFromImagePath(const AZStd::string& imagePath, AZStd::string& metafilePath);

        /**
        * Find a suitable preset a given image file.
        * @param imageFilePath: Filepath string of the image file. The function may load the image from the path for better detection
        * @param image: an optional image object which can be used for preset selection if there is no match based file mask.
        * @return suggested preset uuid. 
        */
        AZ::Uuid GetSuggestedPreset(const AZStd::string& imageFilePath, IImageObjectPtr image = nullptr);

        bool DoesSupportPlatform(const AZStd::string& platformId);

        static const char* s_environmentVariableName;
        static AZ::EnvironmentVariable<BuilderSettingManager*> s_globalInstance;
        static AZStd::mutex s_instanceMutex;
        static const PlatformName s_defaultPlatform;

        BuilderSettingManager(){}

    private: // functions
        AZ_DISABLE_COPY_MOVE(BuilderSettingManager);

        StringOutcome ProcessPreset(QString& preset, QSettings& rcINI, PlatformNameVector& all_platforms);

        /**
         * Clear Builder Settings and any cached maps/lists
         */
        void ClearSettings();

        /**
        * Regenerate Builder Settings and any cached maps/lists
        */
        void RegenerateMappings();
        
    private: // variables

        //builder settings for each platform
        AZStd::map <PlatformName, BuilderSettings> m_builderSettings;
        AZStd::map <PresetName, PresetName> m_presetAliases;

        /**
         * Cached list of presets mapped by their file masks.
         * @Key file mask, use empty string to indicate all presets without filtering
         * @Value set of preset names that matches the file mask
         */
        AZStd::map <FileMask, AZStd::set<PresetName> > m_presetFilterMap;
                
        /**
        * A mutex to protect when modifying any map in this manager
        */
        AZStd::mutex m_presetMapLock;

        //default presets for certian file masks
        AZStd::map <FileMask, AZ::Uuid > m_defaultPresetByFileMask;

        //default preset for none power of two image
        AZ::Uuid m_defaultPresetNonePOT;

        //default preset for power of two
        AZ::Uuid m_defaultPreset;

        //default preset for power of two with alpha
        AZ::Uuid m_defaultPresetAlpha;

        //generated from hashing the builder settings file
        AZ::u32 m_builderSettingsFileVersion;
    };
} // namespace ImageProcessing
