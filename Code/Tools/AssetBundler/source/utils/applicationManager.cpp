/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/utils/applicationManager.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Jobs/Algorithms.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/Slice/SliceSystemComponent.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzFramework/FileTag/FileTagComponent.h>
#include <AzFramework/Input/System/InputSystemComponent.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzFramework/Components/AzFrameworkConfigurationSystemComponent.h>

#include <AzToolsFramework/Archive/ArchiveComponent.h>
#include <AzToolsFramework/Asset/AssetDebugInfo.h>
#include <AzToolsFramework/Asset/AssetUtils.h>
#include <AzToolsFramework/AssetBundle/AssetBundleComponent.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalogBus.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>

namespace AssetBundler
{
    const char compareVariablePrefix = '$';

    ApplicationManager::ApplicationManager(int* argc, char*** argv, QObject* parent)
        : QObject(parent)
        , AzToolsFramework::ToolsApplication(argc, argv)
    {
    }

    ApplicationManager::~ApplicationManager()
    {
        DestroyApplication();
    }

    bool ApplicationManager::Init()
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();

        ComponentApplication::StartupParameters startupParameters;
        // The AssetBundler does not need to load gems
        startupParameters.m_loadDynamicModules = false;
        Start(AzFramework::Application::Descriptor(), startupParameters);

        AZ::SerializeContext* context;
        EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(context, "No serialize context");
        AzToolsFramework::AssetSeedManager::Reflect(context);
        AzToolsFramework::AssetFileInfoListComparison::Reflect(context);
        AzToolsFramework::AssetBundleSettings::Reflect(context);

