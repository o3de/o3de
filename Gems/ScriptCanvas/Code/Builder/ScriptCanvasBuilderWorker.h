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
#include <AzCore/Asset/AssetCommon.h>


namespace AZ
{
    namespace Data
    {
        class AssetHandler;
    }
}

namespace ScriptCanvasBuilder
{
    constexpr const char* s_scriptCanvasBuilder = "ScriptCanvasBuilder";
    constexpr const char* s_scriptCanvasProcessJobKey = "Script Canvas Process Job";
    constexpr const char* s_unitTestParseErrorPrefix = "LY_SC_UnitTest";

    enum class BuilderVersion : int
    {
        SplitCopyFromCompileJobs = 9,
        ChangeScriptRequirementToAsset,
        RemoveDebugVariablesFromRelease,
        FailJobsOnMissingLKG,
        QuantumLeap,
        DependencyArguments,
        DependencyRequirementsData,
        AddAssetDependencySearch,
        PrefabIntegration,
        CorrectGraphVariableVersion,
        ReflectEntityIdNodes,
        FixExecutionStateNodeableConstruction,
        SwitchAssetsToBinary,
        // add new entries above
        Current,
    };

    struct AssetHandlers;

    class Worker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        static AZ::Uuid GetUUID();

        Worker() = default;
        Worker(const Worker&) = delete;

        void Activate(const AssetHandlers& handlers);

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;

        const char* GetFingerprintString() const;

        int GetVersionNumber() const;

        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

        void ShutDown() override {};

    private:
        AZ::Data::AssetHandler* m_runtimeAssetHandler = nullptr;
        AZ::Data::AssetHandler* m_subgraphInterfaceHandler = nullptr;
        AZ::Data::AssetHandler* m_builderHandler = nullptr;

        mutable AZStd::vector<AZ::Data::AssetFilterInfo> m_processEditorAssetDependencies;
        // cached on first time query
        mutable AZStd::string m_fingerprintString;
    };
}
