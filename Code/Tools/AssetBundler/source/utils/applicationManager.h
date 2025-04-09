/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzToolsFramework/Asset/AssetBundler.h>
#include <AzToolsFramework/Asset/AssetUtils.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <source/utils/utils.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalogManager.h>
#endif
namespace AssetBundler
{
    struct SeedsParams
    {
        AZ_CLASS_ALLOCATOR(SeedsParams, AZ::SystemAllocator);

        FilePath m_seedListFile;
        AZStd::vector<AZStd::string> m_addSeedList;
        AZStd::vector<AZStd::string> m_removeSeedList;

        bool m_addPlatformToAllSeeds = false;
        bool m_removePlatformFromAllSeeds = false;
        bool m_updateSeedPathHint = false;
        bool m_removeSeedPathHint = false;
        bool m_ignoreFileCase = false;

        bool m_save = false;
        bool m_print = false;

        AzFramework::PlatformFlags m_platformFlags = AzFramework::PlatformFlags::Platform_NONE;
        FilePath m_assetCatalogFile;
    };

    struct AssetListsParams
    {
        AZ_CLASS_ALLOCATOR(AssetListsParams, AZ::SystemAllocator);

        FilePath m_assetListFile;
        AZStd::vector<FilePath> m_seedListFiles;
        AZStd::vector<AZStd::string> m_addSeedList;
        AZStd::vector<AZStd::string> m_skipList;

        bool m_addDefaultSeedListFiles = false;

        bool m_print = false;
        bool m_dryRun = false;
        bool m_generateDebugFile = false;
        bool m_allowOverwrites = false;

        AzFramework::PlatformFlags m_platformFlags = AzFramework::PlatformFlags::Platform_NONE;
        FilePath m_assetCatalogFile;
    };

    enum ComparisonRulesStepAction
    {
        Add,
        AddToEnd,
        Remove,
        Move,
        Edit,
        Default,
    };

    struct ComparisonRulesParams
    {
        AZ_CLASS_ALLOCATOR(ComparisonRulesParams, AZ::SystemAllocator);

        AZStd::vector<AzToolsFramework::AssetFileInfoListComparison::ComparisonType> m_comparisonTypeList;
        AZStd::vector<AZStd::string> m_filePatternList;
        AZStd::vector<AzToolsFramework::AssetFileInfoListComparison::FilePatternType> m_filePatternTypeList;
        AZStd::vector<AZStd::string> m_tokenNamesList;
        AZStd::vector<AZStd::string> m_firstInputList;
        AZStd::vector<AZStd::string> m_secondInputList;
        FilePath m_comparisonRulesFile;

        ComparisonRulesStepAction m_comparisonRulesStepAction = ComparisonRulesStepAction::Default;
        size_t m_initialLine = 0;
        size_t m_destinationLine = 0;

        unsigned int m_intersectionCount = 0;

        bool m_print = false;
    };

    struct ComparisonParams
    {
        AZ_CLASS_ALLOCATOR(ComparisonParams, AZ::SystemAllocator);
        // Comparison input/output
        AZStd::vector<AZStd::string> m_firstCompareFile;
        AZStd::vector<AZStd::string> m_secondCompareFile;
        AZStd::vector<AZStd::string> m_outputs;

        AZStd::vector<AZStd::string> m_printComparisons;

        bool m_printLast = false;
        bool m_allowOverwrites = false;

        AzFramework::PlatformFlags m_platformFlags = AzFramework::PlatformFlags::Platform_NONE;

        // Comparison definitions
        FilePath m_comparisonRulesFile;
        ComparisonRulesParams m_comparisonRulesParams;
    };

    struct BundleSettingsParams
    {
        AZ_CLASS_ALLOCATOR(BundleSettingsParams, AZ::SystemAllocator);

        FilePath m_bundleSettingsFile;
        FilePath m_assetListFile;
        FilePath m_outputBundlePath;

        int m_bundleVersion = -1;
        int m_maxBundleSizeInMB = -1;

        bool m_print = false;

        AzFramework::PlatformFlags m_platformFlags = AzFramework::PlatformFlags::Platform_NONE;
    };

    struct BundlesParams
    {
        AZ_CLASS_ALLOCATOR(BundlesParams, AZ::SystemAllocator);

        FilePath m_bundleSettingsFile;
        FilePath m_assetListFile;
        FilePath m_outputBundlePath;

        int m_bundleVersion = -1;
        int m_maxBundleSizeInMB = -1;

        AzFramework::PlatformFlags m_platformFlags = AzFramework::PlatformFlags::Platform_NONE;

        bool m_allowOverwrites = false;
    };

    typedef AZStd::vector<BundlesParams> BundlesParamsList;

    struct BundleSeedParams
    {
        AZ_CLASS_ALLOCATOR(BundleSeedParams, AZ::SystemAllocator);

        AZStd::vector<AZStd::string> m_addSeedList;

