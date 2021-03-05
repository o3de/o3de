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

#include <AzCore/std/string/string.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/IO/SystemFile.h> //AZ_MAX_PATH_LEN
#include <AzCore/Outcome/Outcome.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzToolsFramework/Asset/AssetBundler.h>
#include <AzToolsFramework/Asset/AssetUtils.h>


namespace AssetBundler
{
    enum CommandType
    {
        Invalid,
        Seeds,
        AssetLists,
        ComparisonRules,
        Compare,
        BundleSettings,
        Bundles,
        BundleSeed
    };

    ////////////////////////////////////////////////////////////////////////////////////////////
    // General
    extern const char* AppWindowName;
    extern const char* AppWindowNameVerbose;
    extern const char* HelpFlag;
    extern const char* HelpFlagAlias;
    extern const char* VerboseFlag;
    extern const char* SaveFlag;
    extern const char* PlatformArg;
    extern const char* PrintFlag;
    extern const char* AssetCatalogFileArg;
    extern const char* AllowOverwritesFlag;
    extern const char* IgnoreFileCaseFlag;
    extern const char* ProjectArg;
    ////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Seeds
    extern const char* SeedsCommand;
    extern const char* SeedListFileArg;
    extern const char* AddSeedArg;
    extern const char* RemoveSeedArg;
    extern const char* AddPlatformToAllSeedsFlag;
    extern const char* RemovePlatformFromAllSeedsFlag;
    extern const char* UpdateSeedPathArg;
    extern const char* RemoveSeedPathArg;
    ////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Asset Lists
    extern const char* AssetListsCommand;
    extern const char* AssetListFileArg;
    extern const char* AddDefaultSeedListFilesFlag;
    extern const char* DryRunFlag;
    extern const char* GenerateDebugFileFlag;
    extern const char* SkipArg;
    ////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Comparison Rules
    extern const char* ComparisonRulesCommand;
    extern const char* ComparisonRulesFileArg;
    extern const char* ComparisonTypeArg;
    extern const char* ComparisonFilePatternArg;
    extern const char* ComparisonFilePatternTypeArg;
    extern const char* ComparisonTokenNameArg;
    extern const char* ComparisonFirstInputArg;
    extern const char* ComparisonSecondInputArg;
    extern const char* AddComparisonStepArg;
    extern const char* RemoveComparisonStepArg;
    extern const char* MoveComparisonStepArg;
    extern const char* EditComparisonStepArg;
    ////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Compare
    extern const char* CompareCommand;
    extern const char* CompareFirstFileArg;
    extern const char* CompareSecondFileArg;
    extern const char* CompareOutputFileArg;
    extern const char* ComparePrintArg;
    extern const char* IntersectionCountArg;
    ////////////////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////
    // Bundle Settings 
    extern const char* BundleSettingsCommand;
    extern const char* BundleSettingsFileArg;
    extern const char* OutputBundlePathArg;
    extern const char* BundleVersionArg;
    extern const char* MaxBundleSizeArg;
    ////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Bundles
    extern const char* BundlesCommand;
    ////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Bundle Seed
    extern const char* BundleSeedCommand;
    ////////////////////////////////////////////////////////////////////////////////////////////

    extern const char* AssetCatalogFilename;
    extern char g_cachedEngineRoot[AZ_MAX_PATH_LEN];
    static const size_t MaxErrorMessageLength = 4096;

    //! This struct stores gem related information
    struct GemInfo
    {
        AZ_CLASS_ALLOCATOR(GemInfo, AZ::SystemAllocator, 0);
        GemInfo(AZStd::string name, AZStd::string relativeFilePath, AZStd::string absoluteFilePath);
        GemInfo() = default;
        AZStd::string m_gemName;
        AZStd::string m_relativeFilePath;
        AZStd::string m_absoluteFilePath;
    };


    // The Warning Absorber is used to absorb warnings 
    // One case that this is being used is during loading of the asset catalog.
    // During loading the asset catalog tries to communicate to the AP which is not required for this application. 
    class WarningAbsorber
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        WarningAbsorber();
        ~WarningAbsorber();

