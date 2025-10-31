/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderApplication.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/IO/IStreamer.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderComponent.h>
#include <AssetBuilderInfo.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <ToolsComponents/ToolsAssetCatalogComponent.h>
#include <AssetBuilderStatic.h>

// Command-line parameter options:
static const char* const s_paramHelp = "help"; // Print help information.
static const char* const s_paramTask = "task"; // Task to run.
static const char* const s_paramProjectName = "project-name"; // Name of the current project.
static const char* const s_paramProjectCacheRoot = "project-cache-path"; // Full path to the project cache folder.
static const char* const s_paramModule = "module"; // For resident mode, the path to the builder dll folder, otherwise the full path to a single builder dll to use.
static const char* const s_paramPort = "port"; // Optional, port number to use to connect to the AP.
static const char* const s_paramIp = "remoteip"; // optional, IP address to use to connect to the AP
static const char* const s_paramId = "id"; // UUID string that identifies the builder.  Only used for resident mode when the AP directly starts up the AssetBuilder.
static const char* const s_paramInput = "input"; // For non-resident mode, full path to the file containing the serialized job request.
static const char* const s_paramOutput = "output"; // For non-resident mode, full path to the file to write the job response to.
static const char* const s_paramDebug = "debug"; // Debug mode for the create and process job of the specified file.
static const char* const s_paramDebugCreate = "debug_create"; // Debug mode for the create job of the specified file.
static const char* const s_paramDebugProcess = "debug_process"; // Debug mode for the process job of the specified file.
static const char* const s_paramPlatformTags = "tags"; // Additional list of tags to add platform tag list.
static const char* const s_paramPlatform = "platform"; // Platform to use
static const char* const s_paramRegisterBuilders = "register"; // Indicates the AP is starting up and requesting a list of registered builders

// Task modes:
static const char* const s_taskResident = "resident"; // stays up and running indefinitely, accepting jobs via network connection
static const char* const s_taskCreateJob = "create"; // runs a builders createJobs function
static const char* const s_taskProcessJob = "process"; // runs processJob function
static const char* const s_taskDebug = "debug"; // runs a one shot job in a fake environment for a specified file.
static const char* const s_taskDebugCreate = "debug_create"; // runs a one shot job in a fake environment for a specified file.
static const char* const s_taskDebugProcess = "debug_process"; // runs a one shot job in a fake environment for a specified file.

//! Scoped Setters for the SettingsRegistry to its previous value on destruction
struct ScopedSettingsRegistrySetter
{
    using SettingsRegistrySetterTypes = AZStd::variant<bool, AZ::s64, AZ::u64, double, AZStd::string_view>;
    using SettingsRegistryGetterTypes = AZStd::variant<bool, AZ::s64, AZ::u64, double, AZStd::string>;

    ScopedSettingsRegistrySetter(AZ::SettingsRegistryInterface& settingsRegistry, AZStd::string_view jsonPointer,
        SettingsRegistrySetterTypes newValue)
        : m_settingsRegistry(settingsRegistry)
        , m_jsonPointer(jsonPointer)
    {
        AZStd::string oldValue;
        if (m_settingsRegistry.Get(oldValue, jsonPointer))
        {
            m_oldValue = AZStd::move(oldValue);
        }

        AZStd::visit([this](auto&& value) {m_settingsRegistry.Set(m_jsonPointer, AZStd::move(value)); }, AZStd::move(newValue));
    }

    ~ScopedSettingsRegistrySetter()
    {
        // Reset the old value within the Settings Registry if it was set
        // Or remove it if not
        if (m_oldValue)
        {
            AZStd::visit([this](auto&& value) {m_settingsRegistry.Set(m_jsonPointer, AZStd::move(value)); }, AZStd::move(*m_oldValue));
        }
        else
        {
            m_settingsRegistry.Remove(m_jsonPointer);
        }
    }
    AZ::SettingsRegistryInterface& m_settingsRegistry;
    AZStd::string_view m_jsonPointer;
    AZStd::optional<SettingsRegistryGetterTypes> m_oldValue;
};

//! FileIO classes which resets the set key to its previous value on destruction
struct ScopedAliasSetter
{
    ScopedAliasSetter(AZ::IO::FileIOBase& fileIoBase, const char* alias,
        const char* newValue)
        : m_fileIoBase(fileIoBase)
        , m_alias(alias)
    {

        if (const char* oldValue = m_fileIoBase.GetAlias(m_alias); oldValue != nullptr)
        {
            m_oldValue = oldValue;
        }

        m_fileIoBase.SetAlias(alias, newValue);
    }

    ~ScopedAliasSetter()
    {
        // Reset the old alias if it was set or clear it if not
        if (m_oldValue)
        {
            m_fileIoBase.SetAlias(m_alias, m_oldValue->c_str());
        }
        else
        {
            m_fileIoBase.ClearAlias(m_alias);
        }
    }
    AZ::IO::FileIOBase& m_fileIoBase;
    const char* m_alias;
    AZStd::optional<AZStd::string> m_oldValue;
};

//////////////////////////////////////////////////////////////////////////