        BundlesParams m_bundleParams;
    };

    class ApplicationManager
        : public QObject
        , public AZ::Debug::TraceMessageBus::Handler
        , public AzToolsFramework::ToolsApplication
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ApplicationManager, AZ::SystemAllocator)
        ApplicationManager(int* argc, char*** argv, QObject* parent = nullptr);
        ApplicationManager(int* argc, char*** argv, QObject* parent, AZ::ComponentApplicationSettings componentAppSettings);
        ApplicationManager(int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings);
        virtual ~ApplicationManager();

        virtual bool Init();
        void DestroyApplication();
        virtual bool Run();

        ////////////////////////////////////////////////////////////////////////////////////////////
        // AzFramework::Application overrides
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        ////////////////////////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////////////////////////
        // TraceMessageBus Interface
        bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPrintf(const char* window, const char* message) override;
        ////////////////////////////////////////////////////////////////////////////////////////////

        AZStd::string GetCurrentProjectName() { return m_currentProjectName; }
        AZStd::vector<AzFramework::GemInfo> GetGemInfoList() { return m_gemInfoList; }

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // AzFramework::Application overrides
        void SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations) override;
        ////////////////////////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Get Generic Command Info
        CommandType GetCommandType(const AzFramework::CommandLine* parser, bool suppressErrors);
        bool ShouldPrintHelp(const AzFramework::CommandLine* parser);
        bool ShouldPrintVerbose(const AzFramework::CommandLine* parser);
        void InitArgValidationLists();
        ////////////////////////////////////////////////////////////////////////////////////////////


        ////////////////////////////////////////////////////////////////////////////////////////////
        // Store Detailed Command Info and Validate parser input (command correctness)
        AZ::Outcome<SeedsParams, AZStd::string> ParseSeedsCommandData(const AzFramework::CommandLine* parser);
        AZ::Outcome<AssetListsParams, AZStd::string> ParseAssetListsCommandData(const AzFramework::CommandLine* parser);
        AZ::Outcome<ComparisonRulesParams, AZStd::string> ParseComparisonRulesCommandData(const AzFramework::CommandLine* parser);
        AZ::Outcome<ComparisonParams, AZStd::string> ParseCompareCommandData(const AzFramework::CommandLine* parser);
        AZ::Outcome<BundleSettingsParams, AZStd::string> ParseBundleSettingsCommandData(const AzFramework::CommandLine* parser);
        AZ::Outcome<BundlesParamsList, AZStd::string> ParseBundlesCommandData(const AzFramework::CommandLine* parser);
        AZ::Outcome<BundleSeedParams, AZStd::string> ParseBundleSeedCommandData(const AzFramework::CommandLine* parser);

        AZ::Outcome<void, AZStd::string> ValidateInputArgs(const AzFramework::CommandLine* parser, const AZStd::vector<const char*>& validArgList);
        AZ::Outcome<AZStd::string, AZStd::string> GetFilePathArg(const AzFramework::CommandLine* parser, const char* argName, const char* subCommandName, bool isRequired = false);
        template <typename T>
        AZ::Outcome<AZStd::vector<T>, AZStd::string> GetArgsList(const AzFramework::CommandLine* parser, const char* argName, const char* subCommandName, bool isRequired = false);
        AZ::Outcome<AzFramework::PlatformFlags, AZStd::string> GetPlatformArg(const AzFramework::CommandLine* parser);
        AzFramework::PlatformFlags GetInputPlatformFlagsOrEnabledPlatformFlags(AzFramework::PlatformFlags inputPlatformFlags);
        AZStd::vector<AZStd::string> GetAddSeedArgList(const AzFramework::CommandLine* parser);
        AZStd::vector<AZStd::string> GetSkipArgList(const AzFramework::CommandLine* parser);
        ////////////////////////////////////////////////////////////////////////////////////////////


        ////////////////////////////////////////////////////////////////////////////////////////////
        // Run Commands and Validate param data (value correctness)
        bool RunSeedsCommands(const AZ::Outcome<SeedsParams, AZStd::string>& paramsOutcome);
        bool RunAssetListsCommands(const AZ::Outcome<AssetListsParams, AZStd::string>& paramsOutcome);
        bool RunComparisonRulesCommands(const AZ::Outcome<ComparisonRulesParams, AZStd::string>& paramsOutcome);
        bool RunCompareCommand(const AZ::Outcome<ComparisonParams, AZStd::string>& paramsOutcome);
        bool RunBundleSettingsCommands(const AZ::Outcome<BundleSettingsParams, AZStd::string>& paramsOutcome);
        bool RunBundlesCommands(const AZ::Outcome<BundlesParamsList, AZStd::string>& paramsOutcome);
        bool RunBundleSeedCommands(const AZ::Outcome<BundleSeedParams, AZStd::string>& paramsOutcome);
        ////////////////////////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Helpers
        AZ::Outcome<void, AZStd::string> InitAssetCatalog(AzFramework::PlatformFlags platforms, const AZStd::string& assetCatalogFile = AZStd::string());
        //! Given a gem seed file, validates whether the seed file is valid for the current project 
        //! and platform flags specified before loading the file from disk.
        //! Does not do any validation on non gem seed files.
        AZ::Outcome<void, AZStd::string> LoadSeedListFile(const AZStd::string& seedListFileAbsolutePath, AzFramework::PlatformFlags platformFlags);
        AZ::Outcome<void, AZStd::string> LoadProjectDependenciesFile(AzFramework::PlatformFlags platformFlags);
        void PrintSeedList(const AZStd::string& seedListFileAbsolutePath);
        bool RunPlatformSpecificAssetListCommands(const AssetListsParams& params, AzFramework::PlatformFlags platformFlags);
        void PrintAssetLists(const AssetListsParams& params,
            const AZStd::fixed_vector<AzFramework::PlatformId, AzFramework::PlatformId::NumPlatformIds>& platformIds,
            bool printExistingFiles,
            const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList,
            const AZStd::vector<AZStd::string>& wildcardPatternExclusionList);
        AZStd::vector<FilePath> GetAllPlatformSpecificFilesOnDisk(const FilePath& platformIndependentFilePath, AzFramework::PlatformFlags platformFlags = AzFramework::PlatformFlags::Platform_NONE);
        AZ::Outcome<void, AZStd::string> ApplyBundleSettingsOverrides(
            AzToolsFramework::AssetBundleSettings& bundleSettings, 
            const AZStd::string& assetListFilePath, 
            const AZStd::string& outputBundleFilePath, 
            int bundleVersion, 
            int maxBundleSize);
        AZ::Outcome<void, AZStd::string> ParseComparisonTypesAndPatterns(const AzFramework::CommandLine* parser, ComparisonRulesParams& params);
        AZ::Outcome<void, AZStd::string> ParseComparisonTypesAndPatternsForEditCommand(const AzFramework::CommandLine* parser, ComparisonRulesParams& params);
        AZ::Outcome<void, AZStd::string> ParseComparisonRulesFirstAndSecondInputArgs(const AzFramework::CommandLine* parser, ComparisonRulesParams& params);
        AZ::Outcome<BundlesParamsList, AZStd::string> ParseBundleSettingsAndOverrides(const AzFramework::CommandLine* parser, const char* commandName);
        bool ConvertRulesParamsToComparisonData(const ComparisonRulesParams& params, AzToolsFramework::AssetFileInfoListComparison& assetListComparison, size_t startingIndex);
        bool EditComparisonData(const ComparisonRulesParams& params, AzToolsFramework::AssetFileInfoListComparison& assetListComparison, size_t index);
        void PrintComparisonRules(const AzToolsFramework::AssetFileInfoListComparison& assetListComparison, const AZStd::string& comparisonRulesAbsoluteFilePath);
        bool IsDefaultToken(const AZStd::string& pathOrToken);
        void PrintComparisonAssetList(const AzToolsFramework::AssetFileInfoList& infoList, const AZStd::string& resultName);
        void AddPlatformToAllComparisonParams(ComparisonParams& params, const AZStd::string& platformName);
        void AddPlatformToComparisonParam(AZStd::string& inOut, const AZStd::string& platformName);
        //! Error message to display when neither of two optional arguments was found
        static AZStd::string GetBinaryArgOptionFailure(const char* arg1, const char* arg2);

        bool SeedsOperationRequiresCatalog(const SeedsParams& params);
        ////////////////////////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Output Help Text
        void OutputHelp(CommandType commandType);
        void OutputHelpSeeds();
        void OutputHelpAssetLists();
        void OutputHelpComparisonRules();
        void OutputHelpCompare();
        void OutputHelpBundleSettings();
        void OutputHelpBundles();
        void OutputHelpBundleSeed();
        ////////////////////////////////////////////////////////////////////////////////////////////


        AZStd::unique_ptr<AzToolsFramework::AssetSeedManager> m_assetSeedManager;
        AZStd::unique_ptr<AzToolsFramework::PlatformAddressedAssetCatalogManager> m_platformCatalogManager;
        AZStd::vector<AzFramework::GemInfo> m_gemInfoList;
        bool m_showVerboseOutput = false;
        AZStd::string m_currentProjectName;

        CommandType m_commandType = CommandType::Invalid;

        AZStd::vector<const char*> m_allSeedsArgs;
        AZStd::vector<const char*> m_allAssetListsArgs;
        AZStd::vector<const char*> m_allComparisonRulesArgs;
        AZStd::vector<const char*> m_allCompareArgs;
        AZStd::vector<const char*> m_allBundleSettingsArgs;
        AZStd::vector<const char*> m_allBundlesArgs;
        AZStd::vector<const char*> m_allBundleSeedArgs;
    };
}