        bool OnWarning(const char* window, const char* message) override;
        bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;
    };

    // computes the asset alias and game name either through the asset catalog file provided by the user or using the platform and game folder 
    AZ::Outcome<void, AZStd::string> ComputeAssetAliasAndGameName(const AZStd::string& platformIdentifier, const AZStd::string& assetCatalogFile, AZStd::string& assetAlias, AZStd::string& gameName);

    // Computes the engine root and cache it locally
    bool ComputeEngineRoot();

    // Retrurns the engine root
    const char* GetEngineRoot();

    /** 
    * Determines the name of the currently enabled game project
    * @return Current Project name on success, error message on failure
    */
    AZ::Outcome<AZStd::string, AZStd::string> GetCurrentProjectName();


    /**
    * Constructs an absolute path to the project folder found at: dev/ProjectName
    * @param engineRoot The absolute path of the dev/ folder
    * @param projectName A project present in the dev/ folder
    * @return Absolute path of the Project Folder on success, error message on failure
    */
    AZ::Outcome<AZStd::string, AZStd::string> GetProjectFolderPath(const AZStd::string& engineRoot, const AZStd::string& projectName);

    /**
    * Constructs an absolute path to the project-specific cache folder found at: dev/Cache/ProjectName
    * @param engineRoot The absolute path of the dev/ folder
    * @param projectName A project present in the dev/ folder
    * @return Absolute path of the project-specific cache folder on success, error message on failure
    */
    AZ::Outcome<AZStd::string, AZStd::string> GetProjectCacheFolderPath(const AZStd::string& engineRoot, const AZStd::string& projectName);

    /**
    * Calculates the list of enabled platforms for the input project by reading the folder names inside the project-specific cache folder.
    * If the Asset Processor has not been run yet, or has not been run since the enabled platform list inside AssetProcessorPlatformConfig.ini 
    *   was changed, the output of this function will be incorrect.
    *
    * @param projectCacheFolder The directory of a project-specific cache folder: dev/Cache/ProjectName
    * @param platformNames [out] The list of platforms enabled in the project
    * @return void on success, error message on failure
    */
    AZ::Outcome<void, AZStd::string> GetPlatformNamesFromCacheFolder(const AZStd::string& projectCacheFolder, AZStd::vector<AZStd::string>& platformNames);

    /**
    * Computes the absolute path to the Asset Catalog file for a specified project and platform.
    * With platform set as "pc" and project as "ProjectName", the path will resemble: C:/dev/Cache/ProjectName/pc/projectname/assetcatalog.xml
    *
    * @param pathToCacheFolder The absolute path to the Cache folder. ex: C:/dev/Cache
    * @param platformIdentifier The platform identifier of the desired Asset Catalog. Valid inputs can be found by reading the folder names 
    *                             found inside dev/Cache/ProjectName
    * @param projectName The name of the project you want to search
    * @return Absolute Path to the Asset Catalog file on success, error message on failure
    */
    AZ::Outcome<AZStd::string, AZStd::string> GetAssetCatalogFilePath(const char* pathToCacheFolder, const char* platformIdentifier, const char* projectName);

    /**
    * Computes the absolute path to the platform-specific Cache folder where product assets are stored. 
    * With platform set as "pc" and project as "ProjectName", the path will resemble: C:/dev/Cache/ProjectName/pc/projectname/
    * 
    * @param projectSpecificCacheFolderAbsolutePath The absolute path to the Cache folder. Example: C:/dev/Cache/ProjectName
    * @param platform the platform of the desired cache location
    * @param projectName The name of the current project
    * @return Absolute path to the platform-specific Cache folder where product assets are stored 
    */
    AZStd::string GetPlatformSpecificCacheFolderPath(const AZStd::string& projectSpecificCacheFolderAbsolutePath, const AZStd::string& platform, const AZStd::string& projectName);

    AZStd::string GenerateKeyFromAbsolutePath(const AZStd::string& absoluteFilePath);

    void ConvertToRelativePath(const AZStd::string& parentFolderPath, AZStd::string& absoluteFilePath);

    AZ::Outcome<void, AZStd::string> MakePath(const AZStd::string& path);

    //! Add the specified platform identifier to the filename
    void AddPlatformIdentifier(AZStd::string& filePath, const AZStd::string& platformIdentifier);

    //! Returns the list of platforms that exist on-disk for the input file path.
    AzFramework::PlatformFlags GetPlatformsOnDiskForPlatformSpecificFile(const AZStd::string& platformIndependentAbsolutePath);

    //! Returns a map of <absolute file path, source folder display name> of all default Seed List files for the current game project.
    AZStd::unordered_map<AZStd::string, AZStd::string> GetDefaultSeedListFiles(const char* root, const char* projectName, const AZStd::vector<AzToolsFramework::AssetUtils::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlags);

    //! Returns a vector of relative paths to Assets that should be included as default Seeds, but are not already in a Seed List file.
    AZStd::vector<AZStd::string> GetDefaultSeeds(const char* root, const char* projectName);

    //! Returns the absolute path of {ProjectName}_Dependencies.xml
    AZStd::string GetProjectDependenciesFile(const char* root, const char* projectName);

    //! Returns the absolute path of the project dependencies file in the default project template
    AZStd::string GetProjectDependenciesFileTemplate(const char* root);

    //! Creates the ProjectName_Dependencies.xml file if it does not exist, and adds returns the relative path to the asset in the Cache.
    AZStd::string GetProjectDependenciesAssetPath(const char* root, const char* projectName);

    //! Returns the map from gem seed list file path to gem name
    AZStd::unordered_map<AZStd::string, AZStd::string> GetGemSeedListFilePathToGemNameMap(const AZStd::vector<AzToolsFramework::AssetUtils::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlags);

    //! Given an absolute gem seed file path determines whether the file is valid for the current game project.
    //! This method is for validating gem seed list files only.
    bool IsGemSeedFilePathValid(const char* root, AZStd::string seedAbsoluteFilePath, const AZStd::vector<AzToolsFramework::AssetUtils::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlags);

    //! Returns platformFlags of all enabled platforms by parsing all the asset processor config files.
    //! Please note that the game project could be in a different location to the engine therefore we need the assetRoot param.
    AzFramework::PlatformFlags GetEnabledPlatformFlags(const char* root, const char* assetRoot, const char* gameName);

    //! Filepath is a helper class that is used to find the absolute path of a file
    //! if the inputted file path is an absolute path than it does nothing
    //! if the inputted file path is a relative path than based on whether the user
    //! also inputted a root directory it computes the absolute path, 
    //! if root directory is provided it uses that otherwise it uses the engine root as the default root folder.
    class FilePath
    {
    public:
        AZ_CLASS_ALLOCATOR(FilePath, AZ::SystemAllocator, 0);
        explicit FilePath(const AZStd::string& filePath, AZStd::string platformIdentifier = AZStd::string(), bool checkFileCase = false, bool ignoreFileCase = false);
        explicit FilePath(const AZStd::string& filePath, bool checkFileCase, bool ignoreFileCase);
        FilePath() = default;
        const AZStd::string& AbsolutePath() const;
        const AZStd::string& OriginalPath() const;
        AZStd::string ErrorString() const;
        bool IsValid() const;
    private:
        void ComputeAbsolutePath(AZStd::string& filePath, const AZStd::string& platformIdentifier, bool checkFileCase, bool ignoreFileCase);

        AZStd::string m_absolutePath;
        AZStd::string m_originalPath;
        AZStd::string m_errorString;
        bool m_validPath = false;
    };

    void ValidateOutputFilePath(FilePath filePath, const char* format, ...);

    //! ScopedTraceHandler can be used to handle and report errors 
    class ScopedTraceHandler : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        
        ScopedTraceHandler();
        ~ScopedTraceHandler();

        //! TraceMessageBus Interface
        bool OnError(const char* /*window*/, const char* /*message*/) override;
        //////////////////////////////////////////////////////////
        //! Returns the error count
        int GetErrorCount() const;
        //! Report all the errors
        void ReportErrors();
        //! Clear all the errors
        void ClearErrors();
    private:
        AZStd::vector<AZStd::string> m_errors;
        bool m_reportingError = false;
    };

    AZ::Outcome<AzToolsFramework::AssetFileInfoListComparison::ComparisonType, AZStd::string> ParseComparisonType(const AZStd::string& comparisonType);
    AZ::Outcome<AzToolsFramework::AssetFileInfoListComparison::FilePatternType, AZStd::string> ParseFilePatternType(const AZStd::string& filePatternType);
    bool LooksLikePath(const AZStd::string& inputString);
    bool LooksLikeWildcardPattern(const AZStd::string& inputPattern);
}