void AssetBuilderComponent::PrintHelp()
{
    AZ_TracePrintf("Help", "\nAssetBuilder is part of the Asset Processor so tasks are run in an isolated environment.\n");
    AZ_TracePrintf("Help", "The following command line options are available for the AssetBuilder.\n");
    AZ_TracePrintf("Help", "%s - Print help information.\n", s_paramHelp);
    AZ_TracePrintf("Help", "%s - Task to run.\n", s_paramTask);
    AZ_TracePrintf("Help", "%s - Name of the current project.\n", s_paramProjectName);
    AZ_TracePrintf("Help", "%s - Full path to the project cache folder.\n", s_paramProjectCacheRoot);
    AZ_TracePrintf("Help", "%s - For resident mode, the path to the builder dll folder, otherwise the full path to a single builder dll to use.\n", s_paramModule);
    AZ_TracePrintf("Help", "%s - Optional, port number to use to connect to the AP.\n", s_paramPort);
    AZ_TracePrintf("Help", "%s - UUID string that identifies the builder.  Only used for resident mode when the AP directly starts up the AssetBuilder.\n", s_paramId);
    AZ_TracePrintf("Help", "%s - For non-resident mode, full path to the file containing the serialized job request.\n", s_paramInput);
    AZ_TracePrintf("Help", "%s - For non-resident mode, full path to the file to write the job response to.\n", s_paramOutput);
    AZ_TracePrintf("Help", "%s - Debug mode for the create and process job of the specified file.\n", s_paramDebug);
    AZ_TracePrintf("Help", "  Debug mode optionally uses -%s, -%s, -%s, -%s and -gameroot.\n", s_paramInput, s_paramOutput, s_paramModule, s_paramPort);
    AZ_TracePrintf("Help", "  Example: -%s Objects\\Tutorials\\shapes.fbx\n", s_paramDebug);
    AZ_TracePrintf("Help", "%s - Debug mode for the create job of the specified file.\n", s_paramDebugCreate);
    AZ_TracePrintf("Help", "%s - Debug mode for the process job of the specified file.\n", s_paramDebugProcess);
    AZ_TracePrintf("Help", "%s - Additional tags to add to the debug platform for job processing. One tag can be supplied per option\n", s_paramPlatformTags);
    AZ_TracePrintf("Help", "%s - Platform to use for debugging. ex: pc\n", s_paramPlatform);
}

bool AssetBuilderComponent::IsInDebugMode(const AzFramework::CommandLine& commandLine)
{
    if (commandLine.HasSwitch(s_paramDebug) || commandLine.HasSwitch(s_paramDebugCreate) || commandLine.HasSwitch(s_paramDebugProcess))
    {
        return true;
    }

    if (commandLine.HasSwitch(s_paramTask))
    {
        const AZStd::string& task = commandLine.GetSwitchValue(s_paramTask, 0);
        if (task == s_taskDebug || task == s_taskDebugCreate || task == s_taskDebugProcess)
        {
            return true;
        }
    }

    return false;
}


void AssetBuilderComponent::Activate()
{
    BuilderBus::Handler::BusConnect();
    AssetBuilderSDK::AssetBuilderBus::Handler::BusConnect();
    AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusConnect();

    // the asset builder app never writes source files, only assets, so there is no need to do any kind of asset upgrading
    AZ::Data::AssetManager::Instance().SetAssetInfoUpgradingEnabled(false);
}

void AssetBuilderComponent::Deactivate()
{
    BuilderBus::Handler::BusDisconnect();
    AssetBuilderSDK::AssetBuilderBus::Handler::BusDisconnect();
    AzFramework::EngineConnectionEvents::Bus::Handler::BusDisconnect();
    AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusDisconnect();
}

void AssetBuilderComponent::Reflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<AssetBuilderComponent, AZ::Component>()
            ->Version(1);
    }
}

bool AssetBuilderComponent::DoHelloPing()
{
    using namespace AssetBuilder;

    BuilderHelloRequest request;
    BuilderHelloResponse response;

    AZStd::string id;

    if (!GetParameter(s_paramId, id))
    {
        return false;
    }

    request.m_uuid = AZ::Uuid::CreateString(id.c_str());

    AZ_TracePrintf(
        "AssetBuilderComponent", "RunInResidentMode: Pinging asset processor with the builder UUID %s\n",
        request.m_uuid.ToString<AZStd::string>().c_str());

    bool result = AzFramework::AssetSystem::SendRequest(request, response);

    AZ_Error("AssetBuilder", result, "Failed to send hello request to Asset Processor");
    // This error is only shown if we successfully got a response AND the response explicitly indicates the AP rejected the builder
    AZ_Error("AssetBuilder", !result || response.m_accepted, "Asset Processor rejected connection request");

    if (result)
    {
        AZ_TracePrintf("AssetBuilder", "Builder ID: %s\n", response.m_uuid.ToString<AZStd::string>().c_str());
    }

    return result;
}

bool AssetBuilderComponent::Run()
{
    AZ_TracePrintf("AssetBuilderComponent", "Run:  Parsing command line.\n");
    const AzFramework::CommandLine* commandLine = nullptr;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);
    if (commandLine->HasSwitch(s_paramHelp))
    {
        PrintHelp();
        UnloadBuilders();
        return true;
    }

    AZStd::string task;
    AZStd::string debugFile;

    if (GetParameter(s_paramDebug, debugFile, false))
    {
        task = s_taskDebug;
    }
    else if (GetParameter(s_paramDebugCreate, debugFile, false))
    {
        task = s_taskDebugCreate;
    }
    else if (GetParameter(s_paramDebugProcess, debugFile, false))
    {
        task = s_taskDebugProcess;
    }
    else if (!GetParameter(s_paramTask, task))
    {
        AZ_Error("AssetBuilder", false, "No task specified. Use -help for options.");
        UnloadBuilders();
        return false;
    }

    bool isDebugTask = (task == s_taskDebug || task == s_taskDebugCreate || task == s_taskDebugProcess);
    if (!GetParameter(s_paramProjectName, m_gameName, !isDebugTask))
    {
        m_gameName = AZ::Utils::GetProjectName();
    }

    if (!GetParameter(s_paramProjectCacheRoot, m_gameCache, !isDebugTask))
    {
        if (!isDebugTask)
        {
            UnloadBuilders();
            return false;
        }
    }

    AZ_TracePrintf("AssetBuilderComponent", "Run: Connecting back to Asset Processor...\n");
    bool connectedToAssetProcessor = ConnectToAssetProcessor();
    //AP connection is required to access the asset catalog
    AZ_Error("AssetBuilder", connectedToAssetProcessor, "Failed to establish a network connection to the AssetProcessor. Use -help for options.");

    bool registerBuilders = commandLine->GetNumSwitchValues(s_paramRegisterBuilders) > 0;

    IBuilderApplication* builderApplication = AZ::Interface<IBuilderApplication>::Get();

    if (!builderApplication)
    {
        AZ_Error("AssetBuilder", false, "Failed to retreive IBuilderApplication interface");
        return false;
    }

    builderApplication->InitializeBuilderComponents();

    bool result = false;

    if (connectedToAssetProcessor)
    {
        if (task == s_taskResident)
        {
            result = RunInResidentMode(registerBuilders);
        }
        else if (task == s_taskDebug)
        {
            static const bool runCreateJobs = true;
            static const bool runProcessJob = true;
            result = RunDebugTask(AZStd::move(debugFile), runCreateJobs, runProcessJob);
        }
        else if (task == s_taskDebugCreate)
        {
            static const bool runCreateJobs = true;
            static const bool runProcessJob = false;
            result = RunDebugTask(AZStd::move(debugFile), runCreateJobs, runProcessJob);
        }
        else if (task == s_taskDebugProcess)
        {
            static const bool runCreateJobs = false;
            static const bool runProcessJob = true;
            result = RunDebugTask(AZStd::move(debugFile), runCreateJobs, runProcessJob);
        }
        else
        {
            result = RunOneShotTask(task);
        }
    }

    // note that we destroy (unload) the builder dlls soon after this (see UnloadBuilders() below),
    // so we must tick here before that occurs.
    // ticking here causes assets that have a 0 refcount (and are thus in the destroy list) to actually be destroyed.
    AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);

    AZ_Error("AssetBuilder", result, "Failed to handle `%s` request", task.c_str());

    AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::StartDisconnectingAssetProcessor);

    UnloadBuilders();

    return result;
}

