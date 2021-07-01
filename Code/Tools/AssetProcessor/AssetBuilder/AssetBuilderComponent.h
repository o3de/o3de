/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzFramework/Network/SocketConnection.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include "AssetBuilderInfo.h"

//! This bus is used to signal to the AssetBuilderComponent to start up and execute while providing a return code
class BuilderBusTraits
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    typedef AZStd::recursive_mutex MutexType;

    virtual ~BuilderBusTraits() = default;

    virtual bool Run() = 0;
};

typedef AZ::EBus<BuilderBusTraits> BuilderBus;

//! Main component of the AssetBuilder that handles interfacing with the AssetProcessor and the Builder module(s)
//! In resident mode, the component will keep the application up and running indefinitely while accepting job requests from the AP network connection
//! The other mods (create, process) will read a job from an `input` file and write the response to the `output` file and then terminate
class AssetBuilderComponent
    : public AZ::Component,
    public BuilderBus::Handler,
    public AssetBuilderSDK::AssetBuilderBus::Handler,
    public AzFramework::EngineConnectionEvents::Bus::Handler,
    public AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler
{
public:
    AZ_COMPONENT(AssetBuilderComponent, "{04332899-5d73-4d41-86b7-b1017d349673}")
    static void Reflect(AZ::ReflectContext* context);

    AssetBuilderComponent() = default;
    ~AssetBuilderComponent() override = default;

    void PrintHelp();

    // AZ::Component overrides
    void Activate() override;
    void Deactivate() override;

    // BuilderBus Handler
    bool Run() override;

    // AssetBuilderBus Handler
    bool FindBuilderInformation(const AZ::Uuid& builderGuid, AssetBuilderSDK::AssetBuilderDesc& descriptionOut) override;

    void RegisterBuilderInformation(const AssetBuilderSDK::AssetBuilderDesc& builderDesc) override;
    void RegisterComponentDescriptor(AZ::ComponentDescriptor* descriptor) override;
    
    //EngineConnectionEvents Handler
    void Disconnected(AzFramework::SocketConnection* connection) override;

    static bool IsInDebugMode(const AzFramework::CommandLine& commandLine);
    //AssetDatabaseRequestsBus Handler
    bool GetAssetDatabaseLocation(AZStd::string& location) override;
protected:

    AZ_DISABLE_COPY_MOVE(AssetBuilderComponent);

    enum class JobType
    {
        Create,
        Process
    };

    //! Describes a job request that came in from the network connection
    struct Job
    {
        JobType m_jobType;
        AZ::u32 m_requestSerial;
        AZStd::unique_ptr<AzFramework::AssetSystem::BaseAssetProcessorMessage> m_netRequest;
        AZStd::unique_ptr<AzFramework::AssetSystem::BaseAssetProcessorMessage> m_netResponse;
    };

    //! Reads a command line parameter and places it in the outValue parameter.  Returns false if the value is empty, true otherwise
    //! If required is true, an AZ_Error message is output
    bool GetParameter(const char* paramName, AZStd::string& outValue, bool required = true) const;

    //! Returns the platform specific extension for dynamic libraries
    static const char* GetLibraryExtension();

    bool ConnectToAssetProcessor();
    bool LoadBuilders(const AZStd::string& builderFolder);
    bool LoadBuilder(const AZStd::string& filePath);
    void UnloadBuilders();

    //! Hooks up net job request handling and keeps the AssetBuilder running indefinitely
    bool RunInResidentMode();
    bool RunDebugTask(AZStd::string&& debugFile, bool runCreateJobs, bool runProcessJob);
    bool RunOneShotTask(const AZStd::string& task);

    template<typename TNetRequest, typename TNetResponse>
    void ResidentJobHandler(AZ::u32 serial, const void* data, AZ::u32 dataLength, JobType jobType);
    void CreateJobsResidentHandler(AZ::u32 typeId, AZ::u32 serial, const void* data, AZ::u32 dataLength);
    void ProcessJobResidentHandler(AZ::u32 typeId, AZ::u32 serial, const void* data, AZ::u32 dataLength);

    bool IsBuilderForFile(const AZStd::string& filePath, const AssetBuilderSDK::AssetBuilderDesc& builderDescription) const;

    //! Run by a separate thread to avoid blocking the net recv thread
    //! Handles calling the appropriate builder job function for the incoming job
    void JobThread();

    void ProcessJob(const AssetBuilderSDK::ProcessJobFunction& job, const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& outResponse);

    //! Handles a builder registration request
    bool HandleRegisterBuilder(const AZStd::string& inputFilePath, const AZStd::string& outputFilePath) const;

    //! If needed looks at collected data and updates the result code from the job accordingly.
    void UpdateResultCode(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

    //! Handles reading the request from file, passing it to the specified function and writing the response to file
    template<typename TRequest, typename TResponse>
    bool HandleTask(const AZStd::string& inputFilePath, const AZStd::string& outputFilePath, const AZStd::function<void(const TRequest& request, TResponse& response)>& handlerFunc);

    //! Flush the File Streamer cache to ensure that there aren't stale file handles or data between asset job runs.
    void FlushFileStreamerCache();

    //! Map used to look up the asset builder to handle a request
    AZStd::unordered_map<AZ::Uuid, AZStd::unique_ptr<AssetBuilderSDK::AssetBuilderDesc>> m_assetBuilderDescMap;

    //! List of loaded builders
    AZStd::vector<AZStd::unique_ptr<AssetBuilder::ExternalModuleAssetBuilderInfo>> m_assetBuilderInfoList;

    //! Currently loading builder
    AssetBuilder::ExternalModuleAssetBuilderInfo* m_currentAssetBuilder = nullptr;
    
    //! Thread for running a job, so we don't block the network thread while doing work
    AZStd::thread_desc m_jobThreadDesc;
    AZStd::thread m_jobThread;

    //! Indicates if resident mode is up and running
    AZStd::atomic<bool> m_running{};

    //! Main thread will wait on this event in resident mode.  Releasing it will shut down the application
    AZStd::binary_semaphore m_mainEvent;
    //! Use to signal a new job is ready to be processed
    AZStd::binary_semaphore m_jobEvent;
    
    //! Lock for m_queuedJob
    AZStd::mutex m_jobMutex;

    //! Stored job that is waiting to be picked up for processing by the job thread
    AZStd::unique_ptr<Job> m_queuedJob;

    AZStd::string m_gameName;
    AZStd::string m_projectPath;
    AZStd::string m_gameCache;
};
