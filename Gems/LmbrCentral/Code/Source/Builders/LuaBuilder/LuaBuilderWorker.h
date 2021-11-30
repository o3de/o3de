/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Outcome/Outcome.h>
namespace AZ
{
    class ScriptContext;
}

namespace LuaBuilder
{
    class LuaBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_TYPE_INFO(LuaBuilderWorker, "{166A7962-A3E4-4451-AC1A-AAD32E29C52C}");
        ~LuaBuilderWorker() override = default;

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        //////////////////////////////////////////////////////////////////////////
        // AssetBuilderSDK::AssetBuilderCommandBus
        void ShutDown() override;
        //////////////////////////////////////////////////////////////////////////

        void ParseDependencies(const AZStd::string& file, AssetBuilderSDK::ProductPathDependencySet& outDependencies);

    private:
        bool m_isShuttingDown = false;

        using JobStepOutcome = AZ::Outcome<AssetBuilderSDK::JobProduct, AssetBuilderSDK::ProcessJobResultCode>;

        JobStepOutcome RunCompileJob(const AssetBuilderSDK::ProcessJobRequest& request);
        JobStepOutcome RunCopyJob(const AssetBuilderSDK::ProcessJobRequest& request);
        JobStepOutcome WriteAssetInfo(
            const AssetBuilderSDK::ProcessJobRequest& request,
            AZStd::string_view destFileName,
            AZStd::string_view debugName,
            AZ::ScriptContext& scriptContext);
    };
} // namespace LuaBuilder