bool AssetBuilderComponent::ConnectToAssetProcessor()
{
    //get the asset processor connection params from the bootstrap
    AzFramework::AssetSystem::ConnectionSettings connectionSettings;
    bool succeeded = AzFramework::AssetSystem::ReadConnectionSettingsFromSettingsRegistry(connectionSettings);
    if (!succeeded)
    {
        AZ_Error("Asset Builder", false, "Getting bootstrap params failed");
        return false;
    }

    //override bootstrap params
    //the asset builder may have been given an optional ip to use
    AZStd::string overrideIp;
    if (GetParameter(s_paramIp, overrideIp, false))
    {
        connectionSettings.m_assetProcessorIp = overrideIp;
    }

    //the asset builder may have been given an optional port to use
    AZStd::string overridePort;
    if (GetParameter(s_paramPort, overridePort, false))
    {
        connectionSettings.m_assetProcessorPort = static_cast<AZ::u16>(AZStd::stoi(overridePort));
    }

    //the asset builder may have been given an optional asset platform to use
    AZStd::string overrideAssetPlatform;
    if (GetParameter(s_paramPlatform, overrideAssetPlatform, false))
    {
        connectionSettings.m_assetPlatform = overrideAssetPlatform;
    }

    //the asset builder may have been given an optional project name to use
    AZStd::string overrideProjectName;
    if (GetParameter(s_paramProjectName, overrideProjectName, false))
    {
        connectionSettings.m_projectName = overrideProjectName;
    }

    connectionSettings.m_connectionIdentifier = "Asset Builder";
    connectionSettings.m_connectionDirection = AzFramework::AssetSystem::ConnectionSettings::ConnectionDirection::ConnectToAssetProcessor;
    connectionSettings.m_launchAssetProcessorOnFailedConnection = false; // builders shouldn't launch the AssetProcessor
    connectionSettings.m_waitUntilAssetProcessorIsReady = false; // builders are what make the AssetProcessor ready, so the cannot wait until the AssetProcessor is ready
    connectionSettings.m_waitForConnect = true; // application is a builder so it needs to wait for a connection

    //connect to Asset Processor.
    bool connectedToAssetProcessor = false;
    AzFramework::AssetSystemRequestBus::BroadcastResult(connectedToAssetProcessor,
        &AzFramework::AssetSystemRequestBus::Events::EstablishAssetProcessorConnection, connectionSettings);

    return connectedToAssetProcessor;
}

//////////////////////////////////////////////////////////////////////////

bool AssetBuilderComponent::SendRegisteredBuildersToAp()
{
    AssetBuilder::BuilderRegistrationRequest registrationRequest;

    for (const auto& [uuid, desc] : m_assetBuilderDescMap)
    {
        AssetBuilder::BuilderRegistration registration;

        registration.m_name = desc->m_name;
        registration.m_analysisFingerprint = desc->m_analysisFingerprint;
        registration.m_flags = desc->m_flags;
        registration.m_flagsByJobKey = desc->m_flagsByJobKey;
        registration.m_version = desc->m_version;
        registration.m_busId = desc->m_busId;
        registration.m_patterns = desc->m_patterns;
        registration.m_productsToKeepOnFailure = desc->m_productsToKeepOnFailure;

        registrationRequest.m_builders.push_back(AZStd::move(registration));
    }

    bool result = SendRequest(registrationRequest);

    AZ_Error("AssetBuilder", result, "Failed to send builder registration request to Asset Processor");

    return result;
}

bool AssetBuilderComponent::RunInResidentMode(bool sendRegistration)
{
    using namespace AssetBuilder;
    using namespace AZStd::placeholders;

    AZ_TracePrintf("AssetBuilderComponent", "RunInResidentMode: Starting resident mode (waiting for commands to arrive)\n");

    AzFramework::SocketConnection::GetInstance()->AddMessageHandler(CreateJobsNetRequest::MessageType(), AZStd::bind(&AssetBuilderComponent::CreateJobsResidentHandler, this, _1, _2, _3, _4));
    AzFramework::SocketConnection::GetInstance()->AddMessageHandler(ProcessJobNetRequest::MessageType(), AZStd::bind(&AssetBuilderComponent::ProcessJobResidentHandler, this, _1, _2, _3, _4));

    bool result = DoHelloPing() && ((sendRegistration && SendRegisteredBuildersToAp()) || !sendRegistration);

    if (result)
    {
        m_running = true;

        m_jobThreadDesc.m_name = "Builder Job Thread";
        m_jobThread = AZStd::thread(m_jobThreadDesc, AZStd::bind(&AssetBuilderComponent::JobThread, this));

        AzFramework::EngineConnectionEvents::Bus::Handler::BusConnect(); // Listen for disconnects

        AZ_TracePrintf("AssetBuilder", "Resident mode ready\n");
        m_mainEvent.acquire();
        AZ_TracePrintf("AssetBuilder", "Shutting down\n");

        m_running = false;
    }

    if (m_jobThread.joinable())
    {
        m_jobEvent.release();
        m_jobThread.join();
    }

    return result;
}

