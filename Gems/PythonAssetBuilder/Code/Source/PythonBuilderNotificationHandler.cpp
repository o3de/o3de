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

#include <PythonBuilderNotificationHandler.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <EditorPythonBindings/EditorPythonBindingsSymbols.h>
#include <Source/PythonAssetBuilderSystemComponent.h>

namespace PythonAssetBuilder
{
    void PythonBuilderNotificationHandler::Reflect(AZ::ReflectContext * context)
    {
        if (auto&& behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<PythonBuilderNotificationBus>("PythonBuilderNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "asset.builder")
                ->Handler<PythonBuilderNotificationHandler>()
                ->Event("OnCreateJobsRequest", &PythonBuilderNotificationBus::Events::OnCreateJobsRequest)
                ->Event("OnProcessJobRequest", &PythonBuilderNotificationBus::Events::OnProcessJobRequest)
                ->Event("OnShutdown", &PythonBuilderNotificationBus::Events::OnShutdown)
                ->Event("OnCancel", &PythonBuilderNotificationBus::Events::OnCancel)
                ;
        }
    }

    AssetBuilderSDK::CreateJobsResponse PythonBuilderNotificationHandler::OnCreateJobsRequest(const AssetBuilderSDK::CreateJobsRequest& request)
    {
        AssetBuilderSDK::CreateJobsResponse response;
        try
        {
            auto editorPythonEventsInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get();
            if (editorPythonEventsInterface)
            {
                editorPythonEventsInterface->ExecuteWithLock([&response, &request, this]()
                {
                        this->CallResult(response, FN_OnCreateJobsRequest, request);
                });
            }
            else
            {
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
            }
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error(PythonBuilder, false, "OnCreateJobsRequest exception %s", e.what());
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
        }
        return response;
    }

    AssetBuilderSDK::ProcessJobResponse PythonBuilderNotificationHandler::OnProcessJobRequest(const AssetBuilderSDK::ProcessJobRequest& request)
    {
        AssetBuilderSDK::ProcessJobResponse response;
        try
        {
            auto editorPythonEventsInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get();
            if (editorPythonEventsInterface)
            {
                editorPythonEventsInterface->ExecuteWithLock([&response, &request, this]()
                {
                    this->CallResult(response, FN_OnProcessJobRequest, request);
                });
            }
            else
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            }
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error(PythonBuilder, false, "OnProcessJobRequest exception %s", e.what());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
        }
        return response;
    }

    void PythonBuilderNotificationHandler::OnShutdown()
    {
        try
        {
            auto editorPythonEventsInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get();
            if (editorPythonEventsInterface)
            {
                editorPythonEventsInterface->ExecuteWithLock([this]()
                {
                    this->Call(FN_OnShutdown);
                });
            }
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning(PythonBuilder, false, "OnShutdown exception %s", e.what());
        }
    }

    void PythonBuilderNotificationHandler::OnCancel()
    {
        try
        {
            auto editorPythonEventsInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get();
            if (editorPythonEventsInterface)
            {
                editorPythonEventsInterface->ExecuteWithLock([this]()
                {
                    this->Call(FN_OnCancel);
                });
            }
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error(PythonBuilder, false, "OnCancel exception %s", e.what());
        }
    }
}
