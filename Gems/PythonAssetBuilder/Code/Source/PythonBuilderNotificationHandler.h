/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <PythonAssetBuilder/PythonBuilderNotificationBus.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzCore/RTTI/BehaviorContext.h>

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