bool AssetBuilderComponent::RunDebugTask(AZStd::string&& debugFile, bool runCreateJobs, bool runProcessJob)
{
    AZ_TracePrintf("AssetBuilderComponent", "RunDebugTask - running debug task on file : %s\n", debugFile.c_str());
    AZ_TracePrintf("AssetBuilderComponent", "RunDebugTask - CreateJobs: %s\n", runCreateJobs ? "True" : "False");
    AZ_TracePrintf("AssetBuilderComponent", "RunDebugTask - ProcessJob: %s\n", runProcessJob ? "True" : "False");

    if (debugFile.empty())
    {
        if (!GetParameter(s_paramInput, debugFile))
        {
            AZ_Error("AssetBuilder", false, "No input file was specified. Use -help for options.");
            return false;
        }
    }
    AZ::StringFunc::Path::Normalize(debugFile);

    if (!GetParameter(s_paramProjectCacheRoot, m_gameCache, false))
    {
        if (m_gameCache.empty())
        {
            // Query the project cache root path from the Settings Registry
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (!settingsRegistry || !settingsRegistry->Get(m_gameCache, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder))
            {
                m_gameCache = ".";
            }
        }
    }

    bool result = false;
    AZ::Data::AssetInfo info;
    AZStd::string watchFolder;
    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, debugFile.c_str(), info, watchFolder);
    if (!result)
    {
        AZ_Error("AssetBuilder", false, "Failed to locate asset info for '%s'.", debugFile.c_str());
        return false;
    }

    AZStd::string binDir;
    AZStd::string module;
    if (GetParameter(s_paramModule, module, false))
    {
        AZ::StringFunc::Path::GetFullPath(module.c_str(), binDir);
        if (!LoadBuilder(module))
        {
            AZ_Error("AssetBuilder", false, "Failed to load module '%s'.", module.c_str());
            return false;
        }
    }
    else
    {
        const char* executableFolder = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(executableFolder, &AZ::ComponentApplicationBus::Events::GetExecutableFolder);
        if (!executableFolder)
        {
            AZ_Error("AssetBuilder", false, "Unable to determine application root.");
            return false;
        }

        AZ::StringFunc::Path::Join(executableFolder, "Builders", binDir);

        if (!LoadBuilders(binDir))
        {
            AZ_Error("AssetBuilder", false, "Failed to load one or more builders from '%s'.", binDir.c_str());
            return false;
        }
    }

    AZStd::string baseTempDirPath;
    if (!GetParameter(s_paramOutput, baseTempDirPath, false))
    {
        AZStd::string fileName;
        AZ::StringFunc::Path::GetFullFileName(debugFile.c_str(), fileName);
        AZStd::replace(fileName.begin(), fileName.end(), '.', '_');

        AZ::StringFunc::Path::Join(binDir.c_str(), "Debug", baseTempDirPath);
        AZ::StringFunc::Path::Join(baseTempDirPath.c_str(), fileName.c_str(), baseTempDirPath);
    }

    // Default tags for the debug task are "tools" and "debug"
    // Additional tags are parsed from command line parameters
    AZStd::unordered_set<AZStd::string> platformTags{"tools", "debug"};
    {
        const AzFramework::CommandLine* commandLine = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);
        if (commandLine)
        {
            size_t tagSwitchSize = commandLine->GetNumSwitchValues(s_paramPlatformTags);
            for (int tagIndex = 0; tagIndex < tagSwitchSize; ++tagIndex)
            {
                platformTags.emplace(commandLine->GetSwitchValue(s_paramPlatformTags, tagIndex));
            }
        }
    }

    AZStd::string platform;

    if(!GetParameter(s_paramPlatform, platform, false))
    {
        platform = "debug platform";
    }

    auto* fileIO = AZ::IO::FileIOBase::GetInstance();

    for (auto& it : m_assetBuilderDescMap)
    {
        AZStd::unique_ptr<AssetBuilderSDK::AssetBuilderDesc>& builder = it.second;
        AZ_Assert(builder, "Invalid description for builder registered.");
        if (!IsBuilderForFile(info.m_relativePath, *builder))
        {
            AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Skipping '%s'.\n", builder->m_name.c_str());
            continue;
        }
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Debugging builder '%s'.\n", builder->m_name.c_str());

        AZStd::string tempDirPath;
        AZ::StringFunc::Path::Join(baseTempDirPath.c_str(), builder->m_name.c_str(), tempDirPath);

        AZStd::vector<AssetBuilderSDK::PlatformInfo> enabledDebugPlatformInfos =
        {
            {
                platform.c_str(), platformTags
            }
        };

        AZStd::vector<AssetBuilderSDK::JobDescriptor> jobDescriptions;
        if (runCreateJobs)
        {
            AZStd::string createJobsTempDirPath;
            AZ::StringFunc::Path::Join(tempDirPath.c_str(), "CreateJobs", createJobsTempDirPath);
            AZ::IO::Result fileResult = fileIO->CreatePath(createJobsTempDirPath.c_str());
            if (!fileResult)
            {
                AZ_Error("AssetBuilder", false, "Unable to create or clear debug folder '%s'.", createJobsTempDirPath.c_str());
                return false;
            }

            AssetBuilderSDK::CreateJobsRequest createRequest(builder->m_busId, info.m_relativePath, watchFolder,
                enabledDebugPlatformInfos, info.m_assetId.m_guid);

            AZ_TraceContext("Source", debugFile);
            AZ_TraceContext("Platforms", AssetBuilderSDK::PlatformInfo::PlatformVectorAsString(createRequest.m_enabledPlatforms));

            AssetBuilderSDK::CreateJobsResponse createResponse;
            builder->m_createJobFunction(createRequest, createResponse);

            AZStd::string responseFile;
            AZ::StringFunc::Path::Join(createJobsTempDirPath.c_str(), "CreateJobsResponse.xml", responseFile);
            if (!AZ::Utils::SaveObjectToFile(responseFile, AZ::DataStream::ST_XML, &createResponse))
            {
                AZ_Error("AssetBuilder", false, "Failed to serialize response to file: %s", responseFile.c_str());
                return false;
            }

            if (runProcessJob)
            {
                jobDescriptions = AZStd::move(createResponse.m_createJobOutputs);
            }
        }

        AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick); // flush assets in case any are present with 0 refcount.

        if (runProcessJob)
        {
            AZStd::string processJobTempDirPath;
            AZ::StringFunc::Path::Join(tempDirPath.c_str(), "ProcessJobs", processJobTempDirPath);
            AZ::IO::Result fileResult = fileIO->CreatePath(processJobTempDirPath.c_str());
            if (!fileResult)
            {
                AZ_Error("AssetBuilder", false, "Unable to create debug or clear folder '%s'.", processJobTempDirPath.c_str());
                return false;
            }

            AssetBuilderSDK::PlatformInfo enabledDebugPlatformInfo = {
                platform.c_str(), { "tools", "debug" }
            };

            AssetBuilderSDK::ProcessJobRequest processRequest;
            processRequest.m_watchFolder = watchFolder;
            processRequest.m_sourceFile = info.m_relativePath;
            processRequest.m_platformInfo = enabledDebugPlatformInfo;
            processRequest.m_sourceFileUUID = info.m_assetId.m_guid;
            AZ::StringFunc::AssetDatabasePath::Join(processRequest.m_watchFolder.c_str(), processRequest.m_sourceFile.c_str(), processRequest.m_fullPath);
            processRequest.m_tempDirPath = processJobTempDirPath;
            processRequest.m_jobId = 0;
            processRequest.m_builderGuid = builder->m_busId;
            processRequest.m_platformInfo.m_tags = platformTags;
            AZ_TraceContext("Source", debugFile);

            if (jobDescriptions.empty())
            {
                for (const auto& platformInfo : enabledDebugPlatformInfos)
                {
                    AssetBuilderSDK::JobDescriptor placeholder;
                    placeholder.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
                    placeholder.m_jobKey = AZStd::string::format("%s_DEBUG", builder->m_name.c_str());
                    placeholder.m_jobParameters[AZ_CRC_CE("Debug")] = "true";
                    jobDescriptions.emplace_back(AZStd::move(placeholder));
                }
            }

            for (size_t i = 0; i < jobDescriptions.size(); ++i)
            {
                AssetBuilderSDK::AssetBuilderTraceBus::Broadcast(&AssetBuilderSDK::AssetBuilderTraceBus::Events::ResetErrorCount);
                AssetBuilderSDK::AssetBuilderTraceBus::Broadcast(&AssetBuilderSDK::AssetBuilderTraceBus::Events::ResetWarningCount);

                processRequest.m_jobDescription = jobDescriptions[i];

                AssetBuilderSDK::ProcessJobResponse processResponse;
                ProcessJob(builder->m_processJobFunction, processRequest, processResponse);

                AZStd::string responseFile;
                AZ::StringFunc::Path::Join(processJobTempDirPath.c_str(),
                    AZStd::string::format("%zu_%s", i, AssetBuilderSDK::s_processJobResponseFileName).c_str(), responseFile);
                if (!AZ::Utils::SaveObjectToFile(responseFile, AZ::DataStream::ST_XML, &processResponse))
                {
                    AZ_Error("AssetBuilder", false, "Failed to serialize response to file: %s", responseFile.c_str());
                    return false;
                }
            }
        }
    }

    return true;
}