        [[maybe_unused]] AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO != nullptr, "AZ::IO::FileIOBase must be ready for use.\n");

        m_assetSeedManager = AZStd::make_unique<AzToolsFramework::AssetSeedManager>();
        AZ_TracePrintf(AssetBundler::AppWindowName, "\n");

        // There is no need to update the UserSettings file, so we can avoid a race condition by disabling save on shutdown
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        return true;
    }

    void ApplicationManager::DestroyApplication()
    {
        m_showVerboseOutput = false;
        m_assetSeedManager.reset();
        Stop();
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    bool ApplicationManager::Run()
    {
        const AZ::CommandLine* parser = GetCommandLine();

        bool shouldPrintHelp = ShouldPrintHelp(parser);

        // Check for what command we are running, and if the user wants to see the Help text
        m_commandType = GetCommandType(parser, shouldPrintHelp);

        if (shouldPrintHelp)
        {
            // If someone requested the help text, it doesn't matter if their command is invalid
            OutputHelp(m_commandType);
            return true;
        }

        if (m_commandType == CommandType::Invalid)
        {
            OutputHelp(m_commandType);
            return false;
        }

        if (parser->HasSwitch(ProjectArg))
        {
            if (parser->GetNumSwitchValues(ProjectArg) != 1)
            {
                AZ_Error(AppWindowName, false, "Invalid command : \"--%s\" must have exactly one value.", ProjectArg);
                return false;
            }
            m_currentProjectName = parser->GetSwitchValue(ProjectArg, 0);
            AZ_TracePrintf(AssetBundler::AppWindowName, "Setting project to ( %s ).\n", m_currentProjectName.c_str());
        }
        m_showVerboseOutput = ShouldPrintVerbose(parser);

        m_currentProjectName = AZStd::string_view{ AZ::Utils::GetProjectName() };

        if (m_currentProjectName.empty())
        {
            AZ_Error(AppWindowName, false, "Unable to retrieve project name from the Settings Registry");
            return false;
        }

        // Gems
        if (!AzFramework::GetGemsInfo(m_gemInfoList, *m_settingsRegistry))
        {
            AZ_Error(AppWindowName, false, "Failed to read Gems for project: %s\n", m_currentProjectName.c_str());
            return false;
        }

        m_platformCatalogManager = AZStd::make_unique<AzToolsFramework::PlatformAddressedAssetCatalogManager>();

        InitArgValidationLists();

        switch (m_commandType)
        {
        case CommandType::Seeds:
            return RunSeedsCommands(ParseSeedsCommandData(parser));
        case CommandType::AssetLists:
            return RunAssetListsCommands(ParseAssetListsCommandData(parser));
        case CommandType::ComparisonRules:
            return RunComparisonRulesCommands(ParseComparisonRulesCommandData(parser));
        case CommandType::Compare:
            return RunCompareCommand(ParseCompareCommandData(parser));
        case CommandType::BundleSettings:
            return RunBundleSettingsCommands(ParseBundleSettingsCommandData(parser));
        case CommandType::Bundles:
            return RunBundlesCommands(ParseBundlesCommandData(parser));
        case CommandType::BundleSeed:
            return RunBundleSeedCommands(ParseBundleSeedCommandData(parser));
        }

        return false;
    }

    AZ::ComponentTypeList ApplicationManager::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = AzFramework::Application::GetRequiredSystemComponents();

        components.emplace_back(azrtti_typeid<AzToolsFramework::AssetBundleComponent>());
        components.emplace_back(azrtti_typeid<AzToolsFramework::ArchiveComponent>());
        components.emplace_back(azrtti_typeid<AzToolsFramework::Prefab::PrefabSystemComponent>());

        for (auto iter = components.begin(); iter != components.end();)
        {
            if (*iter == azrtti_typeid<AzFramework::GameEntityContextComponent>() ||
                *iter == azrtti_typeid<AzFramework::AzFrameworkConfigurationSystemComponent>() ||
                *iter == azrtti_typeid<AzFramework::InputSystemComponent>() ||
                *iter == azrtti_typeid<AZ::SliceSystemComponent>())
            {
                // Asset Bundler does not require the above components to be active
                iter = components.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        return components;
    }

    void ApplicationManager::SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations)
    {
        AzToolsFramework::ToolsApplication::SetSettingsRegistrySpecializations(specializations);
        specializations.Append("assetbundler");
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Get Generic Command Info
    ////////////////////////////////////////////////////////////////////////////////////////////

    CommandType ApplicationManager::GetCommandType(const AZ::CommandLine* parser, [[maybe_unused]] bool suppressErrors)
    {
        // Verify that the user has only typed in one sub-command
        size_t numMiscValues = parser->GetNumMiscValues();
        if (numMiscValues == 0)
        {
            AZ_Error(AppWindowName, suppressErrors, "Invalid command: Must provide a sub-command (ex: \"%s\").", AssetBundler::SeedsCommand);
            return CommandType::Invalid;
        }
        else if (numMiscValues > 1)
        {
            AZ_Error(AppWindowName, suppressErrors, "Invalid command: Cannot perform more than one sub-command operation at once");
            return CommandType::Invalid;
        }

        AZStd::string subCommand = parser->GetMiscValue(0);
        if (!azstricmp(subCommand.c_str(),AssetBundler::SeedsCommand))
        {
            return CommandType::Seeds;
        }
        else if (!azstricmp(subCommand.c_str(), AssetBundler::AssetListsCommand))
        {
            return CommandType::AssetLists;
        }
        else if (!azstricmp(subCommand.c_str(), AssetBundler::ComparisonRulesCommand))
        {
            return CommandType::ComparisonRules;
        }
        else if (!azstricmp(subCommand.c_str(), AssetBundler::CompareCommand))
        {
            return CommandType::Compare;
        }
        else if (!azstricmp(subCommand.c_str(), AssetBundler::BundleSettingsCommand))
        {
            return CommandType::BundleSettings;
        }
        else if (!azstricmp(subCommand.c_str(), AssetBundler::BundlesCommand))
        {
            return CommandType::Bundles;
        }
        else if (!azstricmp(subCommand.c_str(), AssetBundler::BundleSeedCommand))
        {
            return CommandType::BundleSeed;
        }
        else
        {
            AZ_Error(AppWindowName, false, "( %s ) is not a valid sub-command", subCommand.c_str());
            return CommandType::Invalid;
        }
    }

    bool ApplicationManager::ShouldPrintHelp(const AZ::CommandLine* parser)
    {
        return parser->HasSwitch(AssetBundler::HelpFlag) || parser->HasSwitch(AssetBundler::HelpFlagAlias);
    }

    bool ApplicationManager::ShouldPrintVerbose(const AZ::CommandLine* parser)
    {
        return parser->HasSwitch(AssetBundler::VerboseFlag);
    }

    void ApplicationManager::InitArgValidationLists()
    {
        m_allSeedsArgs = {
            SeedListFileArg,
            AddSeedArg,
            RemoveSeedArg,
            AddPlatformToAllSeedsFlag,
            RemovePlatformFromAllSeedsFlag,
            UpdateSeedPathArg,
            RemoveSeedPathArg,
            PrintFlag,
            PlatformArg,
            AssetCatalogFileArg,
            VerboseFlag,
            ProjectArg,
            IgnoreFileCaseFlag
        };

        m_allAssetListsArgs = {
            AssetListFileArg,
            SeedListFileArg,
            AddSeedArg,
            AddDefaultSeedListFilesFlag,
            PlatformArg,
            AssetCatalogFileArg,
            PrintFlag,
            DryRunFlag,
            GenerateDebugFileFlag,
            AllowOverwritesFlag,
            VerboseFlag,
            SkipArg,
            ProjectArg
        };

        m_allComparisonRulesArgs = {
            ComparisonRulesFileArg,
            ComparisonTypeArg,
            ComparisonFilePatternArg,
            ComparisonFilePatternTypeArg,
            ComparisonTokenNameArg,
            ComparisonFirstInputArg,
            ComparisonSecondInputArg,
            AddComparisonStepArg,
            RemoveComparisonStepArg,
            MoveComparisonStepArg,
            EditComparisonStepArg,
            PrintFlag,
            VerboseFlag,
            ProjectArg
        };

        m_allCompareArgs =
        {
            ComparisonRulesFileArg,
            ComparisonTypeArg,
            ComparisonFilePatternArg,
            ComparisonFilePatternTypeArg,
            CompareFirstFileArg,
            CompareSecondFileArg,
            CompareOutputFileArg,
            ComparePrintArg,
            IntersectionCountArg,
            AllowOverwritesFlag,
            PlatformArg,
            VerboseFlag,
            ProjectArg
        };

        m_allBundleSettingsArgs = {
            BundleSettingsFileArg,
            AssetListFileArg,
            OutputBundlePathArg,
            BundleVersionArg,
            MaxBundleSizeArg,
            PlatformArg,
            PrintFlag,
            VerboseFlag,
            ProjectArg
        };

        m_allBundlesArgs = {
            BundleSettingsFileArg,
            AssetListFileArg,
            OutputBundlePathArg,
            BundleVersionArg,
            MaxBundleSizeArg,
            PlatformArg,
            AllowOverwritesFlag,
            VerboseFlag,
            ProjectArg
        };

        m_allBundleSeedArgs = {
            BundleSettingsFileArg,
            AddSeedArg,
            OutputBundlePathArg,
            BundleVersionArg,
            MaxBundleSizeArg,
            PlatformArg,
            AssetCatalogFileArg,
            AllowOverwritesFlag,
            VerboseFlag,
            ProjectArg
        };
    }


    ////////////////////////////////////////////////////////////////////////////////////////////
    // Store Detailed Command Info
    ////////////////////////////////////////////////////////////////////////////////////////////

    AZ::Outcome<SeedsParams, AZStd::string> ApplicationManager::ParseSeedsCommandData(const AZ::CommandLine* parser)
    {
        using namespace AzToolsFramework;

        auto validateArgsOutcome = ValidateInputArgs(parser, m_allSeedsArgs);
        if (!validateArgsOutcome.IsSuccess())
        {
            OutputHelpSeeds();
            return AZ::Failure(validateArgsOutcome.TakeError());
        }

        SeedsParams params;

        params.m_ignoreFileCase = parser->HasSwitch(IgnoreFileCaseFlag);

        // Read in Seed List Files arg
        auto requiredArgOutcome = GetFilePathArg(parser, SeedListFileArg, SeedsCommand, true);
        if (!requiredArgOutcome.IsSuccess())
        {
            return AZ::Failure(requiredArgOutcome.GetError());
        }
        bool checkFileCase = true;
        // Seed List files do not have platform-specific file names
        params.m_seedListFile = FilePath(requiredArgOutcome.GetValue(), checkFileCase, params.m_ignoreFileCase);

        if (!params.m_seedListFile.IsValid())
        {
            return AZ::Failure(params.m_seedListFile.ErrorString());
        }

        // Read in Add/Remove Platform to All Seeds flag
        params.m_addPlatformToAllSeeds = parser->HasSwitch(AddPlatformToAllSeedsFlag);
        params.m_removePlatformFromAllSeeds = parser->HasSwitch(RemovePlatformFromAllSeedsFlag);

        if (params.m_addPlatformToAllSeeds && params.m_removePlatformFromAllSeeds)
        {
            return AZ::Failure(AZStd::string::format("Invalid command: Unable to run \"--%s\" and \"--%s\" at the same time.", AssetBundler::AddPlatformToAllSeedsFlag, AssetBundler::RemovePlatformFromAllSeedsFlag));
        }

        if ((params.m_addPlatformToAllSeeds || params.m_removePlatformFromAllSeeds) && !parser->HasSwitch(PlatformArg))
        {
            return AZ::Failure(AZStd::string::format("Invalid command: When running \"--%s\" or \"--%s\", the \"--%s\" arg is required.", AddPlatformToAllSeedsFlag, RemovePlatformFromAllSeedsFlag, PlatformArg));
        }

        // Read in Platform arg
        auto platformOutcome = GetPlatformArg(parser);
        if (!platformOutcome.IsSuccess())
        {
            return AZ::Failure(platformOutcome.GetError());
        }
        params.m_platformFlags = GetInputPlatformFlagsOrEnabledPlatformFlags(platformOutcome.GetValue());

        // Read in Asset Catalog File arg
        auto argOutcome = GetFilePathArg(parser, AssetCatalogFileArg, SeedsCommand);
        if (!argOutcome.IsSuccess())
        {
            return AZ::Failure(argOutcome.GetError());
        }
        if (!argOutcome.IsSuccess())
        {
            params.m_assetCatalogFile = FilePath(argOutcome.GetValue(), checkFileCase, params.m_ignoreFileCase);
            if (!params.m_assetCatalogFile.IsValid())
            {
                return AZ::Failure(params.m_assetCatalogFile.ErrorString());
            }
        }

        // Read in Add Seed arg
        params.m_addSeedList = GetAddSeedArgList(parser);

        // Read in Remove Seed arg
        size_t numRemoveSeedArgs = 0;
        if (parser->HasSwitch(RemoveSeedArg))
        {
            numRemoveSeedArgs = parser->GetNumSwitchValues(RemoveSeedArg);
            for (size_t removeSeedIndex = 0; removeSeedIndex < numRemoveSeedArgs; ++removeSeedIndex)
            {
                params.m_removeSeedList.push_back(parser->GetSwitchValue(RemoveSeedArg, removeSeedIndex));
            }
        }

        // Read Update Seed Path arg
        params.m_updateSeedPathHint = parser->HasSwitch(UpdateSeedPathArg);

        // Read Update Seed Path arg
        params.m_removeSeedPathHint = parser->HasSwitch(RemoveSeedPathArg);

        // Read in Print flag
        params.m_print = parser->HasSwitch(PrintFlag);

        return AZ::Success(params);
    }

    AZStd::string ApplicationManager::GetBinaryArgOptionFailure(const char* arg1, const char* arg2)
    {
        const char FailureMessage[] = "Missing argument: Either %s or %s must be supplied";
        return AZStd::string::format(FailureMessage, arg1, arg2);
    }

    AZ::Outcome<AssetListsParams, AZStd::string> ApplicationManager::ParseAssetListsCommandData(const AZ::CommandLine* parser)
    {
        auto validateArgsOutcome = ValidateInputArgs(parser, m_allAssetListsArgs);
        if (!validateArgsOutcome.IsSuccess())
        {
            OutputHelpAssetLists();
            return AZ::Failure(validateArgsOutcome.TakeError());
        }

        AssetListsParams params;

        // Read in Platform arg
        auto platformOutcome = GetPlatformArg(parser);
        if (!platformOutcome.IsSuccess())
        {
            return AZ::Failure(platformOutcome.GetError());
        }
        params.m_platformFlags = GetInputPlatformFlagsOrEnabledPlatformFlags(platformOutcome.GetValue());

        // Read in Print flag
        params.m_print = parser->HasSwitch(PrintFlag);

        // Read in Asset List File arg
        auto requiredArgOutcome = GetFilePathArg(parser, AssetListFileArg, AssetListsCommand, false);
        params.m_assetListFile = FilePath(requiredArgOutcome.GetValue());
       
        if (!params.m_print && !params.m_assetListFile.IsValid())
        {
            return AZ::Failure(GetBinaryArgOptionFailure(PrintFlag,AssetListFileArg));
        }

        // Read in Seed List File arg
        size_t numSeedListFiles = parser->GetNumSwitchValues(SeedListFileArg);
        for (size_t seedListFileIndex = 0; seedListFileIndex < numSeedListFiles; ++seedListFileIndex)
        {
            params.m_seedListFiles.emplace_back(FilePath(parser->GetSwitchValue(SeedListFileArg, seedListFileIndex)));
        }

        // Read in Add Seed arg
        params.m_addSeedList = GetAddSeedArgList(parser);

        // Read in Skip arg
        params.m_skipList = GetSkipArgList(parser);

        // Read in Add Default Seed List Files arg
        params.m_addDefaultSeedListFiles = parser->HasSwitch(AddDefaultSeedListFilesFlag);

        // Read in Asset Catalog File arg
        auto argOutcome = GetFilePathArg(parser, AssetCatalogFileArg, AssetListsCommand);
        if (!argOutcome.IsSuccess())
        {
            return AZ::Failure(argOutcome.GetError());
        }
        if (!argOutcome.IsSuccess())
        {
            params.m_assetCatalogFile = FilePath(argOutcome.GetValue());
        }

        // Read in Dry Run flag
        params.m_dryRun = parser->HasSwitch(DryRunFlag);

        // Read in Generate Debug File flag
        params.m_generateDebugFile = parser->HasSwitch(GenerateDebugFileFlag);

        // Read in Allow Overwrites flag
        params.m_allowOverwrites = parser->HasSwitch(AllowOverwritesFlag);

        return AZ::Success(params);
    }

    AZ::Outcome<ComparisonRulesParams, AZStd::string> ApplicationManager::ParseComparisonRulesCommandData(const AZ::CommandLine* parser)
    {
        auto validateArgsOutcome = ValidateInputArgs(parser, m_allComparisonRulesArgs);
        if (!validateArgsOutcome.IsSuccess())
        {
            OutputHelpComparisonRules();
            return AZ::Failure(validateArgsOutcome.TakeError());
        }

        ScopedTraceHandler traceHandler;
        ComparisonRulesParams params;

        auto requiredArgOutcome = GetFilePathArg(parser, ComparisonRulesFileArg, ComparisonRulesCommand, true);
        if (!requiredArgOutcome.IsSuccess())
        {
            return AZ::Failure(requiredArgOutcome.GetError());
        }
        params.m_comparisonRulesFile = FilePath(requiredArgOutcome.GetValue());

        if (params.m_comparisonRulesFile.AbsolutePath().empty())
        {
            return AZ::Failure(AZStd::string::format("Invalid command: \"--%s\" cannot be empty.", ComparisonRulesFileArg));
        }

        // Read in Add Comparison Step arg
        if (parser->HasSwitch(AddComparisonStepArg))
        {
            size_t numInputs = parser->GetNumSwitchValues(AddComparisonStepArg);
            switch (numInputs)
            {
            case 0:
                params.m_comparisonRulesStepAction = ComparisonRulesStepAction::AddToEnd;
                break;
            case 1:
                params.m_comparisonRulesStepAction = ComparisonRulesStepAction::Add;
                params.m_destinationLine = static_cast<size_t>(AZ::StringFunc::ToInt(parser->GetSwitchValue(AddComparisonStepArg, 0).c_str()));
                break;
            default:
                return AZ::Failure(AZStd::string::format("Invalid command: \"--%s\" cannot have more than one input value.", AddComparisonStepArg));
            }

            // Read in what the user wants to add
            auto parseComparisonTypesOutcome = ParseComparisonTypesAndPatterns(parser, params);
            if (!parseComparisonTypesOutcome.IsSuccess())
            {
                return AZ::Failure(parseComparisonTypesOutcome.GetError());
            }
        }

        // Read in Remove Comparison Step arg
        if (parser->HasSwitch(RemoveComparisonStepArg))
        {
            if (params.m_comparisonRulesStepAction != ComparisonRulesStepAction::Default)
            {
                return AZ::Failure(AZStd::string::format(
                    "Invalid command: Only one of the following args may be used in a single command: \"--%s\", \"--%s\", \"--%s\", \"--%s\".",
                    AddComparisonStepArg, RemoveComparisonStepArg, MoveComparisonStepArg, EditComparisonStepArg));
            }

            if (parser->GetNumSwitchValues(RemoveComparisonStepArg) != 1)
            {
                return AZ::Failure(AZStd::string::format(
                    "Invalid command: \"--%s\" requires exatly one input value (the line number you wish to remove).",
                    RemoveComparisonStepArg));
            }

            params.m_comparisonRulesStepAction = ComparisonRulesStepAction::Remove;
            params.m_initialLine = static_cast<size_t>(AZ::StringFunc::ToInt(parser->GetSwitchValue(RemoveComparisonStepArg, 0).c_str()));
        }

        // Read in Move Comparison Step arg
        if (parser->HasSwitch(MoveComparisonStepArg))
        {
            if (params.m_comparisonRulesStepAction != ComparisonRulesStepAction::Default)
            {
                return AZ::Failure(AZStd::string::format(
                    "Invalid command: Only one of the following args may be used in a single command: \"--%s\", \"--%s\", \"--%s\", \"--%s\".",
                    AddComparisonStepArg, RemoveComparisonStepArg, MoveComparisonStepArg, EditComparisonStepArg));
            }

            if (parser->GetNumSwitchValues(MoveComparisonStepArg) != 2)
            {
                return AZ::Failure(AZStd::string::format(
                    "Invalid command: \"--%s\" requires exatly two input values (the line number of the Comparison Step you wish to move, and the destination line)",
                    MoveComparisonStepArg));
            }

            params.m_comparisonRulesStepAction = ComparisonRulesStepAction::Move;
            params.m_initialLine = static_cast<size_t>(AZ::StringFunc::ToInt(parser->GetSwitchValue(MoveComparisonStepArg, 0).c_str()));
            params.m_destinationLine = static_cast<size_t>(AZ::StringFunc::ToInt(parser->GetSwitchValue(MoveComparisonStepArg, 1).c_str()));
        }

        // Read in Edit Comparison Step arg
        if (parser->HasSwitch(EditComparisonStepArg))
        {
            if (params.m_comparisonRulesStepAction != ComparisonRulesStepAction::Default)
            {
                return AZ::Failure(AZStd::string::format(
                    "Invalid command: Only one of the following args may be used in a single command: \"--%s\", \"--%s\", \"--%s\", \"--%s\".",
                    AddComparisonStepArg, RemoveComparisonStepArg, MoveComparisonStepArg, EditComparisonStepArg));
            }

            if (parser->GetNumSwitchValues(EditComparisonStepArg) != 1)
            {
                return AZ::Failure(AZStd::string::format(
                    "Invalid command: \"--%s\" requires exactly one input value (the line number of the Comparison Step you wish to edit)",
                    EditComparisonStepArg));
            }

            params.m_comparisonRulesStepAction = ComparisonRulesStepAction::Edit;
            params.m_initialLine = static_cast<size_t>(AZ::StringFunc::ToInt(parser->GetSwitchValue(EditComparisonStepArg, 0).c_str()));

            // When editing a Comparison Step, we can only accept one input for every value type
            auto parseComparisonTypesForEditOutcome = ParseComparisonTypesAndPatternsForEditCommand(parser, params);
            if (!parseComparisonTypesForEditOutcome.IsSuccess())
            {
                return AZ::Failure(parseComparisonTypesForEditOutcome.GetError());
            }
        }

        auto parseFirstAndSecondInputsOutcome = ParseComparisonRulesFirstAndSecondInputArgs(parser, params);
        if (!parseFirstAndSecondInputsOutcome.IsSuccess())
        {
            return AZ::Failure(parseFirstAndSecondInputsOutcome.GetError());
        }

        if (parser->HasSwitch(ComparisonTypeArg) &&
            !parser->HasSwitch(AddComparisonStepArg) &&
            !parser->HasSwitch(EditComparisonStepArg))
        {
            return AZ::Failure(AZStd::string::format(
                "Invalid command: \"--%s\" cannot be used without one of the following operations: \"--%s\", \"--%s\".",
                ComparisonTypeArg, AddComparisonStepArg, EditComparisonStepArg));
        }

        for (const auto& comparisonType : params.m_comparisonTypeList)
        {
            if (comparisonType == AzToolsFramework::AssetFileInfoListComparison::ComparisonType::IntersectionCount)
            {
                return AZ::Failure(AZStd::string::format("Adding compare operation ( %s ) to comparison rule file is not supported currently.", AzToolsFramework::AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(AzToolsFramework::AssetFileInfoListComparison::ComparisonType::IntersectionCount)]));
            }
        }

        // Read in Print flag
        params.m_print = parser->HasSwitch(PrintFlag);

        return AZ::Success(params);
    }

    AZ::Outcome<void, AZStd::string> ApplicationManager::ParseComparisonTypesAndPatterns(const AZ::CommandLine* parser, ComparisonRulesParams& params)
    {
        int filePatternsConsumed = 0;
        size_t numComparisonTypes = parser->GetNumSwitchValues(ComparisonTypeArg);
        size_t numFilePatterns = parser->GetNumSwitchValues(ComparisonFilePatternArg);
        size_t numPatternTypes = parser->GetNumSwitchValues(ComparisonFilePatternTypeArg);

        size_t numIntersectionCount = parser->GetNumSwitchValues(IntersectionCountArg);

        if (numIntersectionCount > 1)
        {
            return AZ::Failure(AZStd::string::format("Invalid command: \"--%s\" must have exactly one value.", IntersectionCountArg));
        }

        params.m_intersectionCount = AZStd::stoi(parser->GetSwitchValue(IntersectionCountArg, 0));

        size_t numTokenNames = parser->GetNumSwitchValues(ComparisonTokenNameArg);

        if (numTokenNames > 0 && numComparisonTypes != numTokenNames)
        {
            return AZ::Failure(AZStd::string::format(
                "Number of comparisonTypes ( %zu ) and tokenNames ( %zu ) must match. Token values can always be edited later using the \"--%s\" and \"--%s\" args.",
                numComparisonTypes, numTokenNames, EditComparisonStepArg, ComparisonTokenNameArg));
        }

        if (numPatternTypes != numFilePatterns)
        {
            return AZ::Failure(AZStd::string::format("Number of filePatternTypes ( %zu ) and filePatterns ( %zu ) must match.", numPatternTypes, numFilePatterns));
        }

        for (size_t comparisonTypeIndex = 0; comparisonTypeIndex < numComparisonTypes; ++comparisonTypeIndex)
        {
            auto comparisonTypeOutcome = ParseComparisonType(parser->GetSwitchValue(ComparisonTypeArg, comparisonTypeIndex));
            if (!comparisonTypeOutcome.IsSuccess())
            {
                return AZ::Failure(comparisonTypeOutcome.GetError());
            }

            auto comparisonType = comparisonTypeOutcome.GetValue();
            if (comparisonType == AzToolsFramework::AssetFileInfoListComparison::ComparisonType::FilePattern)
            {
                if (filePatternsConsumed >= numFilePatterns)
                {
                    return AZ::Failure(AZStd::string::format("Number of file patterns comparisons exceeded number of file patterns provided ( %zu ).", numFilePatterns));
                }

                params.m_filePatternList.emplace_back(parser->GetSwitchValue(ComparisonFilePatternArg, filePatternsConsumed));

                auto filePatternTypeOutcome = ParseFilePatternType(parser->GetSwitchValue(ComparisonFilePatternTypeArg, filePatternsConsumed));
                if (!filePatternTypeOutcome.IsSuccess())
                {
                    return AZ::Failure(filePatternTypeOutcome.GetError());
                }
                params.m_filePatternTypeList.emplace_back(filePatternTypeOutcome.GetValue());
                filePatternsConsumed++;
            }
            else
            {
                params.m_filePatternList.emplace_back("");
                params.m_filePatternTypeList.emplace_back(AzToolsFramework::AssetFileInfoListComparison::FilePatternType::Default);
            }

            if (numTokenNames > 0)
            {
                AZStd::string tokenName = parser->GetSwitchValue(ComparisonTokenNameArg, comparisonTypeIndex);
                AzToolsFramework::AssetFileInfoListComparison::FormatOutputToken(tokenName);
                params.m_tokenNamesList.emplace_back(tokenName);
            }
            else
            {
                params.m_tokenNamesList.emplace_back("");
            }

            params.m_comparisonTypeList.emplace_back(comparisonType);
        }

        if (filePatternsConsumed != numFilePatterns)
        {
            return AZ::Failure(AZStd::string::format("Number of provided file patterns exceeded the number of file pattern comparisons ( %zu ).", numFilePatterns));
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> ApplicationManager::ParseComparisonTypesAndPatternsForEditCommand(const AZ::CommandLine* parser, ComparisonRulesParams& params)
    {
        if (parser->HasSwitch(ComparisonTypeArg))
        {
            size_t numComparisonTypes = parser->GetNumSwitchValues(ComparisonTypeArg);
            if (numComparisonTypes > 1)
            {
                return AZ::Failure(AZStd::string::format(
                    "Invalid command: when using the \"--%s\" arg, the \"--%s\" arg can accept no more than one input value.",
                    EditComparisonStepArg, ComparisonTypeArg));
            }

            auto comparisonTypeOutcome = ParseComparisonType(parser->GetSwitchValue(ComparisonTypeArg, 0));
            if (!comparisonTypeOutcome.IsSuccess())
            {
                return AZ::Failure(comparisonTypeOutcome.GetError());
            }
            params.m_comparisonTypeList.emplace_back(comparisonTypeOutcome.GetValue());
        }

        if (parser->HasSwitch(ComparisonFilePatternTypeArg))
        {
            size_t numPatternTypes = parser->GetNumSwitchValues(ComparisonFilePatternTypeArg);
            if (numPatternTypes > 1)
            {
                return AZ::Failure(AZStd::string::format(
                    "Invalid command: when using the \"--%s\" arg, the \"--%s\" arg can accept no more than one input value.",
                    EditComparisonStepArg, ComparisonFilePatternTypeArg));
            }

            auto filePatternTypeOutcome = ParseFilePatternType(parser->GetSwitchValue(ComparisonFilePatternTypeArg, 0));
            if (!filePatternTypeOutcome.IsSuccess())
            {
                return AZ::Failure(filePatternTypeOutcome.GetError());
            }
            params.m_filePatternTypeList.emplace_back(filePatternTypeOutcome.GetValue());
        }

        if (parser->HasSwitch(ComparisonFilePatternArg))
        {
            size_t numFilePatterns = parser->GetNumSwitchValues(ComparisonFilePatternArg);

            switch (numFilePatterns)
            {
            case 0:
                // Our CLI parser will not return empty strings, so we need an extra case to check if a user wants to remove a FilePattern
                params.m_filePatternList.emplace_back("");
                break;
            case 1:
                params.m_filePatternList.emplace_back(parser->GetSwitchValue(ComparisonFilePatternArg, 0));
                break;
            default:
                return AZ::Failure(AZStd::string::format(
                    "Invalid command: when using the \"--%s\" arg, the \"--%s\" arg can accept no more than one input value.",
                    EditComparisonStepArg, ComparisonFilePatternArg));
                break;
            }
        }

        if (parser->HasSwitch(ComparisonTokenNameArg))
        {
            AZStd::string tokenName;
            size_t numTokenNames = parser->GetNumSwitchValues(ComparisonTokenNameArg);
            switch (numTokenNames)
            {
            case 0:
                // Our CLI parser will not return empty strings, so we need an extra case to check if a user wants to remove a Token altogether
                params.m_tokenNamesList.emplace_back("");
                break;
            case 1:
                tokenName = parser->GetSwitchValue(ComparisonTokenNameArg, 0);
                AzToolsFramework::AssetFileInfoListComparison::FormatOutputToken(tokenName);
                params.m_tokenNamesList.emplace_back(tokenName);
                break;
            default:
                return AZ::Failure(AZStd::string::format(
                    "Invalid command: when using the \"--%s\" arg, the \"--%s\" arg can accept no more than one input value.",
                    EditComparisonStepArg, ComparisonTokenNameArg));
                break;
            }
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> ApplicationManager::ParseComparisonRulesFirstAndSecondInputArgs(const AZ::CommandLine* parser, ComparisonRulesParams& params)
    {
        if (params.m_comparisonTypeList.size() > 1 && (parser->HasSwitch(ComparisonFirstInputArg) || parser->HasSwitch(ComparisonSecondInputArg)))
        {
            return AZ::Failure(AZStd::string::format(
                "Invalid command: the \"--%s\" and \"--%s\" args can only operate on one Comparison Step at a time.",
                ComparisonFirstInputArg, ComparisonSecondInputArg));
        }

        size_t numInputs;
        AZStd::string inputStr;

        if (parser->HasSwitch(ComparisonFirstInputArg))
        {
            numInputs = parser->GetNumSwitchValues(ComparisonFirstInputArg);
            switch (numInputs)
            {
            case 0:
                // Our CLI parser will not return empty strings, so we need an extra case to check if a user wants to remove an input altogether
                params.m_firstInputList.emplace_back("");
                break;
            case 1:
                inputStr = parser->GetSwitchValue(ComparisonFirstInputArg, 0);
                if (LooksLikePath(inputStr))
                {
                    return AZ::Failure(AZStd::string::format(
                        "Invalid command: the \"--%s\" arg only accepts Tokens as inputs. Paths are not valid inputs.",
                        ComparisonFirstInputArg));
                }
                AzToolsFramework::AssetFileInfoListComparison::FormatOutputToken(inputStr);
                params.m_firstInputList.emplace_back(inputStr);
                break;
            default:
                return AZ::Failure(AZStd::string::format(
                    "Invalid command: when using the \"--%s\" arg, the \"--%s\" arg can accept no more than one input value.",
                    EditComparisonStepArg, ComparisonFirstInputArg));
                break;
            }
        }

        if (parser->HasSwitch(ComparisonSecondInputArg))
        {
            numInputs = parser->GetNumSwitchValues(ComparisonSecondInputArg);
            switch (numInputs)
            {
            case 0:
                // Our CLI parser will not return empty strings, so we need an extra case to check if a user wants to remove an input altogether
                params.m_secondInputList.emplace_back("");
                break;
            case 1:
                inputStr = parser->GetSwitchValue(ComparisonSecondInputArg, 0);
                if (LooksLikePath(inputStr))
                {
                    return AZ::Failure(AZStd::string::format(
                        "Invalid command: the \"--%s\" arg only accepts Tokens as inputs. Paths are not valid inputs.",
                        ComparisonSecondInputArg));
                }
                AzToolsFramework::AssetFileInfoListComparison::FormatOutputToken(inputStr);
                params.m_secondInputList.emplace_back(inputStr);
                break;
            default:
                return AZ::Failure(AZStd::string::format(
                    "Invalid command: when using the \"--%s\" arg, the \"--%s\" arg can accept no more than one input value.",
                    EditComparisonStepArg, ComparisonSecondInputArg));
                break;
            }
        }

        return AZ::Success();
    }

    AZ::Outcome<ComparisonParams, AZStd::string> ApplicationManager::ParseCompareCommandData(const AZ::CommandLine* parser)
    {
        auto validateArgsOutcome = ValidateInputArgs(parser, m_allCompareArgs);
        if (!validateArgsOutcome.IsSuccess())
        {
            OutputHelpCompare();
            return AZ::Failure(validateArgsOutcome.TakeError());
        }

        ComparisonParams params;

        // Read in Platform arg
        auto platformOutcome = GetPlatformArg(parser);
        if (!platformOutcome.IsSuccess())
        {
            return AZ::Failure(platformOutcome.GetError());
        }
        params.m_platformFlags = GetInputPlatformFlagsOrEnabledPlatformFlags(platformOutcome.GetValue());

        AZStd::string inferredPlatform;
        // read in input files (first and second)
        for (size_t idx = 0; idx < parser->GetNumSwitchValues(CompareFirstFileArg); idx++)
        {
            AZStd::string value = parser->GetSwitchValue(CompareFirstFileArg, idx);
            if (!value.starts_with(compareVariablePrefix)) // Don't make this a path if it starts with the variable prefix
            {
                FilePath path = FilePath(value);
                value = path.AbsolutePath();
                inferredPlatform = AzToolsFramework::GetPlatformIdentifier(value);
            }
            params.m_firstCompareFile.emplace_back(AZStd::move(value));
        }

        for (size_t idx = 0; idx < parser->GetNumSwitchValues(CompareSecondFileArg); idx++)
        {
            AZStd::string value = parser->GetSwitchValue(CompareSecondFileArg, idx);
            if (!value.starts_with(compareVariablePrefix)) // Don't make this a path if it starts with the variable prefix
            {
                FilePath path = FilePath(value);
                value = path.AbsolutePath();
            }
            params.m_secondCompareFile.emplace_back(AZStd::move(value));
        }

        // read in output files
        for (size_t idx = 0; idx < parser->GetNumSwitchValues(CompareOutputFileArg); idx++)
        {
            AZStd::string value = parser->GetSwitchValue(CompareOutputFileArg, idx);
            if (!value.starts_with(compareVariablePrefix)) // Don't make this a path if it starts with the variable prefix
            {
                FilePath path = FilePath(value, inferredPlatform);
                value = path.AbsolutePath();
            }

            params.m_outputs.emplace_back(AZStd::move(value));
        }

        // Make Path object for existing rules file to load
        AZ::Outcome< AZStd::string, AZStd::string> pathArgOutcome = GetFilePathArg(parser, ComparisonRulesFileArg, CompareCommand, false);
        if (!pathArgOutcome.IsSuccess())
        {
            return AZ::Failure(pathArgOutcome.GetError());
        }

        params.m_comparisonRulesFile = FilePath(pathArgOutcome.GetValue());

        // Parse info for additional rules
        auto comparisonParseOutcome = ParseComparisonTypesAndPatterns(parser, params.m_comparisonRulesParams);
        if (!comparisonParseOutcome.IsSuccess())
        {
            return AZ::Failure(comparisonParseOutcome.GetError());
        }

        for (size_t idx = 0; idx < parser->GetNumSwitchValues(ComparePrintArg); idx++)
        {
            AZStd::string value = parser->GetSwitchValue(ComparePrintArg, idx);
            if (!value.starts_with(compareVariablePrefix)) // Don't make this a path if it starts with the variable prefix
            {
                FilePath path = FilePath(value);
                value = path.AbsolutePath();
            }
            params.m_printComparisons.emplace_back(AZStd::move(value));
        }

        params.m_printLast = parser->HasSwitch(ComparePrintArg) && params.m_printComparisons.empty();

        if (params.m_comparisonRulesParams.m_intersectionCount && (params.m_outputs.size() != 0 && params.m_outputs.size() != 1))
        {
            return AZ::Failure(AZStd::string::format("Invalid command: \"--%s\" must have either be 0 or 1 value for compare operation of type ( %s ).",
                CompareOutputFileArg, AzToolsFramework::AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(AzToolsFramework::AssetFileInfoListComparison::ComparisonType::IntersectionCount)]));
        }

        // Read in Allow Overwrites flag
        params.m_allowOverwrites = parser->HasSwitch(AllowOverwritesFlag);

        return AZ::Success(params);
    }

    AZ::Outcome<BundleSettingsParams, AZStd::string> ApplicationManager::ParseBundleSettingsCommandData(const AZ::CommandLine* parser)
    {
        auto validateArgsOutcome = ValidateInputArgs(parser, m_allBundleSettingsArgs);
        if (!validateArgsOutcome.IsSuccess())
        {
            OutputHelpBundleSettings();
            return AZ::Failure(validateArgsOutcome.TakeError());
        }

        BundleSettingsParams params;

        // Read in Platform arg
        auto platformOutcome = GetPlatformArg(parser);
        if (!platformOutcome.IsSuccess())
        {
            return AZ::Failure(platformOutcome.GetError());
        }
        params.m_platformFlags = GetInputPlatformFlagsOrEnabledPlatformFlags(platformOutcome.GetValue());

        // Read in Bundle Settings File arg
        auto requiredArgOutcome = GetFilePathArg(parser, BundleSettingsFileArg, BundleSettingsCommand, true);
        if (!requiredArgOutcome.IsSuccess())
        {
            return AZ::Failure(requiredArgOutcome.GetError());
        }
        params.m_bundleSettingsFile = FilePath(requiredArgOutcome.GetValue());

        // Read in Asset List File arg
        auto argOutcome = GetFilePathArg(parser, AssetListFileArg, BundleSettingsCommand);
        if (!argOutcome.IsSuccess())
        {
            return AZ::Failure(argOutcome.GetError());
        }
        if (!argOutcome.GetValue().empty())
        {
            params.m_assetListFile = FilePath(argOutcome.GetValue());
        }

        // Read in Output Bundle Path arg
        argOutcome = GetFilePathArg(parser, OutputBundlePathArg, BundleSettingsCommand);
        if (!argOutcome.IsSuccess())
        {
            return AZ::Failure(argOutcome.GetError());
        }
        if (!argOutcome.GetValue().empty())
        {
            params.m_outputBundlePath = FilePath(argOutcome.GetValue());
        }

        // Read in Bundle Version arg
        if (parser->HasSwitch(BundleVersionArg))
        {
            if (parser->GetNumSwitchValues(BundleVersionArg) != 1)
            {
                return AZ::Failure(AZStd::string::format("Invalid command: \"--%s\" must have exactly one value.", BundleVersionArg));
            }
            params.m_bundleVersion = AZStd::stoi(parser->GetSwitchValue(BundleVersionArg, 0));
        }

        // Read in Max Bundle Size arg
        if (parser->HasSwitch(MaxBundleSizeArg))
        {
            if (parser->GetNumSwitchValues(MaxBundleSizeArg) != 1)
            {
                return AZ::Failure(AZStd::string::format("Invalid command: \"--%s\" must have exactly one value.", MaxBundleSizeArg));
            }
            params.m_maxBundleSizeInMB = AZStd::stoi(parser->GetSwitchValue(MaxBundleSizeArg, 0));
        }

        // Read in Print flag
        params.m_print = parser->HasSwitch(PrintFlag);

        return AZ::Success(params);
    }

    AZ::Outcome<BundlesParamsList, AZStd::string> ApplicationManager::ParseBundleSettingsAndOverrides(const AZ::CommandLine* parser, const char* commandName)
    {
        // Read in Bundle Settings File args
        auto bundleSettingsOutcome = GetArgsList<FilePath>(parser, BundleSettingsFileArg, commandName);
        if (!bundleSettingsOutcome.IsSuccess())
        {
            return AZ::Failure(bundleSettingsOutcome.GetError());
        }

        // Read in Asset List File args
        auto assetListOutcome = GetArgsList<FilePath>(parser, AssetListFileArg, commandName);
        if (!assetListOutcome.IsSuccess())
        {
            return AZ::Failure(assetListOutcome.GetError());
        }

        // Read in Output Bundle Path args
        auto bundleOutputPathOutcome = GetArgsList<FilePath>(parser, OutputBundlePathArg, commandName);
        if (!bundleOutputPathOutcome.IsSuccess())
        {
            return AZ::Failure(bundleOutputPathOutcome.GetError());
        }

        AZStd::vector<FilePath> bundleSettingsFileList = bundleSettingsOutcome.TakeValue();
        AZStd::vector<FilePath> assetListFileList = assetListOutcome.TakeValue();
        AZStd::vector<FilePath> outputBundleFileList = bundleOutputPathOutcome.TakeValue();

        size_t bundleSettingListSize = bundleSettingsFileList.size();
        size_t assetFileListSize = assetListFileList.size();
        size_t outputBundleListSize = outputBundleFileList.size();


        // * We are validating the following cases here
        // * AssetFileList should always be equal to outputBundleList size even if they are of zero length.
        // * BundleSettingList can be a zero size list if the number of elements in assetFileList matches the number of elements in outputBundleList.
        // * If bundleSettingList contains non zero elements than either it should have the same number of elements as in assetFileList or the number of elements in assetFileList should be zero.
        if (bundleSettingListSize)
        {
            if (assetFileListSize != outputBundleListSize)
            {
                return AZ::Failure(AZStd::string::format("Invalid command:  \"--%s\" and \"--%s\" are required and should contain the same number of args.", AssetListFileArg, OutputBundlePathArg));
            }
            else if (bundleSettingListSize != assetFileListSize && assetFileListSize != 0)
            {
                return AZ::Failure(AZStd::string::format("Invalid command: \"--%s\", \"--%s\" and \"--%s\" should contain the same number of args.", BundleSettingsFileArg, AssetListFileArg, OutputBundlePathArg));
            }
        }
        else
        {
            if (assetFileListSize != outputBundleListSize)
            {
                return AZ::Failure(AZStd::string::format("Invalid command:  \"--%s\" and \"--%s\" are required and should contain the same number of args.", AssetListFileArg, OutputBundlePathArg));
            }
        }

        size_t expectedListSize = AZStd::max(assetFileListSize, bundleSettingListSize);

        // Read in Bundle Version args
        auto bundleVersionOutcome = GetArgsList<AZStd::string>(parser, BundleVersionArg, commandName);
        if (!bundleVersionOutcome.IsSuccess())
        {
            return AZ::Failure(bundleVersionOutcome.GetError());
        }

        AZStd::vector<AZStd::string> bundleVersionList = bundleVersionOutcome.TakeValue();
        size_t bundleVersionListSize = bundleVersionList.size();

        if (bundleVersionListSize != expectedListSize && bundleVersionListSize >= 2)
        {
            if (expectedListSize != 1)
            {
                return AZ::Failure(AZStd::string::format("Invalid command: Number of args in \"--%s\" can either be zero, one or %zu. Actual size detected %zu.", BundleVersionArg, expectedListSize, bundleVersionListSize));
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Invalid command: Number of args in \"--%s\" is %zu. Expected number of args is one.", BundleVersionArg, bundleVersionListSize ));
            }
        }

        // Read in Max Bundle Size args
        auto maxBundleSizeOutcome = GetArgsList<AZStd::string>(parser, MaxBundleSizeArg, commandName);
        if (!maxBundleSizeOutcome.IsSuccess())
        {
            return AZ::Failure(maxBundleSizeOutcome.GetError());
        }

        AZStd::vector<AZStd::string> maxBundleSizeList = maxBundleSizeOutcome.TakeValue();
        size_t maxBundleListSize = maxBundleSizeList.size();

        if (maxBundleListSize != expectedListSize && maxBundleListSize >= 2)
        {
            if (expectedListSize != 1)
            {
                return AZ::Failure(AZStd::string::format("Invalid command: Number of args in \"--%s\" can either be zero, one or %zu. Actual size detected %zu.", MaxBundleSizeArg, expectedListSize, maxBundleListSize));
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Invalid command: Number of args in \"--%s\" is %zu. Expected number of args is one.", MaxBundleSizeArg, maxBundleListSize));
            }
        }

        // Read in Platform arg
        auto platformOutcome = GetPlatformArg(parser);
        if (!platformOutcome.IsSuccess())
        {
            return AZ::Failure(platformOutcome.GetError());
        }

        // Read in Allow Overwrites flag
        bool allowOverwrites = parser->HasSwitch(AllowOverwritesFlag);
        BundlesParamsList bundleParamsList;

        for (int idx = 0; idx < expectedListSize; idx++)
        {
            BundlesParams bundleParams;
            bundleParams.m_bundleSettingsFile = bundleSettingListSize ? bundleSettingsFileList[idx] : FilePath();
            bundleParams.m_assetListFile = assetFileListSize ? assetListFileList[idx] : FilePath();
            bundleParams.m_outputBundlePath = outputBundleListSize ? outputBundleFileList[idx] : FilePath();
            if (bundleVersionListSize)
            {
                bundleParams.m_bundleVersion = bundleVersionListSize == 1 ? AZStd::stoi(bundleVersionList[0]) : AZStd::stoi(bundleVersionList[idx]);
            }

            if (maxBundleListSize)
            {
                bundleParams.m_maxBundleSizeInMB = maxBundleListSize == 1 ? AZStd::stoi(maxBundleSizeList[0]) : AZStd::stoi(maxBundleSizeList[idx]);
            }

            bundleParams.m_platformFlags = platformOutcome.GetValue();
            bundleParams.m_allowOverwrites = allowOverwrites;
            bundleParamsList.emplace_back(bundleParams);
        }

        return AZ::Success(bundleParamsList);
    }

    AZ::Outcome<BundlesParamsList, AZStd::string> ApplicationManager::ParseBundlesCommandData(const AZ::CommandLine* parser)
    {
        auto validateArgsOutcome = ValidateInputArgs(parser, m_allBundlesArgs);
        if (!validateArgsOutcome.IsSuccess())
        {
            OutputHelpBundles();
            return AZ::Failure(validateArgsOutcome.TakeError());
        }

        auto parseSettingsOutcome = ParseBundleSettingsAndOverrides(parser, BundlesCommand);
        if (!parseSettingsOutcome.IsSuccess())
        {
            return AZ::Failure(parseSettingsOutcome.GetError());
        }

        return AZ::Success(parseSettingsOutcome.TakeValue());
    }

    AZ::Outcome<BundleSeedParams, AZStd::string> ApplicationManager::ParseBundleSeedCommandData(const AZ::CommandLine* parser)
    {

        auto validateArgsOutcome = ValidateInputArgs(parser, m_allBundleSeedArgs);
        if (!validateArgsOutcome.IsSuccess())
        {
            OutputHelpBundleSeed();
            return AZ::Failure(validateArgsOutcome.TakeError());
        }

        BundleSeedParams params;

        params.m_addSeedList = GetAddSeedArgList(parser);
        auto parseSettingsOutcome = ParseBundleSettingsAndOverrides(parser, BundleSeedCommand);
        if (!parseSettingsOutcome.IsSuccess())
        {
            return AZ::Failure(parseSettingsOutcome.GetError());
        }
        BundlesParamsList paramsList = parseSettingsOutcome.TakeValue();

        params.m_bundleParams = paramsList[0];

        return AZ::Success(params);
    }

    AZ::Outcome<void, AZStd::string> ApplicationManager::ValidateInputArgs(const AZ::CommandLine* parser, const AZStd::vector<const char*>& validArgList)
    {
        constexpr AZStd::string_view ApplicationArgList = "/O3DE/AzCore/Application/ValidCommandOptions";
        AZStd::vector<AZStd::string> validApplicationArgs;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->GetObject(validApplicationArgs, ApplicationArgList);
        }
        for (const auto& paramInfo : *parser)
        {
            // Skip positional arguments
            if (paramInfo.m_option.empty())
            {
                continue;
            }
            bool isValidArg = false;

            for (const auto& validArg : validArgList)
            {
                if (AZ::StringFunc::Equal(paramInfo.m_option, validArg))
                {
                    isValidArg = true;
                    break;
                }
            }
            for (const auto& validArg : validApplicationArgs)
            {
                if (AZ::StringFunc::Equal(paramInfo.m_option, validArg))
                {
                    isValidArg = true;
                    break;
                }
            }

            if (!isValidArg)
            {
                return AZ::Failure(AZStd::string::format(R"(Invalid argument: "--%s" is not a valid argument for this sub-command.)", paramInfo.m_option.c_str()));
            }
        }

        return AZ::Success();
    }

    AZ::Outcome<AZStd::string, AZStd::string> ApplicationManager::GetFilePathArg(const AZ::CommandLine* parser, const char* argName, const char* subCommandName, bool isRequired)
    {
        if (!parser->HasSwitch(argName))
        {
            if (isRequired)
            {
                return AZ::Failure(AZStd::string::format("Invalid command: \"--%s\" is required when running \"%s\".", argName, subCommandName));
            }
            return AZ::Success(AZStd::string());
        }

        if (parser->GetNumSwitchValues(argName) != 1)
        {
            return AZ::Failure(AZStd::string::format("Invalid command: \"--%s\" must have exactly one value.", argName));
        }

        return AZ::Success(parser->GetSwitchValue(argName, 0));
    }

    
    template <typename T>
    AZ::Outcome<AZStd::vector<T>, AZStd::string> ApplicationManager::GetArgsList(const AZ::CommandLine* parser, const char* argName, const char* subCommandName, bool isRequired)
    {
        AZStd::vector<T> args;

        if (!parser->HasSwitch(argName))
        {
            if (isRequired)
            {
                return AZ::Failure(AZStd::string::format("Invalid command: \"--%s\" is required when running \"%s\".", argName, subCommandName));
            }

            return AZ::Success(args);
        }

        size_t numValues = parser->GetNumSwitchValues(argName);

        for (int idx = 0; idx < numValues; ++idx)
        {
            args.emplace_back(T(parser->GetSwitchValue(argName, idx)));
        }

        return AZ::Success(args);
    }

    AZ::Outcome<AzFramework::PlatformFlags, AZStd::string> ApplicationManager::GetPlatformArg(const AZ::CommandLine* parser)
    {
        using namespace AzFramework;
        PlatformFlags platform = AzFramework::PlatformFlags::Platform_NONE;
        if (!parser->HasSwitch(PlatformArg))
        {
            return AZ::Success(platform);
        }

        size_t numValues = parser->GetNumSwitchValues(PlatformArg);
        if (numValues <= 0)
        {
            return AZ::Failure(AZStd::string::format("Invalid command: \"--%s\" must have at least one value.", PlatformArg));
        }

        for (int platformIdx = 0; platformIdx < numValues; ++platformIdx)
        {
            AZStd::string platformStr = parser->GetSwitchValue(PlatformArg, platformIdx);
            platform |= AzFramework::PlatformHelper::GetPlatformFlag(platformStr);
        }

        return AZ::Success(platform);
    }

    AzFramework::PlatformFlags ApplicationManager::GetInputPlatformFlagsOrEnabledPlatformFlags(AzFramework::PlatformFlags inputPlatformFlags)
    {
        using namespace AzToolsFramework;
        if (inputPlatformFlags != AzFramework::PlatformFlags::Platform_NONE)
        {
            return inputPlatformFlags;
        }

        // If no platform was specified, defaulting to platforms specified in the asset processor config files
        AzFramework::PlatformFlags platformFlags = GetEnabledPlatformFlags(
            AZStd::string_view{ AZ::Utils::GetEnginePath() },
            AZStd::string_view{ AZ::Utils::GetEnginePath() },
            AZStd::string_view{ AZ::Utils::GetProjectPath() });
        [[maybe_unused]] auto platformsString = AzFramework::PlatformHelper::GetCommaSeparatedPlatformList(platformFlags);

        AZ_TracePrintf(AppWindowName, "No platform specified, defaulting to platforms ( %s ).\n", platformsString.c_str());
        return platformFlags;
    }

    AZStd::vector<AZStd::string> ApplicationManager::GetAddSeedArgList(const AZ::CommandLine* parser)
    {
        AZStd::vector<AZStd::string> addSeedList;
        size_t numAddSeedArgs = parser->GetNumSwitchValues(AddSeedArg);
        for (size_t addSeedIndex = 0; addSeedIndex < numAddSeedArgs; ++addSeedIndex)
        {
            addSeedList.push_back(parser->GetSwitchValue(AddSeedArg, addSeedIndex));
        }
        return addSeedList;
    }

    AZStd::vector<AZStd::string> ApplicationManager::GetSkipArgList(const AZ::CommandLine* parser)
    {
        AZStd::vector<AZStd::string> skipList;
        size_t numArgs = parser->GetNumSwitchValues(SkipArg);
        for (size_t argIndex = 0; argIndex < numArgs; ++argIndex)
        {
            skipList.push_back(parser->GetSwitchValue(SkipArg, argIndex));
        }
        return skipList;
    }

    bool ApplicationManager::SeedsOperationRequiresCatalog(const SeedsParams& params)
    {
        return params.m_addSeedList.size() || params.m_addPlatformToAllSeeds || params.m_updateSeedPathHint || params.m_print;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Run Commands
    ////////////////////////////////////////////////////////////////////////////////////////////

    bool ApplicationManager::RunSeedsCommands(const AZ::Outcome<SeedsParams, AZStd::string>& paramsOutcome)
    {
        using namespace AzFramework;
        if (!paramsOutcome.IsSuccess())
        {
            AZ_Error(AppWindowName, false, paramsOutcome.GetError().c_str());
            return false;
        }

        SeedsParams params = paramsOutcome.GetValue();

        if (SeedsOperationRequiresCatalog(params))
        {
            // Asset Catalog
            auto catalogOutcome = InitAssetCatalog(params.m_platformFlags, params.m_assetCatalogFile.AbsolutePath());
            if (!catalogOutcome.IsSuccess())
            {
                AZ_Error(AppWindowName, false, catalogOutcome.GetError().c_str());
                return false;
            }
        }

        // Seed List File
        auto seedOutcome = LoadSeedListFile(params.m_seedListFile.AbsolutePath(), params.m_platformFlags);
        if (!seedOutcome.IsSuccess())
        {
            AZ_Error(AppWindowName, false, seedOutcome.GetError().c_str());
            return false;
        }

        for (const PlatformId platformId : AzFramework::PlatformHelper::GetPlatformIndices(params.m_platformFlags))
        {
            // Add Seeds
            PlatformFlags platformFlag = AzFramework::PlatformHelper::GetPlatformFlagFromPlatformIndex(platformId);
            for (const AZStd::string& assetPath : params.m_addSeedList)
            {
                m_assetSeedManager->AddSeedAsset(assetPath, platformFlag);
            }

            // Remove Seeds
            for (const AZStd::string& assetPath : params.m_removeSeedList)
            {
                m_assetSeedManager->RemoveSeedAsset(assetPath, platformFlag);
            }

            // Add Platform to All Seeds
            if (params.m_addPlatformToAllSeeds)
            {
                m_assetSeedManager->AddPlatformToAllSeeds(platformId);
            }

            // Remove Platform from All Seeds
            if (params.m_removePlatformFromAllSeeds)
            {
                m_assetSeedManager->RemovePlatformFromAllSeeds(platformId);
            }
        }

        if (params.m_updateSeedPathHint)
        {
            m_assetSeedManager->UpdateSeedPath();
        }

        if (params.m_removeSeedPathHint)
        {
            m_assetSeedManager->RemoveSeedPath();
        }

        if (params.m_print)
        {
            PrintSeedList(params.m_seedListFile.AbsolutePath());
        }

        // Save
        AZ_TracePrintf(AssetBundler::AppWindowName, "Saving Seed List to ( %s )...\n", params.m_seedListFile.AbsolutePath().c_str());
        if (!m_assetSeedManager->Save(params.m_seedListFile.AbsolutePath()))
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Unable to save Seed List to ( %s ).", params.m_seedListFile.AbsolutePath().c_str());
            return false;
        }

        AZ_TracePrintf(AssetBundler::AppWindowName, "Save successful!\n");

        return true;
    }

    bool ApplicationManager::RunAssetListsCommands(const AZ::Outcome<AssetListsParams, AZStd::string>& paramsOutcome)
    {
        using namespace AzFramework;
        if (!paramsOutcome.IsSuccess())
        {
            AZ_Error(AppWindowName, false, paramsOutcome.GetError().c_str());
            return false;
        }

        AssetListsParams params = paramsOutcome.GetValue();

        // Asset Catalog
        auto catalogOutcome = InitAssetCatalog(params.m_platformFlags, params.m_assetCatalogFile.AbsolutePath());
        if (!catalogOutcome.IsSuccess())
        {
            AZ_Error(AppWindowName, false, catalogOutcome.GetError().c_str());
            return false;
        }

        // Seed List Files
        AZ::Outcome<void, AZStd::string> seedListOutcome;
        AZStd::string seedListFileAbsolutePath;
        for (const FilePath& seedListFile : params.m_seedListFiles)
        {
            seedListFileAbsolutePath = seedListFile.AbsolutePath();
            if (!AZ::IO::FileIOBase::GetInstance()->Exists(seedListFileAbsolutePath.c_str()))
            {
                AZ_Error(AppWindowName, false, "Cannot load Seed List file ( %s ): File does not exist.\n", seedListFileAbsolutePath.c_str());
                return false;
            }

            seedListOutcome = LoadSeedListFile(seedListFileAbsolutePath, params.m_platformFlags);
            if (!seedListOutcome.IsSuccess())
            {
                AZ_Error(AppWindowName, false, seedListOutcome.GetError().c_str());
                return false;
            }
        }

        // Add Default Seed List Files
        if (params.m_addDefaultSeedListFiles)
        {
            AZStd::unordered_map<AZStd::string, AZStd::string> defaultSeedListFiles = GetDefaultSeedListFiles(GetEngineRoot(), AZ::Utils::GetProjectPath(),
                m_gemInfoList, params.m_platformFlags);
            if (defaultSeedListFiles.empty())
            {
                // Error has already been thrown
                return false;
            }

            for (const auto& seedListFile : defaultSeedListFiles)
            {
                seedListOutcome = LoadSeedListFile(seedListFile.first, params.m_platformFlags);
                if (!seedListOutcome.IsSuccess())
                {
                    AZ_Error(AppWindowName, false, seedListOutcome.GetError().c_str());
                    return false;
                }
            }

            AZStd::vector<AZStd::string> defaultSeeds = GetDefaultSeeds(AZ::Utils::GetProjectPath(), m_currentProjectName);
            if (defaultSeeds.empty())
            {
                // Error has already been thrown
                return false;
            }

            for (const auto& seed : defaultSeeds)
            {
                m_assetSeedManager->AddSeedAsset(seed, params.m_platformFlags);
            }
        }

        if (!RunPlatformSpecificAssetListCommands(params, params.m_platformFlags))
        {
            return false;
        }

        return true;
    }

    bool ApplicationManager::RunComparisonRulesCommands(const AZ::Outcome<ComparisonRulesParams, AZStd::string>& paramsOutcome)
    {
        using namespace AzToolsFramework;

        if (!paramsOutcome.IsSuccess())
        {
            AZ_Error(AppWindowName, false, paramsOutcome.GetError().c_str());
            return false;
        }

        ComparisonRulesParams params = paramsOutcome.GetValue();
        AssetFileInfoListComparison comparisonOperations;

        // Read the input ComparisonRules file into memory. If it does not already exist, we are going to create a new file.
        if (AZ::IO::FileIOBase::GetInstance()->Exists(params.m_comparisonRulesFile.AbsolutePath().c_str()))
        {
            auto rulesFileLoadOutcome = AssetFileInfoListComparison::Load(params.m_comparisonRulesFile.AbsolutePath());
            if (!rulesFileLoadOutcome.IsSuccess())
            {
                AZ_Error(AppWindowName, false, rulesFileLoadOutcome.GetError().c_str());
                return false;
            }
            comparisonOperations = rulesFileLoadOutcome.GetValue();
        }

        // Perform any editing operations (no need to throw errors on failure, they are already thrown elsewhere)
        switch (params.m_comparisonRulesStepAction)
        {
        case(ComparisonRulesStepAction::Add):
            if (!ConvertRulesParamsToComparisonData(params, comparisonOperations, params.m_destinationLine))
            {
                return false;
            }
            break;
        case(ComparisonRulesStepAction::AddToEnd):
            if (!ConvertRulesParamsToComparisonData(params, comparisonOperations, comparisonOperations.GetNumComparisonSteps()))
            {
                return false;
            }
            break;
        case(ComparisonRulesStepAction::Remove):
            if (!comparisonOperations.RemoveComparisonStep(params.m_initialLine))
            {
                return false;
            }
            break;
        case(ComparisonRulesStepAction::Move):
            if (!comparisonOperations.MoveComparisonStep(params.m_initialLine, params.m_destinationLine))
            {
                return false;
            }
            break;
        case(ComparisonRulesStepAction::Edit):
            if (!EditComparisonData(params, comparisonOperations, params.m_initialLine))
            {
                return false;
            }
            break;
        }

        if (params.m_print)
        {
            PrintComparisonRules(comparisonOperations, params.m_comparisonRulesFile.AbsolutePath());
        }

        // Attempt to save
        if (params.m_comparisonRulesStepAction != ComparisonRulesStepAction::Default)
        {
            AZ_TracePrintf(AssetBundler::AppWindowName, "Saving Comparison Rules file to ( %s )...\n", params.m_comparisonRulesFile.AbsolutePath().c_str());
            if (!comparisonOperations.Save(params.m_comparisonRulesFile.AbsolutePath().c_str()))
            {
                AZ_Error(AssetBundler::AppWindowName, false, "Failed to save Comparison Rules file ( %s ).", params.m_comparisonRulesFile.AbsolutePath().c_str());
                return false;
            }
            AZ_TracePrintf(AssetBundler::AppWindowName, "Save successful!\n");
        }

        return true;
    }

    bool ApplicationManager::ConvertRulesParamsToComparisonData(const ComparisonRulesParams& params, AzToolsFramework::AssetFileInfoListComparison& assetListComparison, size_t startingIndex)
    {
        using namespace AzToolsFramework;

        for (int idx = 0; idx < params.m_comparisonTypeList.size(); idx++)
        {
            AssetFileInfoListComparison::ComparisonData comparisonData;
            comparisonData.m_comparisonType = params.m_comparisonTypeList[idx];
            comparisonData.m_filePattern = params.m_filePatternList[idx];
            comparisonData.m_filePatternType = params.m_filePatternTypeList[idx];
            comparisonData.m_output = params.m_tokenNamesList[idx];
            comparisonData.m_intersectionCount = params.m_intersectionCount;

            if (!params.m_firstInputList.empty())
            {
                comparisonData.m_firstInput = params.m_firstInputList[idx];
            }

            if (comparisonData.m_comparisonType != AssetFileInfoListComparison::ComparisonType::FilePattern)
            {
                if (!params.m_secondInputList.empty())
                {
                    comparisonData.m_secondInput = params.m_secondInputList[idx];
                }
            }

            if (!assetListComparison.AddComparisonStep(comparisonData, startingIndex))
            {
                // Error has already been thrown
                return false;
            }

            ++startingIndex;
        }

        return true;
    }

    bool ApplicationManager::EditComparisonData(const ComparisonRulesParams& params, AzToolsFramework::AssetFileInfoListComparison& assetListComparison, size_t index)
    {
        using namespace AzToolsFramework;

        // Errors are thrown by the Asset List Comparison functions, no need to write our own here

        if (!params.m_comparisonTypeList.empty() && !assetListComparison.SetComparisonType(index, params.m_comparisonTypeList[0]))
        {
            return false;
        }

        if (!params.m_filePatternTypeList.empty() && !assetListComparison.SetFilePatternType(index, params.m_filePatternTypeList[0]))
        {
            return false;
        }

        if (!params.m_filePatternList.empty() && !assetListComparison.SetFilePattern(index, params.m_filePatternList[0]))
        {
            return false;
        }

        if (!params.m_tokenNamesList.empty() && !assetListComparison.SetOutput(index, params.m_tokenNamesList[0]))
        {
            return false;
        }

        if (!params.m_firstInputList.empty() && !assetListComparison.SetFirstInput(index, params.m_firstInputList[0]))
        {
            return false;
        }

        if (!params.m_secondInputList.empty() && !assetListComparison.SetSecondInput(index, params.m_secondInputList[0]))
        {
            return false;
        }

        return true;
    }

    void ApplicationManager::PrintComparisonRules(const AzToolsFramework::AssetFileInfoListComparison& assetListComparison, const AZStd::string& comparisonRulesAbsoluteFilePath)
    {
        AZ_Printf(AppWindowName, "\nContents of: %s\n\n", comparisonRulesAbsoluteFilePath.c_str());

        const char* inputVariableMessage = "[input at runtime]";

        int lineNum = 0;
        for (const auto& comparisonData : assetListComparison.GetComparisonList())
        {
            const char* comparisonTypeName = AzToolsFramework::AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(comparisonData.m_comparisonType)];
            AZ_Printf(AppWindowName, "%-10i %-15s (%s", lineNum, comparisonTypeName, comparisonData.m_firstInput.empty() ? inputVariableMessage : comparisonData.m_firstInput.c_str());

            if (comparisonData.m_filePatternType != AzToolsFramework::AssetFileInfoListComparison::FilePatternType::Default)
            {
                AZ_Printf(AppWindowName, ")\n");

                const char* filePatternTypeName = AzToolsFramework::AssetFileInfoListComparison::FilePatternTypeNames[aznumeric_cast<AZ::u8>(comparisonData.m_filePatternType)];
                AZ_Printf(AppWindowName, "%-14s %s    \"%s\"\n", "", filePatternTypeName, comparisonData.m_filePattern.c_str());
            }
            else
            {
                AZ_Printf(AppWindowName, ", %s )\n", comparisonData.m_secondInput.empty() ? inputVariableMessage : comparisonData.m_secondInput.c_str());
            }

            AZ_Printf(AppWindowName, "%-14s Output Token: %s\n", "", comparisonData.m_output.empty() ? "[No Token Set]" : comparisonData.m_output.c_str());
            ++lineNum;
        }
        AZ_Printf(AppWindowName, "\n");
    }

    bool ApplicationManager::IsDefaultToken(const AZStd::string& pathOrToken)
    {
        return pathOrToken.size() == 1 && pathOrToken.at(0) == compareVariablePrefix;
    }

    bool ApplicationManager::RunCompareCommand(const AZ::Outcome<ComparisonParams, AZStd::string>& paramsOutcome)
    {
        using namespace AzToolsFramework;

        if (!paramsOutcome.IsSuccess())
        {
            AZ_Error(AppWindowName, false, paramsOutcome.GetError().c_str());
            return false;
        }

        AssetFileInfoListComparison rulesFileComparisonOperations;

        // Load comparison rules from file if one was provided
        if (!paramsOutcome.GetValue().m_comparisonRulesFile.AbsolutePath().empty())
        {

            auto rulesFileLoadResult = AssetFileInfoListComparison::Load(paramsOutcome.GetValue().m_comparisonRulesFile.AbsolutePath());
            if (!rulesFileLoadResult.IsSuccess())
            {
                AZ_Error(AppWindowName, false, rulesFileLoadResult.GetError().c_str());
                return false;
            }
            rulesFileComparisonOperations = rulesFileLoadResult.GetValue();
        }

        bool hasError = false;

        for (AZStd::string platformName : AzFramework::PlatformHelper::GetPlatformsInterpreted(paramsOutcome.GetValue().m_platformFlags))
        {
            AZ_TracePrintf(AssetBundler::AppWindowName, "Running Compare command for the %s platform...\n", platformName.c_str());

            ComparisonParams params = paramsOutcome.GetValue();
            AddPlatformToAllComparisonParams(params, platformName);

            AssetFileInfoListComparison comparisonOperations = rulesFileComparisonOperations;

            // generate comparisons from additional commands and add it to comparisonOperations
            ConvertRulesParamsToComparisonData(params.m_comparisonRulesParams, comparisonOperations, comparisonOperations.GetNumComparisonSteps());

            if (params.m_comparisonRulesParams.m_intersectionCount)
            {
                if ((comparisonOperations.GetComparisonList().size() == 1) && comparisonOperations.GetComparisonList()[0].m_comparisonType != AssetFileInfoListComparison::ComparisonType::IntersectionCount)
                {
                    AZ_Error(AppWindowName, false, "Invalid arguement detected. Command ( --%s ) is incompatible with compare operation of type (%s).",
                        IntersectionCountArg, AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(comparisonOperations.GetComparisonList()[0].m_comparisonType)]);
                    return false;
                }
                // Since IntersectionCount Operation cannot be combined with other operation Comparison List should be 1
                else if (comparisonOperations.GetComparisonList().size() > 1)
                {
                    AZ_Error(AppWindowName, false, "Compare operation of type ( %s ) cannot be combined with other comparison operations. Number of comparison operation detected (%d).",
                        AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(AssetFileInfoListComparison::ComparisonType::IntersectionCount)], comparisonOperations.GetComparisonList().size());
                    return false;
                }

                if (params.m_outputs.size())
                {
                    comparisonOperations.SetOutput(0, params.m_outputs[0]);
                }
            }
            else
            {
                //Store input and output values alongside the Comparison Steps they relate to
                size_t secondInputIdx = 0;
                for (size_t idx = 0; idx < comparisonOperations.GetComparisonList().size(); ++idx)
                {
                    if (idx >= params.m_firstCompareFile.size())
                    {
                        AZ_Error(AppWindowName, false,
                            "Invalid command: The number of \"--%s\" inputs ( %i ) must match the number of Comparison Steps ( %i ).",
                            CompareFirstFileArg, params.m_firstCompareFile.size(), comparisonOperations.GetComparisonList().size());
                        return false;
                    }

                    // Set the first input
                    if (!IsDefaultToken(params.m_firstCompareFile.at(idx)))
                    {
                        comparisonOperations.SetFirstInput(idx, params.m_firstCompareFile.at(idx));
                    }

                    // Set the second input (if needed)
                    if (comparisonOperations.GetComparisonList().at(idx).m_comparisonType != AssetFileInfoListComparison::ComparisonType::FilePattern)
                    {
                        if (secondInputIdx >= params.m_secondCompareFile.size())
                        {
                            AZ_Error(AppWindowName, false,
                                "Invalid command: The number of \"--%s\" inputs ( %i ) must match the number of Comparison Steps that take two inputs.",
                                CompareSecondFileArg, params.m_secondCompareFile.size());
                            return false;
                        }

                        if (!IsDefaultToken(params.m_secondCompareFile.at(secondInputIdx)))
                        {
                            comparisonOperations.SetSecondInput(idx, params.m_secondCompareFile.at(secondInputIdx));
                        }

                        ++secondInputIdx;
                    }

                    // Set the output
                    if (idx >= params.m_outputs.size())
                    {
                        AZ_Error(AppWindowName, false,
                            "Invalid command: The number of \"--%s\" values ( %i ) must match the number of Comparison Steps ( %i ).",
                            CompareOutputFileArg, params.m_outputs.size(), comparisonOperations.GetComparisonList().size());
                        return false;
                    }

                    if (!IsDefaultToken(params.m_outputs.at(idx)))
                    {
                        comparisonOperations.SetOutput(idx, params.m_outputs.at(idx));
                    }
                }
            }


            AZ::Outcome<AssetFileInfoList, AZStd::string> compareOutcome = comparisonOperations.Compare(params.m_firstCompareFile);
            if (!compareOutcome.IsSuccess())
            {
                AZ_Error(AppWindowName, false, compareOutcome.GetError().c_str());
                hasError = true;
                continue;
            }

            if (params.m_printLast)
            {
                PrintComparisonAssetList(compareOutcome.GetValue(), params.m_outputs.size() ? params.m_outputs.back() : "");
            }

            // Check if we are performing a destructive overwrite that the user did not approve 
            if (!params.m_allowOverwrites)
            {
                AZStd::vector<AZStd::string> destructiveOverwriteFilePaths = comparisonOperations.GetDestructiveOverwriteFilePaths();
                if (!destructiveOverwriteFilePaths.empty())
                {
#if defined(AZ_ENABLE_TRACING)
                    for (const AZStd::string& path : destructiveOverwriteFilePaths)
                    {
                        AZ_Error(AssetBundler::AppWindowName, false, "Asset List file ( %s ) already exists, running this command would perform a destructive overwrite.", path.c_str());
                    }
#endif
                    AZ_Printf(AssetBundler::AppWindowName, "\nRun your command again with the ( --%s ) arg if you want to save over the existing file.\n\n", AllowOverwritesFlag)
                    hasError = true;
                    continue;
                }
            }

            AZ_Printf(AssetBundler::AppWindowName, "Saving results of comparison operation...\n");
            auto saveOutcome = comparisonOperations.SaveResults();
            if (!saveOutcome.IsSuccess())
            {
                AZ_Error(AppWindowName, false, saveOutcome.GetError().c_str());
                hasError = true;
                continue;
            }
            AZ_Printf(AssetBundler::AppWindowName, "Save successful!\n");

            for (const AZStd::string& comparisonKey : params.m_printComparisons)
            {
                PrintComparisonAssetList(comparisonOperations.GetComparisonResults(comparisonKey), comparisonKey);
            }
        }

        return !hasError;
    }

    void ApplicationManager::AddPlatformToAllComparisonParams(ComparisonParams& params, const AZStd::string& platformName)
    {
        for (size_t i = 0; i < params.m_firstCompareFile.size(); ++i)
        {
            AddPlatformToComparisonParam(params.m_firstCompareFile[i], platformName);
        }

        for (size_t i = 0; i < params.m_secondCompareFile.size(); ++i)
        {
            AddPlatformToComparisonParam(params.m_secondCompareFile[i], platformName);
        }

        for (size_t i = 0; i < params.m_outputs.size(); ++i)
        {
            AddPlatformToComparisonParam(params.m_outputs[i], platformName);
        }
    }

    void ApplicationManager::AddPlatformToComparisonParam(AZStd::string& inOut, const AZStd::string& platformName)
    {
        // Tokens don't have platforms
        if (AzToolsFramework::AssetFileInfoListComparison::IsTokenFile(inOut))
        {
            return;
        }

        AzToolsFramework::RemovePlatformIdentifier(inOut);
        FilePath tempPath(inOut, platformName);
        inOut = tempPath.AbsolutePath();
    }

    void ApplicationManager::PrintComparisonAssetList(const AzToolsFramework::AssetFileInfoList& infoList, const AZStd::string& resultName)
    {
        using namespace AzToolsFramework;

        if (infoList.m_fileInfoList.size() == 0)
        {
            return;
        }

        AZ_Printf(AssetBundler::AppWindowName, "Printing assets from the comparison result %s.\n", resultName.c_str());
        AZ_Printf(AssetBundler::AppWindowName, "------------------------------------------\n");

        for (const AssetFileInfo& assetFilenfo : infoList.m_fileInfoList)
        {
            AZ_Printf(AssetBundler::AppWindowName, "- %s\n", assetFilenfo.m_assetRelativePath.c_str());
        }

        AZ_Printf(AssetBundler::AppWindowName, "Total number of assets (%u).\n", infoList.m_fileInfoList.size());
        AZ_Printf(AssetBundler::AppWindowName, "---------------------------\n");
    }

    bool ApplicationManager::RunBundleSettingsCommands(const AZ::Outcome<BundleSettingsParams, AZStd::string>& paramsOutcome)
    {
        using namespace AzToolsFramework;
        if (!paramsOutcome.IsSuccess())
        {
            AZ_Error(AppWindowName, false, paramsOutcome.GetError().c_str());
            return false;
        }

        BundleSettingsParams params = paramsOutcome.GetValue();

        for (AZStd::string_view platformName : AzFramework::PlatformHelper::GetPlatformsInterpreted(params.m_platformFlags))
        {
            AssetBundleSettings bundleSettings;

            // Attempt to load Bundle Settings file. If the load operation fails, we are making a new file and there is no need to error.
            FilePath platformSpecificBundleSettingsFilePath = FilePath(params.m_bundleSettingsFile.AbsolutePath(), platformName);
            AZ::Outcome<AssetBundleSettings, AZStd::string> loadBundleSettingsOutcome = AssetBundleSettings::Load(platformSpecificBundleSettingsFilePath.AbsolutePath());
            if (loadBundleSettingsOutcome.IsSuccess())
            {
                bundleSettings = loadBundleSettingsOutcome.TakeValue();
            }

            // Asset List File
            AZStd::string assetListFilePath = FilePath(params.m_assetListFile.AbsolutePath(), platformName).AbsolutePath();
            if (!assetListFilePath.empty())
            {
                if (!AZ::StringFunc::EndsWith(assetListFilePath, AssetSeedManager::GetAssetListFileExtension()))
                {
                    AZ_Error(AppWindowName, false, "Cannot set Asset List file to ( %s ): file extension must be ( %s ).", assetListFilePath.c_str(), AssetSeedManager::GetAssetListFileExtension());
                    return false;
                }

                if (!AZ::IO::FileIOBase::GetInstance()->Exists(assetListFilePath.c_str()))
                {
                    AZ_Error(AppWindowName, false, "Cannot set Asset List file to ( %s ): file does not exist.", assetListFilePath.c_str());
                    return false;
                }

                // Make the path relative to the engine root folder before saving
                AZ::StringFunc::Replace(assetListFilePath, GetEngineRoot(), "");

                bundleSettings.m_assetFileInfoListPath = assetListFilePath;
            }

            // Output Bundle Path
            AZStd::string outputBundlePath = FilePath(params.m_outputBundlePath.AbsolutePath(), platformName).AbsolutePath();
            if (!outputBundlePath.empty())
            {
                if (!AZ::StringFunc::EndsWith(outputBundlePath, AssetBundleSettings::GetBundleFileExtension()))
                {
                    AZ_Error(AppWindowName, false, "Cannot set Output Bundle Path to ( %s ): file extension must be ( %s ).", outputBundlePath.c_str(), AssetBundleSettings::GetBundleFileExtension());
                    return false;
                }

                // Make the path relative to the engine root folder before saving
                AZ::StringFunc::Replace(outputBundlePath, GetEngineRoot(), "");

                bundleSettings.m_bundleFilePath = outputBundlePath;
            }

            // Bundle Version
            if (params.m_bundleVersion > 0 && params.m_bundleVersion <= AzFramework::AssetBundleManifest::CurrentBundleVersion)
            {
                bundleSettings.m_bundleVersion = params.m_bundleVersion;
            }

            // Max Bundle Size (in MB)
            if (params.m_maxBundleSizeInMB > 0 && params.m_maxBundleSizeInMB <= AssetBundleSettings::GetMaxBundleSizeInMB())
            {
                bundleSettings.m_maxBundleSizeInMB = params.m_maxBundleSizeInMB;
            }

            // Print
            if (params.m_print)
            {
                AZ_TracePrintf(AssetBundler::AppWindowName, "\nContents of Bundle Settings file ( %s ):\n", platformSpecificBundleSettingsFilePath.AbsolutePath().c_str());
                AZ_TracePrintf(AssetBundler::AppWindowName, "    Platform: %.*s\n", aznumeric_cast<int>(platformName.size()), platformName.data());
                AZ_TracePrintf(AssetBundler::AppWindowName, "    Asset List file: %s\n", bundleSettings.m_assetFileInfoListPath.c_str());
                AZ_TracePrintf(AssetBundler::AppWindowName, "    Output Bundle path: %s\n", bundleSettings.m_bundleFilePath.c_str());
                AZ_TracePrintf(AssetBundler::AppWindowName, "    Bundle Version: %i\n", bundleSettings.m_bundleVersion);
                AZ_TracePrintf(AssetBundler::AppWindowName, "    Max Bundle Size: %u MB\n\n", bundleSettings.m_maxBundleSizeInMB);
            }

            // Save
            AZ_TracePrintf(AssetBundler::AppWindowName, "Saving Bundle Settings file to ( %s )...\n", platformSpecificBundleSettingsFilePath.AbsolutePath().c_str());

            if (!AssetBundleSettings::Save(bundleSettings, platformSpecificBundleSettingsFilePath.AbsolutePath()))
            {
                AZ_Error(AssetBundler::AppWindowName, false, "Unable to save Bundle Settings file to ( %s ).", platformSpecificBundleSettingsFilePath.AbsolutePath().c_str());
                return false;
            }

            AZ_TracePrintf(AssetBundler::AppWindowName, "Save successful!\n");
        }

        return true;
    }

    bool ApplicationManager::RunBundlesCommands(const AZ::Outcome<BundlesParamsList, AZStd::string>& paramsOutcome)
    {
        using namespace AzToolsFramework;
        if (!paramsOutcome.IsSuccess())
        {
            AZ_Error(AppWindowName, false, paramsOutcome.GetError().c_str());
            return false;
        }

        BundlesParamsList paramsList = paramsOutcome.GetValue();
        AZStd::vector<AZStd::pair<AssetBundleSettings, BundlesParams>> allBundleSettings;
        for (BundlesParams& params : paramsList)
        {
            // If no platform was input we want to loop over all possible platforms and make bundles for whatever we find
            if (params.m_platformFlags == AzFramework::PlatformFlags::Platform_NONE)
            {
                params.m_platformFlags = AzFramework::PlatformFlags::AllNamedPlatforms;
            }

            // Load or generate Bundle Settings
            AzFramework::PlatformFlags allPlatformsInBundle = AzFramework::PlatformFlags::Platform_NONE;
            if (params.m_bundleSettingsFile.AbsolutePath().empty())
            {
                // Verify input file path formats before looking for platform-specific versions 
                auto fileExtensionOutcome = AssetFileInfoList::ValidateAssetListFileExtension(params.m_assetListFile.AbsolutePath());
                if (!fileExtensionOutcome.IsSuccess())
                {
                    AZ_Error(AssetBundler::AppWindowName, false, fileExtensionOutcome.GetError().c_str());
                    return false;
                }

                AZStd::vector<FilePath> allAssetListFilePaths = GetAllPlatformSpecificFilesOnDisk(params.m_assetListFile, params.m_platformFlags);

                // Create temporary Bundle Settings structs for every Asset List file
                for (const auto& assetListFilePath : allAssetListFilePaths)
                {
                    AssetBundleSettings bundleSettings;
                    bundleSettings.m_assetFileInfoListPath = assetListFilePath.AbsolutePath();
                    bundleSettings.m_platform = GetPlatformIdentifier(assetListFilePath.AbsolutePath());
                    allPlatformsInBundle |= AzFramework::PlatformHelper::GetPlatformFlag(bundleSettings.m_platform);
                    allBundleSettings.emplace_back(AZStd::make_pair(bundleSettings, params));
                }
            }
            else
            {
                // Verify input file path formats before looking for platform-specific versions 
                auto fileExtensionOutcome = AssetBundleSettings::ValidateBundleSettingsFileExtension(params.m_bundleSettingsFile.AbsolutePath());
                if (!fileExtensionOutcome.IsSuccess())
                {
                    AZ_Error(AssetBundler::AppWindowName, false, fileExtensionOutcome.GetError().c_str());
                    return false;
                }

                AZStd::vector<FilePath> allBundleSettingsFilePaths = GetAllPlatformSpecificFilesOnDisk(params.m_bundleSettingsFile, params.m_platformFlags);

                // Attempt to load all Bundle Settings files (there may be one or many to load)
                for (const auto& bundleSettingsFilePath : allBundleSettingsFilePaths)
                {
                    AZ::Outcome<AssetBundleSettings, AZStd::string> loadBundleSettingsOutcome = AssetBundleSettings::Load(bundleSettingsFilePath.AbsolutePath());
                    if (!loadBundleSettingsOutcome.IsSuccess())
                    {
                        AZ_Error(AssetBundler::AppWindowName, false, loadBundleSettingsOutcome.GetError().c_str());
                        return false;
                    }

                    allBundleSettings.emplace_back(AZStd::make_pair(loadBundleSettingsOutcome.TakeValue(), params));
                    allPlatformsInBundle |= AzFramework::PlatformHelper::GetPlatformFlag(allBundleSettings.back().first.m_platform);
                }
            }
        }

        AZStd::atomic_uint failureCount = 0;

        // Create all Bundles
        AZ::parallel_for_each(allBundleSettings.begin(), allBundleSettings.end(), [this, &failureCount](AZStd::pair<AzToolsFramework::AssetBundleSettings, BundlesParams> bundleSettings)
            {
                BundlesParams params = bundleSettings.second;
                auto overrideOutcome = ApplyBundleSettingsOverrides(
                    bundleSettings.first,
                    params.m_assetListFile.AbsolutePath(),
                    params.m_outputBundlePath.AbsolutePath(),
                    params.m_bundleVersion,
                    params.m_maxBundleSizeInMB);
                if (!overrideOutcome.IsSuccess())
                {
                    // Metric event has already been sent
                    AZ_Error(AppWindowName, false, overrideOutcome.GetError().c_str());
                    failureCount.fetch_add(1, AZStd::memory_order::memory_order_relaxed);
                    return;
                }

                FilePath bundleFilePath(bundleSettings.first.m_bundleFilePath);

                // Check if we are performing a destructive overwrite that the user did not approve 
                if (!params.m_allowOverwrites && AZ::IO::FileIOBase::GetInstance()->Exists(bundleFilePath.AbsolutePath().c_str()))
                {
                    AZ_Error(AssetBundler::AppWindowName, false, "Bundle ( %s ) already exists, running this command would perform a destructive overwrite.\n\n"
                        "Run your command again with the ( --%s ) arg if you want to save over the existing file.", bundleFilePath.AbsolutePath().c_str(), AllowOverwritesFlag);
                    failureCount.fetch_add(1, AZStd::memory_order::memory_order_relaxed);
                    return;
                }

                AZ_TracePrintf(AssetBundler::AppWindowName, "Creating Bundle ( %s )...\n", bundleFilePath.AbsolutePath().c_str());
                bool result = false;
                AssetBundleCommandsBus::BroadcastResult(result, &AssetBundleCommandsBus::Events::CreateAssetBundle, bundleSettings.first);
                if (!result)
                {
                    AZ_Error(AssetBundler::AppWindowName, false, "Unable to create bundle, target Bundle file path is ( %s ).", bundleFilePath.AbsolutePath().c_str());
                    failureCount.fetch_add(1, AZStd::memory_order::memory_order_relaxed);
                    return;
                }
                AZ_TracePrintf(AssetBundler::AppWindowName, "Bundle ( %s ) created successfully!\n", bundleFilePath.AbsolutePath().c_str());
            });

        return failureCount == 0;
    }

    bool ApplicationManager::RunBundleSeedCommands(const AZ::Outcome<BundleSeedParams, AZStd::string>& paramsOutcome)
    {
        using namespace AzToolsFramework;

        if (!paramsOutcome.IsSuccess())
        {
            AZ_Error(AppWindowName, false, paramsOutcome.GetError().c_str());
            return false;
        }

        BundleSeedParams params = paramsOutcome.GetValue();

        // If no platform was input we want to loop over all possible platforms and make bundles for whatever we find
        if (params.m_bundleParams.m_platformFlags == AzFramework::PlatformFlags::Platform_NONE)
        {
            params.m_bundleParams.m_platformFlags = AzFramework::PlatformFlags::AllNamedPlatforms;
        }

        AZStd::vector<AssetBundleSettings> allBundleSettings;
        if (params.m_bundleParams.m_bundleSettingsFile.AbsolutePath().empty())
        {
            // if no bundle settings file was provided generate one for each platform, values will be overridden later
            for (AZStd::string_view platformName : AzFramework::PlatformHelper::GetPlatformsInterpreted(params.m_bundleParams.m_platformFlags))
            {
                allBundleSettings.emplace_back();
                allBundleSettings.back().m_platform = platformName;
            }
            
        }
        else
        {
            // if a bundle settings file was provided use values from the file, leave the asset list file path behind since it will be ignored anyways
            AZStd::vector<FilePath> allBundleSettingsFilePaths = GetAllPlatformSpecificFilesOnDisk(params.m_bundleParams.m_bundleSettingsFile, params.m_bundleParams.m_platformFlags);

            // Attempt to load all Bundle Settings files (there may be one or many to load)
            for (const auto& bundleSettingsFilePath : allBundleSettingsFilePaths)
            {
                AZ::Outcome<AssetBundleSettings, AZStd::string> loadBundleSettingsOutcome = AssetBundleSettings::Load(bundleSettingsFilePath.AbsolutePath());
                if (!loadBundleSettingsOutcome.IsSuccess())
                {
                    AZ_Error(AssetBundler::AppWindowName, false, loadBundleSettingsOutcome.GetError().c_str());
                    return false;
                }
                allBundleSettings.emplace_back(loadBundleSettingsOutcome.TakeValue());
            }
        }

        // Create all Bundles
        for (AssetBundleSettings& bundleSettings : allBundleSettings)
        {
            auto overrideOutcome = ApplyBundleSettingsOverrides(
                bundleSettings,
                params.m_bundleParams.m_assetListFile.AbsolutePath(),
                params.m_bundleParams.m_outputBundlePath.AbsolutePath(),
                params.m_bundleParams.m_bundleVersion,
                params.m_bundleParams.m_maxBundleSizeInMB);

            if (!overrideOutcome.IsSuccess())
            {
                // Metric event has already been sent
                AZ_Error(AppWindowName, false, overrideOutcome.GetError().c_str());
                return false;
            }

            if (!params.m_bundleParams.m_allowOverwrites && AZ::IO::FileIOBase::GetInstance()->Exists(bundleSettings.m_bundleFilePath.c_str()))
            {
                AZ_Error(AssetBundler::AppWindowName, false, "Bundle ( %s ) already exists, running this command would perform a destructive overwrite.\n\n"
                    "Run your command again with the ( --%s ) arg if you want to save over the existing file.", bundleSettings.m_bundleFilePath.c_str(), AllowOverwritesFlag);
                return false;
            }

            AzFramework::PlatformFlags platformFlag = AzFramework::PlatformHelper::GetPlatformFlag(bundleSettings.m_platform);
            AzFramework::PlatformId platformId = static_cast<AzFramework::PlatformId>(AzFramework::PlatformHelper::GetPlatformIndexFromName(bundleSettings.m_platform.c_str()));

            for (const AZStd::string& assetPath : params.m_addSeedList)
            {
                m_assetSeedManager->AddSeedAsset(assetPath, platformFlag);
            }

            auto assetList = m_assetSeedManager->GetDependenciesInfo(platformId, {});
            if (assetList.size() == 0)
            {
                AZ_TracePrintf(AssetBundler::AppWindowName, "Platform ( %s ) had no assets based on these seeds, skipping bundle generation.\n", bundleSettings.m_platform.c_str());
            }
            else
            {
                AssetFileInfoList assetFileInfoList;
                // convert from AZ::Data::AssetInfo to AssetFileInfo for AssetBundleAPI call
                for (const auto& asset : assetList)
                {
                    AssetFileInfo assetInfo;
                    assetInfo.m_assetId = asset.m_assetId;
                    assetInfo.m_assetRelativePath = asset.m_relativePath;
                    assetFileInfoList.m_fileInfoList.emplace_back(assetInfo);
                }

                AZ_TracePrintf(AssetBundler::AppWindowName, "Creating Bundle ( %s )...\n", bundleSettings.m_bundleFilePath.c_str());
                bool result = false;
                AssetBundleCommandsBus::BroadcastResult(result, &AssetBundleCommandsBus::Events::CreateAssetBundleFromList, bundleSettings, assetFileInfoList);
                if (!result)
                {
                    AZ_Error(AssetBundler::AppWindowName, false, "Unable to create bundle, target Bundle file path is ( %s ).", bundleSettings.m_bundleFilePath.c_str());
                    return false;
                }
                AZ_TracePrintf(AssetBundler::AppWindowName, "Bundle ( %s ) created successfully!\n", bundleSettings.m_bundleFilePath.c_str());
            }
        }

        return true;
    }

    AZ::Outcome<void, AZStd::string> ApplicationManager::InitAssetCatalog(AzFramework::PlatformFlags platforms, const AZStd::string& assetCatalogFile)
    {
        using namespace AzToolsFramework;
        if (platforms == AzFramework::PlatformFlags::Platform_NONE)
        {
            return AZ::Failure(AZStd::string("Invalid platform.\n"));
        }

        for (const AzFramework::PlatformId& platformId : AzFramework::PlatformHelper::GetPlatformIndicesInterpreted(platforms))
        {
            AZStd::string platformSpecificAssetCatalogPath;
            if (assetCatalogFile.empty())
            {
                AZ::StringFunc::Path::ConstructFull(
                    PlatformAddressedAssetCatalog::GetAssetRootForPlatform(platformId).c_str(),
                    AssetBundler::AssetCatalogFilename,
                    platformSpecificAssetCatalogPath);
            }
            else
            {
                platformSpecificAssetCatalogPath = assetCatalogFile;
            }

            AZ_TracePrintf(AssetBundler::AppWindowNameVerbose, "Loading asset catalog from ( %s ).\n", platformSpecificAssetCatalogPath.c_str());

            bool success = false;
            {
                AzToolsFramework::AssetCatalog::PlatformAddressedAssetCatalogRequestBus::EventResult(success, platformId, &AzToolsFramework::AssetCatalog::PlatformAddressedAssetCatalogRequestBus::Events::LoadCatalog, platformSpecificAssetCatalogPath.c_str());
            }
            if (!success && !AzFramework::PlatformHelper::IsSpecialPlatform(platforms))
            {
                return AZ::Failure(AZStd::string::format("Failed to open asset catalog file ( %s ).", platformSpecificAssetCatalogPath.c_str()));
            }
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> ApplicationManager::LoadSeedListFile(const AZStd::string& seedListFileAbsolutePath, AzFramework::PlatformFlags platformFlags)
    {
        AZ::Outcome<void, AZStd::string> fileExtensionOutcome = AzToolsFramework::AssetSeedManager::ValidateSeedFileExtension(seedListFileAbsolutePath);
        if (!fileExtensionOutcome.IsSuccess())
        {
            return fileExtensionOutcome;
        }

        bool seedListFileExists = AZ::IO::FileIOBase::GetInstance()->Exists(seedListFileAbsolutePath.c_str());

        if (seedListFileExists)
        {
            AZ_TracePrintf(AssetBundler::AppWindowName, "Loading Seed List file ( %s ).\n", seedListFileAbsolutePath.c_str());

            if (!IsGemSeedFilePathValid(GetEngineRoot(), seedListFileAbsolutePath, m_gemInfoList, platformFlags))
            {
                return AZ::Failure(AZStd::string::format(
                    "Invalid Seed List file ( %s ). This can happen if you add a seed file from a gem that is not enabled for the current project ( %s ).",
                    seedListFileAbsolutePath.c_str(),
                    m_currentProjectName.c_str()));
            }

            if (!m_assetSeedManager->Load(seedListFileAbsolutePath))
            {
                return AZ::Failure(AZStd::string::format("Failed to load Seed List file ( %s ).", seedListFileAbsolutePath.c_str()));
            }
        }

        return AZ::Success();
    }

    void ApplicationManager::PrintSeedList(const AZStd::string& seedListFileAbsolutePath)
    {
        AZ_Printf(AppWindowName, "\nContents of ( %s ):\n\n", seedListFileAbsolutePath.c_str());
        for (const AzFramework::SeedInfo& seed : m_assetSeedManager->GetAssetSeedList())
        {
            AZ_Printf(AppWindowName, "%-60s%s\n", seed.m_assetRelativePath.c_str(), m_assetSeedManager->GetReadablePlatformList(seed).c_str());
        }
        AZ_Printf(AppWindowName, "\n");
    }

    bool ApplicationManager::RunPlatformSpecificAssetListCommands(const AssetListsParams& params, AzFramework::PlatformFlags platformFlags)
    {
        using namespace AzToolsFramework;
        auto platformIds = AzFramework::PlatformHelper::GetPlatformIndices(platformFlags);
        auto platformIdsInterpreted = AzFramework::PlatformHelper::GetPlatformIndicesInterpreted(platformFlags);

        // Add Seeds
        for (const auto& platformId : platformIds)
        {
            AzFramework::PlatformFlags platformFlag = AzFramework::PlatformHelper::GetPlatformFlagFromPlatformIndex(platformId);

            for (const AZStd::string& assetPath : params.m_addSeedList)
            {
                m_assetSeedManager->AddSeedAsset(assetPath, platformFlag);
            }
        }

        AZStd::unordered_set<AZ::Data::AssetId> exclusionList;
        AZStd::vector<AZStd::string> wildcardPatternExclusionList;

        for (const AZStd::string& asset : params.m_skipList)
        {
            // Is input a wildcard pattern?
            if (LooksLikeWildcardPattern(asset))
            {
                wildcardPatternExclusionList.emplace_back(asset);
                continue;
            }

            // Is input a valid asset in the cache?
            AZ::Data::AssetId assetId = m_assetSeedManager->GetAssetIdByPath(asset, platformFlags);
            if (assetId.IsValid())
            {
                exclusionList.emplace(assetId);
            }
        }

        // Print
        bool printExistingFiles = false;
        if (params.m_print)
        {
            printExistingFiles = !params.m_assetListFile.AbsolutePath().empty()
                && params.m_seedListFiles.empty()
                && params.m_addSeedList.empty()
                && !params.m_addDefaultSeedListFiles;
            PrintAssetLists(params, platformIdsInterpreted, printExistingFiles, exclusionList, wildcardPatternExclusionList);
        }

        // Dry Run
        if (params.m_dryRun || params.m_assetListFile.AbsolutePath().empty() || printExistingFiles)
        {
            return true;
        }

        AZ_Printf(AssetBundler::AppWindowName, "\n");

        AZStd::atomic_uint failureCount = 0;

        // Save 
        AZ::parallel_for_each(platformIdsInterpreted.begin(), platformIdsInterpreted.end(), [this, &params, &failureCount, &exclusionList, &wildcardPatternExclusionList](AzFramework::PlatformId platformId)
        {
            AzFramework::PlatformFlags platformFlag = AzFramework::PlatformHelper::GetPlatformFlagFromPlatformIndex(platformId);

            FilePath platformSpecificAssetListFilePath = FilePath(params.m_assetListFile.AbsolutePath(), AzFramework::PlatformHelper::GetPlatformName(platformId));
            AZStd::string assetListFileAbsolutePath = platformSpecificAssetListFilePath.AbsolutePath();

            AZ_TracePrintf(AssetBundler::AppWindowName, "Saving Asset List file to ( %s )...\n", assetListFileAbsolutePath.c_str());

            // Check if we are performing a destructive overwrite that the user did not approve 
            if (!params.m_allowOverwrites && AZ::IO::FileIOBase::GetInstance()->Exists(assetListFileAbsolutePath.c_str()))
            {
                AZ_Error(AssetBundler::AppWindowName, false, "Asset List file ( %s ) already exists, running this command would perform a destructive overwrite.\n\n"
                    "Run your command again with the ( --%s ) arg if you want to save over the existing file.\n", assetListFileAbsolutePath.c_str(), AllowOverwritesFlag);
                failureCount.fetch_add(1, AZStd::memory_order::memory_order_relaxed);
                return;
            }

            // Generate Debug file
            AZStd::string debugListFileAbsolutePath;
            if (params.m_generateDebugFile)
            {
                debugListFileAbsolutePath = assetListFileAbsolutePath;
                AZ::StringFunc::Path::ReplaceExtension(debugListFileAbsolutePath, AssetFileDebugInfoList::GetAssetListDebugFileExtension());
                AZ_TracePrintf(AssetBundler::AppWindowName, "Saving Asset List Debug file to ( %s )...\n", debugListFileAbsolutePath.c_str());
            }

            if (!m_assetSeedManager->SaveAssetFileInfo(assetListFileAbsolutePath, platformFlag, exclusionList, debugListFileAbsolutePath, wildcardPatternExclusionList))
            {
                AZ_Error(AssetBundler::AppWindowName, false, "Unable to save Asset List file to ( %s ).\n", assetListFileAbsolutePath.c_str());
                failureCount.fetch_add(1, AZStd::memory_order::memory_order_relaxed);
                return;
            }

            AZ_TracePrintf(AssetBundler::AppWindowName, "Save successful! ( %s )\n", assetListFileAbsolutePath.c_str());
        });

        return failureCount == 0;
    }

    void ApplicationManager::PrintAssetLists(const AssetListsParams& params, const AZStd::fixed_vector<AzFramework::PlatformId, AzFramework::PlatformId::NumPlatformIds>& platformIds,
        bool printExistingFiles, const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList, const AZStd::vector<AZStd::string>& wildcardPatternExclusionList)
    {
        using namespace AzToolsFramework;

        // The user wants to print the contents of a pre-existing Asset List file *without* modifying it
        if (printExistingFiles)
        {
            AZStd::vector<FilePath> allAssetListFiles = GetAllPlatformSpecificFilesOnDisk(params.m_assetListFile, params.m_platformFlags);

            for (const FilePath& assetListFilePath : allAssetListFiles)
            {
                auto assetFileInfoOutcome = m_assetSeedManager->LoadAssetFileInfo(assetListFilePath.AbsolutePath());
                if (!assetFileInfoOutcome.IsSuccess())
                {
                    AZ_Error(AssetBundler::AppWindowName, false, assetFileInfoOutcome.GetError().c_str());
                }

                AZ_Printf(AssetBundler::AppWindowName, "\nPrinting contents of ( %s ):\n", assetListFilePath.AbsolutePath().c_str());

                for (const AssetFileInfo& assetFileInfo : assetFileInfoOutcome.GetValue().m_fileInfoList)
                {
                    AZ_Printf(AssetBundler::AppWindowName, "- %s\n", assetFileInfo.m_assetRelativePath.c_str());
                }

                AZ_Printf(AssetBundler::AppWindowName, "Total number of assets in ( %s ): %d\n", assetListFilePath.AbsolutePath().c_str(), assetFileInfoOutcome.GetValue().m_fileInfoList.size());
            }
            return;
        }

        // The user wants to print the contents of a recently-modified Asset List file
        for (const AzFramework::PlatformId platformId : platformIds)
        {
            AssetSeedManager::AssetsInfoList assetsInfoList = m_assetSeedManager->GetDependenciesInfo(platformId, exclusionList, nullptr, wildcardPatternExclusionList);

            AZ_Printf(AssetBundler::AppWindowName, "\nPrinting assets for Platform ( %s ):\n", AzFramework::PlatformHelper::GetPlatformName(platformId));

            for (const AZ::Data::AssetInfo& assetInfo : assetsInfoList)
            {
                AZ_Printf(AssetBundler::AppWindowName, "- %s\n", assetInfo.m_relativePath.c_str());
            }

            AZ_Printf(AssetBundler::AppWindowName, "Total number of assets for Platform ( %s ): %d.\n", AzFramework::PlatformHelper::GetPlatformName(platformId), assetsInfoList.size());
        }
    }

    AZStd::vector<FilePath> ApplicationManager::GetAllPlatformSpecificFilesOnDisk(const FilePath& platformIndependentFilePath, AzFramework::PlatformFlags platformFlags)
    {
        using namespace AzToolsFramework;
        AZStd::vector<FilePath> platformSpecificPaths;

        if (platformIndependentFilePath.AbsolutePath().empty())
        {
            return platformSpecificPaths;
        }

        FilePath testFilePath;

        for (AZStd::string_view platformName : AzFramework::PlatformHelper::GetPlatformsInterpreted(platformFlags))
        {
            testFilePath = FilePath(platformIndependentFilePath.AbsolutePath(), platformName);
            if (!testFilePath.AbsolutePath().empty() && AZ::IO::FileIOBase::GetInstance()->Exists(testFilePath.AbsolutePath().c_str()))
            {
                platformSpecificPaths.emplace_back(testFilePath.AbsolutePath());
            }
        }

        return platformSpecificPaths;
    }

    AZ::Outcome<void, AZStd::string> ApplicationManager::ApplyBundleSettingsOverrides(
        AzToolsFramework::AssetBundleSettings& bundleSettings, 
        const AZStd::string& assetListFilePath, 
        const AZStd::string& outputBundleFilePath, 
        int bundleVersion, 
        int maxBundleSize)
    {
        using namespace AzToolsFramework;

        // Asset List file path
        if (!assetListFilePath.empty())
        {
            FilePath platformSpecificPath = FilePath(assetListFilePath, bundleSettings.m_platform);
            if (platformSpecificPath.AbsolutePath().empty())
            {
                return AZ::Failure(AZStd::string::format(
                    "Failed to apply Bundle Settings overrides: ( %s ) is incompatible with input Bundle Settings file.",
                    assetListFilePath.c_str()));
            }
            bundleSettings.m_assetFileInfoListPath = platformSpecificPath.AbsolutePath();
        }

        // Output Bundle file path
        if (!outputBundleFilePath.empty())
        {
            FilePath platformSpecificPath = FilePath(outputBundleFilePath, bundleSettings.m_platform);
            if (platformSpecificPath.AbsolutePath().empty())
            {
                return AZ::Failure(AZStd::string::format(
                    "Failed to apply Bundle Settings overrides: ( %s ) is incompatible with input Bundle Settings file.",
                    outputBundleFilePath.c_str()));
            }
            bundleSettings.m_bundleFilePath = platformSpecificPath.AbsolutePath();
        }

        // Bundle Version
        if (bundleVersion > 0 && bundleVersion <= AzFramework::AssetBundleManifest::CurrentBundleVersion)
        {
            bundleSettings.m_bundleVersion = bundleVersion;
        }

        // Max Bundle Size
        if (maxBundleSize > 0 && maxBundleSize <= AssetBundleSettings::GetMaxBundleSizeInMB())
        {
            bundleSettings.m_maxBundleSizeInMB = maxBundleSize;
        }

        return AZ::Success();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Output Help Text
    ////////////////////////////////////////////////////////////////////////////////////////////

    void ApplicationManager::OutputHelp(CommandType commandType)
    {
        using namespace AssetBundler;

        AZ_Printf(AppWindowName, "This program can be used to create asset bundles that can be used by the runtime to load assets.\n");
        AZ_Printf(AppWindowName, "--%-20s-Displays more detailed output messages.\n\n", VerboseFlag);

        switch (commandType)
        {
        case CommandType::Seeds:
            OutputHelpSeeds();
            break;
        case CommandType::AssetLists:
            OutputHelpAssetLists();
            break;
        case CommandType::ComparisonRules:
            OutputHelpComparisonRules();
            break;
        case CommandType::Compare:
            OutputHelpCompare();
            break;
        case CommandType::BundleSettings:
            OutputHelpBundleSettings();
            break;
        case CommandType::Bundles:
            OutputHelpBundles();
            break;
        case CommandType::BundleSeed:
            OutputHelpBundleSeed();
            break;
        case CommandType::Invalid:

            AZ_Printf(AppWindowName, "Input to this command follows the format: [subCommandName] --exampleArgThatTakesInput exampleInput --exampleFlagThatTakesNoInput\n");
            AZ_Printf(AppWindowName, "    - Example: \"assetLists --assetListFile example.assetlist --addDefaultSeedListFiles --print\"\n");
            AZ_Printf(AppWindowName, "\n");
            AZ_Printf(AppWindowName, "Some args in this tool take paths as arguments, and there are two main types:\n");
            AZ_Printf(AppWindowName, "          \"path\" - This refers to an Engine-Root-Relative path.\n");
            AZ_Printf(AppWindowName, "                 - Example: \"C:\\O3DE\\dev\\SamplesProject\\test.txt\" can be represented as \"SamplesProject\\test.txt\".\n");
            AZ_Printf(AppWindowName, "    \"cache path\" - This refers to a Cache-Relative path.\n");
            AZ_Printf(AppWindowName, "                 - Example: \"C:\\O3DE\\dev\\Cache\\SamplesProject\\pc\\samplesproject\\animations\\skeletonlist.xml\" is represented as \"animations\\skeletonlist.xml\".\n");
            AZ_Printf(AppWindowName, "\n");

            OutputHelpSeeds();
            OutputHelpAssetLists();
            OutputHelpComparisonRules();
            OutputHelpCompare();
            OutputHelpBundleSettings();
            OutputHelpBundles();
            OutputHelpBundleSeed();

            AZ_Printf(AppWindowName, "\n\nTo see less Help text, type in a Sub-Command before requesting the Help text. For example: \"%s --%s\".\n", SeedsCommand, HelpFlag);

            break;
        }

        if (commandType != CommandType::Invalid)
        {
            AZ_Printf(AppWindowName, "\n\nTo see more Help text, type: \"--%s\" without any other input.\n", HelpFlag);
        }
    }

    void ApplicationManager::OutputHelpSeeds()
    {
        using namespace AzToolsFramework;
        AZ_Printf(AppWindowName, "\n%-25s-Subcommand for performing operations on Seed List files.\n", SeedsCommand);
        AZ_Printf(AppWindowName, "    --%-25s-[Required] Specifies the Seed List file to operate on by path. Must include (.%s) file extension.\n", SeedListFileArg, AssetSeedManager::GetSeedFileExtension());
        AZ_Printf(AppWindowName, "    --%-25s-Adds the asset to the list of root assets for the specified platform.\n", AddSeedArg);
        AZ_Printf(AppWindowName, "%-31s---Takes in a cache path to a pre-processed asset.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Removes the asset from the list of root assets for the specified platform.\n", RemoveSeedArg);
        AZ_Printf(AppWindowName, "%-31s---To completely remove the asset, it must be removed for all platforms.\n", "");
        AZ_Printf(AppWindowName, "%-31s---Takes in a cache path to a pre-processed asset. A cache path is a path relative to \"ProjectPath\\Cache\\platform\\\"\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Adds the specified platform to every Seed in the Seed List file, if possible.\n", AddPlatformToAllSeedsFlag);
        AZ_Printf(AppWindowName, "    --%-25s-Removes the specified platform from every Seed in the Seed List file, if possible.\n", RemovePlatformFromAllSeedsFlag);
        AZ_Printf(AppWindowName, "    --%-25s-Outputs the contents of the Seed List file after performing any specified operations.\n", PrintFlag);
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the platform(s) referenced by all Seed operations.\n", PlatformArg);
        AZ_Printf(AppWindowName, "%-31s---Requires an existing cache of assets for the input platform(s).\n", "");
        AZ_Printf(AppWindowName, "%-31s---Defaults to all enabled platforms. Platforms can be changed by modifying AssetProcessorPlatformConfig.setreg.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Updates the path hints stored in the Seed List file.\n", UpdateSeedPathArg);
        AZ_Printf(AppWindowName, "    --%-25s-Removes the path hints stored in the Seed List file.\n", RemoveSeedPathArg);
        AZ_Printf(AppWindowName, "    --%-25s-Allows input file path to still match if the file path case is different than on disk.\n", IgnoreFileCaseFlag);
        AZ_Printf(AppWindowName, "    --%-25s-[Testing] Specifies the Asset Catalog file referenced by all Seed operations.\n", AssetCatalogFileArg);
        AZ_Printf(AppWindowName, "%-31s---Designed to be used in Unit Tests.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the game project to use rather than the current default project set in bootstrap.cfg's project_path.\n", ProjectArg);
    }

    void ApplicationManager::OutputHelpAssetLists()
    {
        using namespace AzToolsFramework;
        AZ_Printf(AppWindowName, "\n%-25s-Subcommand for generating Asset List Files.\n", AssetListsCommand);
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the Asset List file to operate on by path. Must include (.%s) file extension.\n", AssetListFileArg, AssetSeedManager::GetAssetListFileExtension());
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the Seed List file(s) that will be used as root(s) when generating this Asset List file.\n", SeedListFileArg);
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the Seed(s) to use as root(s) when generating this Asset List File.\n", AddSeedArg);
        AZ_Printf(AppWindowName, "%-31s---Takes in a cache path to a pre-processed asset. A cache path is a path relative to \"ProjectPath\\Cache\\platform\\\"\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-The specified files and all dependencies will be ignored when generating the Asset List file.\n", SkipArg);
        AZ_Printf(AppWindowName, "%-31s---Takes in a comma-separated list of either: cache paths to pre-processed assets, or wildcard patterns.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Automatically include all default Seed List files in generated Asset List File.\n", AddDefaultSeedListFilesFlag);
        AZ_Printf(AppWindowName, "%-31s---This will include Seed List files for the Open 3D Engine Engine and all enabled Gems.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the platform(s) to generate an Asset List file for.\n", PlatformArg);
        AZ_Printf(AppWindowName, "%-31s---Requires an existing cache of assets for the input platform(s).\n", "");
        AZ_Printf(AppWindowName, "%-31s---Defaults to all enabled platforms. Platforms can be changed by modifying AssetProcessorPlatformConfig.setreg.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-[Testing] Specifies the Asset Catalog file referenced by all Asset List operations.\n", AssetCatalogFileArg);
        AZ_Printf(AppWindowName, "%-31s---Designed to be used in Unit Tests.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Outputs the contents of the Asset List file after adding any specified seed files.\n", PrintFlag);
        AZ_Printf(AppWindowName, "    --%-25s-Run all input commands, without saving to the specified Asset List file.\n", DryRunFlag);
        AZ_Printf(AppWindowName, "    --%-25s-Generates a human-readable file that maps every entry in the Asset List file to the Seed that generated it.\n", GenerateDebugFileFlag);
        AZ_Printf(AppWindowName, "    --%-25s-Allow destructive overwrites of files. Include this arg in automation.\n", AllowOverwritesFlag);
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the game project to use rather than the current default project set in bootstrap.cfg's project_path.\n", ProjectArg);
    }

    void ApplicationManager::OutputHelpComparisonRules()
    {
        using namespace AzToolsFramework;
        AZ_Printf(AppWindowName, "\n%-25s-Subcommand for generating Comparison Rules files.\n", ComparisonRulesCommand);
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the Comparison Rules file to operate on by path.\n", ComparisonRulesFileArg);
        AZ_Printf(AppWindowName, "    --%-25s-Adds a Comparison Step to the given Comparison Rules file at the specified line number.\n", AddComparisonStepArg);
        AZ_Printf(AppWindowName, "%-31s---Takes in a non-negative integer. If no input is supplied, the Comparison Step will be added to the end.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Removes the Comparison Step present at the input line number from the given Comparison Rules file.\n", RemoveComparisonStepArg);
        AZ_Printf(AppWindowName, "    --%-25s-Moves a Comparison Step from one line number to another line number in the given Comparison Rules file.\n", MoveComparisonStepArg);
        AZ_Printf(AppWindowName, "%-31s---Takes in a comma-separated pair of non-negative integers: the original line number and the destination line number.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Edits the Comparison Step at the input line number using values from other input arguments.\n", EditComparisonStepArg);
        AZ_Printf(AppWindowName, "%-31s---When editing, other input arguments may only contain one input value.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-A comma seperated list of Comparison types.\n", ComparisonTypeArg);
        AZ_Printf(AppWindowName, "%-31s---Valid inputs: 0 (Delta), 1 (Union), 2 (Intersection), 3 (Complement), 4 (FilePattern).\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-A comma seperated list of file pattern matching types.\n", ComparisonFilePatternTypeArg);
        AZ_Printf(AppWindowName, "%-31s---Valid inputs: 0 (Wildcard), 1 (Regex).\n", "");
        AZ_Printf(AppWindowName, "%-31s---Must match the number of FilePattern comparisons specified in ( --%s ) argument list.\n", "", ComparisonTypeArg);
        AZ_Printf(AppWindowName, "    --%-25s-A comma seperated list of file patterns.\n", ComparisonFilePatternArg);
        AZ_Printf(AppWindowName, "%-31s---Must match the number of FilePattern comparisons specified in ( --%s ) argument list.\n", "", ComparisonTypeArg);
        AZ_Printf(AppWindowName, "    --%-25s-A comma seperated list of output Token names.\n", ComparisonTokenNameArg);
        AZ_Printf(AppWindowName, "    --%-25s-The Token name of the Comparison Step you wish to use as the first input of this Comparison Step.\n", ComparisonFirstInputArg);
        AZ_Printf(AppWindowName, "    --%-25s-The Token name of the Comparison Step you wish to use as the second input of this Comparison Step.\n", ComparisonSecondInputArg);
        AZ_Printf(AppWindowName, "%-31s---Comparison Steps of the ( FilePattern ) type only accept one input Token, and cannot be used with this arg.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Outputs the contents of the Comparison Rules file after performing any specified operations.\n", PrintFlag);
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the game project to use rather than the current default project set in bootstrap.cfg's project_path.\n", ProjectArg);
    }

    void ApplicationManager::OutputHelpCompare()
    {
        using namespace AzToolsFramework;
        AZ_Printf(AppWindowName, "\n%-25s-Subcommand for performing comparisons between asset list files.\n", CompareCommand);
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the Comparison Rules file to load rules from.\n", ComparisonRulesFileArg);
        AZ_Printf(AppWindowName, "%-31s---When entering input and output values, input the single '$' character to use the default values defined in the file.\n", "");
        AZ_Printf(AppWindowName, "%-31s---All additional comparison rules specified in this command will be done after the comparison operations loaded from the rules file.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-A comma seperated list of comparison types.\n", ComparisonTypeArg);
        AZ_Printf(AppWindowName, "%-31s---Valid inputs: 0 (Delta), 1 (Union), 2 (Intersection), 3 (Complement), 4 (FilePattern), 5 (IntersectionCount).\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-A comma seperated list of file pattern matching types.\n", ComparisonFilePatternTypeArg);
        AZ_Printf(AppWindowName, "%-31s---Valid inputs: 0 (Wildcard), 1 (Regex).\n", "");
        AZ_Printf(AppWindowName, "%-31s---Must match the number of FilePattern comparisons specified in ( --%s ) argument list.\n", "", ComparisonTypeArg);
        AZ_Printf(AppWindowName, "    --%-25s-A comma seperated list of file patterns.\n", ComparisonFilePatternArg);
        AZ_Printf(AppWindowName, "%-31s---Must match the number of FilePattern comparisons specified in ( --%s ) argument list.\n", "", ComparisonTypeArg);
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the count that will be used during the %s compare operation.\n", IntersectionCountArg, AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(AssetFileInfoListComparison::ComparisonType::IntersectionCount)]);
        AZ_Printf(AppWindowName, "    --%-25s-A comma seperated list of first inputs for comparison.\n", CompareFirstFileArg);
        AZ_Printf(AppWindowName, "%-31s---Must match the number of comparison operations.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-A comma seperated list of second inputs for comparison.\n", CompareSecondFileArg);
        AZ_Printf(AppWindowName, "%-31s---Must match the number of comparison operations that require two inputs.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-A comma seperated list of outputs for the comparison command.\n", CompareOutputFileArg);
        AZ_Printf(AppWindowName, "%-31s---Must match the number of comparison operations.\n", "");
        AZ_Printf(AppWindowName, "%-31s---Inputs and outputs can be a file or a variable passed from another comparison.\n", "");
        AZ_Printf(AppWindowName, "%-31s---Variables are specified by the prefix %c.\n", "", compareVariablePrefix);
        AZ_Printf(AppWindowName, "    --%-25s-A comma seperated list of paths or variables to print to console after comparison operations complete.\n", ComparePrintArg);
        AZ_Printf(AppWindowName, "%-31s---Leave list blank to just print the final comparison result.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the platform(s) referenced when determining which Asset List files to compare.\n", PlatformArg);
        AZ_Printf(AppWindowName, "%-31s---All input Asset List files must exist for all specified platforms\n", "");
        AZ_Printf(AppWindowName, "%-31s---Defaults to all enabled platforms. Platforms can be changed by modifying AssetProcessorPlatformConfig.setreg.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Allow destructive overwrites of files. Include this arg in automation.\n", AllowOverwritesFlag);
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the game project to use rather than the current default project set in bootstrap.cfg's project_path.\n", ProjectArg);
    }

    void ApplicationManager::OutputHelpBundleSettings()
    {
        using namespace AzToolsFramework;
        AZ_Printf(AppWindowName, "\n%-25s-Subcommand for performing operations on Bundle Settings files.\n", BundleSettingsCommand);
        AZ_Printf(AppWindowName, "    --%-25s-[Required] Specifies the Bundle Settings file to operate on by path. Must include (.%s) file extension.\n", BundleSettingsFileArg, AssetBundleSettings::GetBundleSettingsFileExtension());
        AZ_Printf(AppWindowName, "    --%-25s-Sets the Asset List file to use for Bundle generation. Must include (.%s) file extension.\n", AssetListFileArg, AssetSeedManager::GetAssetListFileExtension());
        AZ_Printf(AppWindowName, "    --%-25s-Sets the path where generated Bundles will be stored. Must include (.%s) file extension.\n", OutputBundlePathArg, AssetBundleSettings::GetBundleFileExtension());
        AZ_Printf(AppWindowName, "    --%-25s-Determines which version of Open 3D Engine Bundles to generate. Current version is (%i).\n", BundleVersionArg, AzFramework::AssetBundleManifest::CurrentBundleVersion);
        AZ_Printf(AppWindowName, "    --%-25s-Sets the maximum size for a single Bundle (in MB). Default size is (%i MB).\n", MaxBundleSizeArg, AssetBundleSettings::GetMaxBundleSizeInMB());
        AZ_Printf(AppWindowName, "%-31s---Bundles larger than this limit will be divided into a series of smaller Bundles and named accordingly.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the platform(s) referenced by all Bundle Settings operations.\n", PlatformArg);
        AZ_Printf(AppWindowName, "%-31s---Defaults to all enabled platforms. Platforms can be changed by modifying AssetProcessorPlatformConfig.setreg.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Outputs the contents of the Bundle Settings file after modifying any specified values.\n", PrintFlag);
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the game project to use rather than the current default project set in bootstrap.cfg's project_path.\n", ProjectArg);
    }

    void ApplicationManager::OutputHelpBundles()
    {
        using namespace AzToolsFramework;
        AZ_Printf(AppWindowName, "\n%-25s-Subcommand for generating bundles. Must provide either (--%s) or (--%s and --%s).\n", BundlesCommand, BundleSettingsFileArg, AssetListFileArg, OutputBundlePathArg);
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the Bundle Settings files to operate on by path. Must include (.%s) file extension.\n", BundleSettingsFileArg, AssetBundleSettings::GetBundleSettingsFileExtension());
        AZ_Printf(AppWindowName, "%-31s---If any other args are specified, they will override the values stored inside this file.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Sets the Asset List files to use for Bundle generation. Must include (.%s) file extension.\n", AssetListFileArg, AssetSeedManager::GetAssetListFileExtension());
        AZ_Printf(AppWindowName, "    --%-25s-Sets the paths where generated Bundles will be stored. Must include (.%s) file extension.\n", OutputBundlePathArg, AssetBundleSettings::GetBundleFileExtension());
        AZ_Printf(AppWindowName, "    --%-25s-Determines which versions of Open 3D Engine Bundles to generate. Current version is (%i).\n", BundleVersionArg, AzFramework::AssetBundleManifest::CurrentBundleVersion);
        AZ_Printf(AppWindowName, "    --%-25s-Sets the maximum size for Bundles (in MB). Default size is (%i MB).\n", MaxBundleSizeArg, AssetBundleSettings::GetMaxBundleSizeInMB());
        AZ_Printf(AppWindowName, "%-31s---Bundles larger than this limit will be divided into a series of smaller Bundles and named accordingly.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the platform(s) that will be referenced when generating Bundles.\n", PlatformArg);
        AZ_Printf(AppWindowName, "%-31s---If no platforms are specified, Bundles will be generated for all available platforms.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Allow destructive overwrites of files. Include this arg in automation.\n", AllowOverwritesFlag);
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the game project to use rather than the current default project set in bootstrap.cfg's project_path.\n", ProjectArg);
    }

    void ApplicationManager::OutputHelpBundleSeed()
    {
        using namespace AzToolsFramework;
        AZ_Printf(AppWindowName, "\n%-25s-Subcommand for generating bundles directly from seeds. Must provide either (--%s) or (--%s).\n", BundleSeedCommand, BundleSettingsFileArg, OutputBundlePathArg);
        AZ_Printf(AppWindowName, "    --%-25s-Adds the asset to the list of root assets for the specified platform.\n", AddSeedArg);
        AZ_Printf(AppWindowName, "%-31s---Takes in a cache path to a pre-processed asset. A cache path is a path relative to \"ProjectPath\\Cache\\platform\\\"\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the Bundle Settings file to operate on by path. Must include (.%s) file extension.\n", BundleSettingsFileArg, AssetBundleSettings::GetBundleSettingsFileExtension());
        AZ_Printf(AppWindowName, "    --%-25s-Sets the path where generated Bundles will be stored. Must include (.%s) file extension.\n", OutputBundlePathArg, AssetBundleSettings::GetBundleFileExtension());
        AZ_Printf(AppWindowName, "    --%-25s-Determines which version of Open 3D Engine Bundles to generate. Current version is (%i).\n", BundleVersionArg, AzFramework::AssetBundleManifest::CurrentBundleVersion);
        AZ_Printf(AppWindowName, "    --%-25s-Sets the maximum size for a single Bundle (in MB). Default size is (%i MB).\n", MaxBundleSizeArg, AssetBundleSettings::GetMaxBundleSizeInMB());
        AZ_Printf(AppWindowName, "%-31s---Bundles larger than this limit will be divided into a series of smaller Bundles and named accordingly.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the platform(s) that will be referenced when generating Bundles.\n", PlatformArg);
        AZ_Printf(AppWindowName, "%-31s---If no platforms are specified, Bundles will be generated for all available platforms.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Allow destructive overwrites of files. Include this arg in automation.\n", AllowOverwritesFlag);
        AZ_Printf(AppWindowName, "    --%-25s-[Testing] Specifies the Asset Catalog file referenced by all Bundle operations.\n", AssetCatalogFileArg);
        AZ_Printf(AppWindowName, "%-31s---Designed to be used in Unit Tests.\n", "");
        AZ_Printf(AppWindowName, "    --%-25s-Specifies the game project to use rather than the current default project set in bootstrap.cfg's project_path.\n", ProjectArg);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Formatting for Output Text
    ////////////////////////////////////////////////////////////////////////////////////////////

    bool ApplicationManager::OnPreError(const char* window, const char* fileName, int line, [[maybe_unused]] const char* func, const char* message)
    {
        printf("\n");
        printf("[ERROR] - %s:\n", window);

        if (m_showVerboseOutput)
        {
            printf("(%s - Line %i)\n", fileName, line);
        }

        printf("%s", message);
        printf("\n");
        return true;
    }

    bool ApplicationManager::OnPreWarning(const char* window, const char* fileName, int line, [[maybe_unused]] const char* func, const char* message)
    {
        printf("\n");
        printf("[WARN] - %s:\n", window);

        if (m_showVerboseOutput)
        {
            printf("(%s - Line %i)\n", fileName, line);
        }

        printf("%s", message);
        printf("\n");
        return true;
    }

    bool ApplicationManager::OnPrintf(const char* window, const char* message)
    {
        if (window == AssetBundler::AppWindowName || (m_showVerboseOutput && window == AssetBundler::AppWindowNameVerbose))
        {
            printf("%s", message);
            return true;
        }

        return !m_showVerboseOutput;
    }
} // namespace AssetBundler
#include <source/utils/moc_applicationManager.cpp>
