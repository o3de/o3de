/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PythonBuilderWorker.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <Source/PythonAssetBuilderSystemComponent.h>
#include <PythonAssetBuilder/PythonBuilderNotificationBus.h>

namespace PythonAssetBuilder
{
    void PythonBuilderWorker::Reflect(AZ::ReflectContext* context)
    {
        if (auto&& serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PythonBuilderWorker>()->Version(0);
        }

        if (auto&& behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<PythonBuilderWorker>("PythonBuilderWorker")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "asset.builder")
                ->Constructor()
                ;
        }
    }

    bool PythonBuilderWorker::ConfigureBuilderInformation(const AssetBuilderSDK::AssetBuilderDesc& assetBuilderDesc)
    {
        using namespace AssetBuilderSDK;

        if (m_assetBuilderDesc)
        {
            AZ_Error(PythonBuilder, false, "Asset Builder (%s) already configured!", assetBuilderDesc.m_name.c_str());
            return false;
        }

        // register the new PythonBuilderWorker instance with the Asset Builder
        m_assetBuilderDesc = AZStd::make_shared<AssetBuilderSDK::AssetBuilderDesc>(assetBuilderDesc);

        // prepare delegate handler for CreateJobs function to be resolved in a Python script
        m_assetBuilderDesc->m_createJobFunction = [this](auto&& request, auto&& response)
        {
            this->CreateJobs(request, response);
        };

        // prepare delegate handler for ProcessJob function to be resolved in a Python script
        m_assetBuilderDesc->m_processJobFunction = [this](auto&& request, auto&& response)
        {
            this->ProcessJob(request, response);
        };

        // connect to the shutdown signal handler
        AssetBuilderCommandBus::Handler::BusConnect(m_assetBuilderDesc->m_busId);

        // register with the Asset Builder
        AssetBuilderBus::Broadcast(&AssetBuilderBus::Events::RegisterBuilderInformation, *m_assetBuilderDesc);
        return true;
    }

    void PythonBuilderWorker::ShutDown()
    {
        // Note - Shutdown will be called on a different thread than your process job thread
        if (!m_isShuttingDown)
        {
            m_isShuttingDown = true;
            PythonBuilderNotificationBus::Event(m_busId, &PythonBuilderNotificationBus::Events::OnShutdown);
            AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusDisconnect();
        }
    }

    void PythonBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        // assume failure
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;

        PythonBuilderNotificationBus::EventResult(
            response,
            request.m_builderid,
            &PythonBuilderNotificationBus::Events::OnCreateJobsRequest,
            request);
    }

    void PythonBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AssetBuilderSDK::JobCommandBus::Handler::BusConnect(request.m_jobId);

        // assume failure
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;

        PythonBuilderNotificationBus::EventResult(
            response,
            request.m_builderGuid,
            &PythonBuilderNotificationBus::Events::OnProcessJobRequest,
            request);

        AssetBuilderSDK::JobCommandBus::Handler::BusDisconnect(request.m_jobId);
    }

    void PythonBuilderWorker::Cancel()
    {
        PythonBuilderNotificationBus::Event(m_busId, &PythonBuilderNotificationBus::Events::OnCancel);
    }
}