void AssetBuilderComponent::FlushFileStreamerCache()
{
    // Force a file streamer flush to ensure that file handles don't remain used or locked between jobs.
    auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
    AZStd::binary_semaphore wait;
    AZ::IO::FileRequestPtr flushRequest = streamer->FlushCaches();
    streamer->SetRequestCompleteCallback(flushRequest, [&wait]([[maybe_unused]] AZ::IO::FileRequestHandle request)
        {
            wait.release();
        });
    streamer->QueueRequest(flushRequest);
    wait.acquire();
}

void AssetBuilderComponent::ProcessJob(const AssetBuilderSDK::ProcessJobFunction& job, const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& outResponse)
{
    // Setup the alias' as appropriate to the job in question.
    auto ioBase = AZ::IO::FileIOBase::GetInstance();
    AZ_Assert(ioBase != nullptr, "AZ::IO::FileIOBase must be ready for use.");

    auto settingsRegistry = AZ::SettingsRegistry::Get();
    AZ_Assert(settingsRegistry != nullptr, "SettingsRegistry must be ready for use in the AssetBuilder.");

    // The root path is the cache plus the platform name.
    AZ::IO::FixedMaxPath newProjectCache(m_gameCache);
    // Check if the platform identifier is a valid "asset platform"
    // If so, use it, other wise use the OS default platform as a fail safe
    // This is to make sure the "debug platform" isn't added as a path segment
    // the Cache ProjectCache folder
    if (AzFramework::PlatformHelper::GetPlatformIdFromName(request.m_platformInfo.m_identifier) != AzFramework::PlatformId::Invalid)
    {
        newProjectCache /= request.m_platformInfo.m_identifier;
    }
    else
    {
        newProjectCache /= AzFramework::OSPlatformToDefaultAssetPlatform(AZ_TRAIT_OS_PLATFORM_CODENAME);
    }

    // Now set the paths and run the job.
    {
        // Save out the prior paths.
        ScopedAliasSetter projectPlatformCacheAliasScope(*ioBase, "@products@", newProjectCache.c_str());
        ScopedSettingsRegistrySetter cacheRootFolderScope(*settingsRegistry,
            AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder, newProjectCache.Native());

        // Invoke the Process Job function
        job(request, outResponse);
    }

    // The asset building ProcessJob method might read any number of source files while processing the asset.
    // Ensure that any exclusive file handle locks caused by this are cleared so that other AssetBuilder processes
    // running in parallel have the ability to read those files as well.
    // This needs to occur after the ProcessJob call, but before the file aliases get cleared.
    FlushFileStreamerCache();

    UpdateResultCode(request, outResponse);
}

