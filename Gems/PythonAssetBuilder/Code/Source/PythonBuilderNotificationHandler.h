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

#include <PythonAssetBuilder/PythonBuilderNotificationBus.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>

namespace PythonAssetBuilder
{
    class PythonBuilderNotificationHandler final
        : public AZ::BehaviorEBusHandler
        , public PythonBuilderNotificationBus::Handler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(PythonBuilderNotificationHandler, "{9CF1761E-3365-42F7-83D0-5039B1B73223}", AZ::SystemAllocator,
            OnCreateJobsRequest, OnProcessJobRequest, OnShutdown, OnCancel);

        static void Reflect(AZ::ReflectContext* context);

    protected:
        AssetBuilderSDK::CreateJobsResponse OnCreateJobsRequest(const AssetBuilderSDK::CreateJobsRequest& request) override;
        AssetBuilderSDK::ProcessJobResponse OnProcessJobRequest(const AssetBuilderSDK::ProcessJobRequest& request) override;
        void OnShutdown() override;
        void OnCancel() override;
    };
}