bool AssetBuilderComponent::RunOneShotTask(const AZStd::string& task)
{
    AZ_TracePrintf("AssetBuilderComponent", "RunOneShotTask - running one-shot task [%s]\n", task.c_str());
    // Load the requested module.  This is not a required param for the task, since the builders can be in gems.
    AZStd::string modulePath;
    if (GetParameter(s_paramModule, modulePath) && !LoadBuilder(modulePath))
    {
        return false;
    }

    AZStd::string inputFilePath, outputFilePath;
    if (!GetParameter(s_paramInput, inputFilePath)
        || !GetParameter(s_paramOutput, outputFilePath))
    {
        return false;
    }

    AZ::StringFunc::Path::Normalize(inputFilePath);
    AZ::StringFunc::Path::Normalize(outputFilePath);
    if (task == s_taskCreateJob)
    {
        auto func = [this](const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
            {
                AZ_TraceContext("Source", request.m_sourceFile);
                AZ_TraceContext("Platforms", AssetBuilderSDK::PlatformInfo::PlatformVectorAsString(request.m_enabledPlatforms));
                auto assetBuilderDescIt = m_assetBuilderDescMap.find(request.m_builderid);
                if (assetBuilderDescIt != m_assetBuilderDescMap.end())
                {
                    assetBuilderDescIt->second->m_createJobFunction(request, response);
                }
                else
                {
                    AZ_Error("AssetBuilder", false, "Builder UUID [%s] does not exist in the AssetBuilderDescMap for source file %s",
                        request.m_builderid.ToString<AZStd::fixed_string<64>>().c_str(), request.m_sourceFile.c_str());
                }
                // The asset building CreateJob method might read any number of source files to gather a dependency list.
                // Ensure that any exclusive file handle locks caused by this are cleared so that other AssetBuilder processes
                // running in parallel have the ability to read those files as well.
                FlushFileStreamerCache();

            };

        return HandleTask<AssetBuilderSDK::CreateJobsRequest, AssetBuilderSDK::CreateJobsResponse>(inputFilePath, outputFilePath, func);
    }
    else if (task == s_taskProcessJob)
    {
        auto func = [this](const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
            {
                AZ_TraceContext("Source", request.m_fullPath);
                AZ_TraceContext("Platform", request.m_platformInfo.m_identifier);
                auto assetBuilderDescIt = m_assetBuilderDescMap.find(request.m_builderGuid);
                if (assetBuilderDescIt != m_assetBuilderDescMap.end())
                {
                    ProcessJob(assetBuilderDescIt->second->m_processJobFunction, request, response);
                }
                else
                {
                    AZ_Error("AssetBuilder", false, "Builder UUID [%s] does not exist in the AssetBuilderDescMap for source file %s",
                        request.m_builderGuid.ToString<AZStd::fixed_string<64>>().c_str(), request.m_sourceFile.c_str());
                }
            };

        return HandleTask<AssetBuilderSDK::ProcessJobRequest, AssetBuilderSDK::ProcessJobResponse>(inputFilePath, outputFilePath, func);
    }
    else
    {
        AZ_Error("AssetBuilder", false, "Unknown task");
        return false;
    }
}

void AssetBuilderComponent::Disconnected(AzFramework::SocketConnection* /*connection*/)
{
    // If we lose connection to the AP, print out an error and shut down.
    // This prevents builders from running indefinitely if the AP crashes
    AZ_Error("AssetBuilder", false, "Lost connection to Asset Processor, shutting down");
    m_mainEvent.release();
}

bool AssetBuilderComponent::GetAssetDatabaseLocation(AZStd::string& location)
{
    AZ_Error("AssetBuilder", false,
        "Accessing the database directly from a builder is not supported. Many queries behave unexpectedly from builders as the Asset"
        "Processor continuously updates tables as well as risking dead locks. Please use the AssetSystemRequestBus or similar buses "
        "to safely query information from the database.");

    location = "<Unsupported>";
    return false;
}

template<typename TNetRequest, typename TNetResponse>
void AssetBuilderComponent::ResidentJobHandler(AZ::u32 serial, const void* data, AZ::u32 dataLength, JobType jobType)
{
    auto job = AZStd::make_unique<Job>();
    job->m_netResponse = AZStd::make_unique<TNetResponse>();
    job->m_requestSerial = serial;
    job->m_jobType = jobType;

    auto* request = AZ::Utils::LoadObjectFromBuffer<TNetRequest>(data, dataLength);

    if (!request)
    {
        AZ_Error("AssetBuilder", false, "Problem deserializing net request");
        AzFramework::AssetSystem::SendResponse(*(job->m_netResponse), serial);

        return;
    }

    job->m_netRequest = AZStd::unique_ptr<TNetRequest>(request);

    // Queue up the job for the worker thread
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_jobMutex);

        if (!m_queuedJob)
        {
            m_queuedJob.swap(job);
        }
        else
        {
            AZ_Error("AssetBuilder", false, "Builder already has a job queued");
            AzFramework::AssetSystem::SendResponse(*(job->m_netResponse), serial);

            return;
        }
    }

    // Wake up the job thread
    m_jobEvent.release();
}

bool AssetBuilderComponent::IsBuilderForFile(const AZStd::string& filePath, const AssetBuilderSDK::AssetBuilderDesc& builderDescription) const
{
    for (const AssetBuilderSDK::AssetBuilderPattern& pattern : builderDescription.m_patterns)
    {
        AssetBuilderSDK::FilePatternMatcher matcher(pattern);
        if (matcher.MatchesPath(filePath))
        {
            return true;
        }
    }

    return false;
}

void AssetBuilderComponent::JobThread()
{
    while (m_running)
    {
        m_jobEvent.acquire();

        AZStd::unique_ptr<Job> job;

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_jobMutex);
            job.swap(m_queuedJob);
        }

        if (!job)
        {
            if (m_running)
            {
                AZ_TracePrintf("AssetBuilder", "JobThread woke up, but there was no queued job\n");
            }

            continue;
        }

        AssetBuilderSDK::AssetBuilderTraceBus::Broadcast(&AssetBuilderSDK::AssetBuilderTraceBus::Events::ResetErrorCount);
        AssetBuilderSDK::AssetBuilderTraceBus::Broadcast(&AssetBuilderSDK::AssetBuilderTraceBus::Events::ResetWarningCount);

        size_t allocatedBytesBefore = 0;
        size_t capacityBytesBefore = 0;
        AZ::AllocatorManager::Instance().GarbageCollect();
        AZ::AllocatorManager::Instance().GetAllocatorStats(allocatedBytesBefore, capacityBytesBefore);
        AZ_TracePrintf("AssetBuilder", "AllocatorManager before: allocatedBytes = %zu capacityBytes = %zu\n", allocatedBytesBefore, capacityBytesBefore);

        switch (job->m_jobType)
        {
            case JobType::Create:
            {
                using namespace AssetBuilder;

                auto* netRequest = azrtti_cast<CreateJobsNetRequest*>(job->m_netRequest.get());
                auto* netResponse = azrtti_cast<CreateJobsNetResponse*>(job->m_netResponse.get());
                AZ_Assert(netRequest && netResponse, "Request or response is null");

                AZ::IO::FixedMaxPath fullPath(netRequest->m_request.m_watchFolder);
                fullPath /= netRequest->m_request.m_sourceFile;

                AZ_TracePrintf("AssetBuilder", "Source = %s\n", fullPath.c_str());
                AZ_TracePrintf("AssetBuilder", "Platforms = %s\n", AssetBuilderSDK::PlatformInfo::PlatformVectorAsString(netRequest->m_request.m_enabledPlatforms).c_str());

                auto assetBuilderDescIt = m_assetBuilderDescMap.find(netRequest->m_request.m_builderid);
                if (assetBuilderDescIt != m_assetBuilderDescMap.end())
                {
                    assetBuilderDescIt->second->m_createJobFunction(netRequest->m_request, netResponse->m_response);
                }
                else
                {
                    AZ_Error("AssetBuilder", false, "Builder UUID [%s] does not exist in the AssetBuilderDescMap for source file %s",
                        netRequest->m_request.m_builderid.ToString<AZStd::fixed_string<64>>().c_str(), netRequest->m_request.m_sourceFile.c_str());
                }

                break;
            }
            case JobType::Process:
            {
                using namespace AssetBuilder;

                AZ_TracePrintf("AssetBuilder", "Running processJob task\n");

                auto* netRequest = azrtti_cast<ProcessJobNetRequest*>(job->m_netRequest.get());
                auto* netResponse = azrtti_cast<ProcessJobNetResponse*>(job->m_netResponse.get());
                AZ_Assert(netRequest && netResponse, "Request or response is null");

                AZ_TracePrintf("AssetBuilder", "Source = %s\n", netRequest->m_request.m_fullPath.c_str());
                AZ_TracePrintf("AssetBuilder", "Platform = %s\n", netRequest->m_request.m_jobDescription.GetPlatformIdentifier().c_str());

                auto assetBuilderDescIt = m_assetBuilderDescMap.find(netRequest->m_request.m_builderGuid);
                if (assetBuilderDescIt != m_assetBuilderDescMap.end())
                {
                    auto* toolsCatalog = AZ::Interface<AssetProcessor::IToolsAssetCatalog>::Get();

                    if (toolsCatalog)
                    {
                        toolsCatalog->SetActivePlatform(netRequest->m_request.m_jobDescription.GetPlatformIdentifier());
                    }
                    else
                    {
                        AZ_Warning("AssetBuilder", false, "Failed to retrieve IToolsAssetCatalog interface, cannot set current platform");
                    }

                    ProcessJob(assetBuilderDescIt->second->m_processJobFunction, netRequest->m_request, netResponse->m_response);
                }
                else
                {
                    AZ_Error("AssetBuilder", false, "Builder UUID [%s] does not exist in the AssetBuilderDescMap for source file %s",
                        netRequest->m_request.m_builderGuid.ToString<AZStd::fixed_string<64>>().c_str(), netRequest->m_request.m_sourceFile.c_str());
                }
                break;
            }
            default:
                AZ_Error("AssetBuilder", false, "Unhandled job request type");
                continue;
        }

        size_t allocatedBytesAfter = 0;
        size_t capacityBytesAfter = 0;
        AZ::AllocatorManager::Instance().GarbageCollect();
        AZ_MALLOC_TRIM(0);
        AZ::AllocatorManager::Instance().GetAllocatorStats(allocatedBytesAfter, capacityBytesAfter);
        AZ_TracePrintf("AssetBuilder", "AllocatorManager after: allocatedBytes = %zu capacityBytes = %zu; allocated change = %zd\n",
            allocatedBytesAfter, capacityBytesAfter, allocatedBytesAfter - allocatedBytesBefore);

        AZ::u32 warningCount, errorCount;
        AssetBuilderSDK::AssetBuilderTraceBus::BroadcastResult(warningCount, &AssetBuilderSDK::AssetBuilderTraceBus::Events::GetWarningCount);
        AssetBuilderSDK::AssetBuilderTraceBus::BroadcastResult(errorCount, &AssetBuilderSDK::AssetBuilderTraceBus::Events::GetErrorCount);

        AZ_TracePrintf("S", "%d errors, %d warnings\n", errorCount, warningCount);

        //Flush our output so the AP can properly associate all output with the current job
        std::fflush(stdout);
        std::fflush(stderr);

        AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);
        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, 0.00f, AZ::ScriptTimePoint(AZStd::chrono::steady_clock::now()));
        AZ::AllocatorManager::Instance().GarbageCollect();

        AzFramework::AssetSystem::SendResponse(*(job->m_netResponse), job->m_requestSerial);
    }
}

void AssetBuilderComponent::CreateJobsResidentHandler(AZ::u32 /*typeId*/, AZ::u32 serial, const void* data, AZ::u32 dataLength)
{
    using namespace AssetBuilder;

    ResidentJobHandler<CreateJobsNetRequest, CreateJobsNetResponse>(serial, data, dataLength, JobType::Create);
}

void AssetBuilderComponent::ProcessJobResidentHandler(AZ::u32 /*typeId*/, AZ::u32 serial, const void* data, AZ::u32 dataLength)
{
    using namespace AssetBuilder;

    ResidentJobHandler<ProcessJobNetRequest, ProcessJobNetResponse>(serial, data, dataLength, JobType::Process);
}

//////////////////////////////////////////////////////////////////////////

template<typename TRequest, typename TResponse>
bool AssetBuilderComponent::HandleTask(const AZStd::string& inputFilePath, const AZStd::string& outputFilePath, const AZStd::function<void(const TRequest& request, TResponse& response)>& assetBuilderFunc)
{
    TRequest request;
    TResponse response;

    if (!AZ::Utils::LoadObjectFromFileInPlace(inputFilePath, request))
    {
        AZ_Error("AssetBuilder", false, "Failed to deserialize request from file: %s", inputFilePath.c_str());
        return false;
    }

    assetBuilderFunc(request, response);

    if (!AZ::Utils::SaveObjectToFile(outputFilePath, AZ::DataStream::ST_XML, &response))
    {
        AZ_Error("AssetBuilder", false, "Failed to serialize response to file: %s", outputFilePath.c_str());
        return false;
    }

    return true;
}

void AssetBuilderComponent::UpdateResultCode(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
{
    if (request.m_jobDescription.m_failOnError)
    {
        AZ::u32 errorCount = 0;
        AssetBuilderSDK::AssetBuilderTraceBus::BroadcastResult(errorCount, &AssetBuilderSDK::AssetBuilderTraceBus::Events::GetErrorCount);
        if (errorCount > 0 && response.m_resultCode == AssetBuilderSDK::ProcessJobResult_Success)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
        }
    }
}

bool AssetBuilderComponent::GetParameter(const char* paramName, AZStd::string& outValue, [[maybe_unused]] bool required /*= true*/) const
{
    const AzFramework::CommandLine* commandLine = nullptr;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);

    size_t optionCount = commandLine->GetNumSwitchValues(paramName);
    if (optionCount > 0)
    {
        outValue = commandLine->GetSwitchValue(paramName, optionCount - 1);
    }

    if (outValue.empty())
    {
        AZ_Error("AssetBuilder", !required, "Missing required parameter `%s`. Use -help for options.", paramName);
        return false;
    }

    return true;
}

const char* AssetBuilderComponent::GetLibraryExtension()
{
    return "*" AZ_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION;
}

bool AssetBuilderComponent::LoadBuilders([[maybe_unused]] const AZStd::string& builderFolder)
{
    // LoadBuilders by folder has been removed.  Builders should all live within gems
    AZ_TracePrintf("AssetBuilderComponent", "LoadBuilders - Called LoadBuilders for [%s] - SKIPPING\n", builderFolder.c_str());
    return true;
}

bool AssetBuilderComponent::LoadBuilder(const AZStd::string& filePath)
{
    using AssetBuilder::AssetBuilderType;
    auto assetBuilderInfo = AZStd::make_unique<AssetBuilder::ExternalModuleAssetBuilderInfo>(QString::fromUtf8(filePath.c_str()));

    if (assetBuilderInfo->GetAssetBuilderType() == AssetBuilderType::Valid)
    {
        if (!assetBuilderInfo->IsLoaded())
        {
            AZ_Warning("AssetBuilder", false, "AssetBuilder was not able to load the library: %s\n", filePath.c_str());
            return false;
        }
    }

    AssetBuilderType builderType = assetBuilderInfo->GetAssetBuilderType();
    if (builderType == AssetBuilderType::Valid)
    {
        AZ_TracePrintf("AssetBuilder", "LoadBuilder - Initializing and registering builder [%s]\n", assetBuilderInfo->GetName().toUtf8().constData());

        m_currentAssetBuilder = assetBuilderInfo.get();
        m_currentAssetBuilder->Initialize();
        m_currentAssetBuilder = nullptr;

        m_assetBuilderInfoList.push_back(AZStd::move(assetBuilderInfo));
        return true;
    }
    if (builderType == AssetBuilderType::Invalid)
    {
        return false;
    }
    return true;
}

void AssetBuilderComponent::UnloadBuilders()
{
    m_assetBuilderDescMap.clear();

    for (auto& assetBuilderInfo : m_assetBuilderInfoList)
    {
        AZ_TracePrintf("AssetBuilderComponent", "UnloadBuilders - unloading builder [%s]\n", assetBuilderInfo->GetName().toUtf8().constData());
        assetBuilderInfo->UnInitialize();
    }

    m_assetBuilderInfoList.clear();
}

bool AssetBuilderComponent::FindBuilderInformation(const AZ::Uuid& builderGuid, AssetBuilderSDK::AssetBuilderDesc& descriptionOut)
{
    auto iter = m_assetBuilderDescMap.find(builderGuid);
    if (iter != m_assetBuilderDescMap.end())
    {
        descriptionOut = *iter->second;
        return true;
    }
    else
    {
        return false;
    }
}

void AssetBuilderComponent::RegisterBuilderInformation(const AssetBuilderSDK::AssetBuilderDesc& builderDesc)
{
    m_assetBuilderDescMap.insert({ builderDesc.m_busId, AZStd::make_unique<AssetBuilderSDK::AssetBuilderDesc>(builderDesc) });

    if (m_currentAssetBuilder)
    {
        m_currentAssetBuilder->RegisterBuilderDesc(builderDesc.m_busId);
    }
}

void AssetBuilderComponent::RegisterComponentDescriptor(AZ::ComponentDescriptor* descriptor)
{
    if (m_currentAssetBuilder)
    {
        m_currentAssetBuilder->RegisterComponentDesc(descriptor);
    }
}
