/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetDataStream.h>

namespace ScriptCanvas { class RuntimeFunctionAsset; }

namespace AZ
{
    namespace Data
    {
        class AssetHandler;
    }
}

namespace ScriptCanvas
{
    class RuntimeAsset;
}

namespace ScriptCanvasBuilder
{
    class Worker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        static AZ::Outcome<AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> CreateRuntimeAsset(AZStd::string_view graphPath);
        static AZ::Data::Asset<ScriptCanvas::RuntimeAsset> ProcessEditorAsset(AZ::Data::AssetHandler& editorAssetHandler,
                                                                              AZStd::shared_ptr<AZ::Data::AssetDataStream> editorAssetStream);

        Worker();
        ~Worker();
        
        int GetVersionNumber() const;
        const char* GetFingerprintString() const;

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;
        
        //////////////////////////////////////////////////////////////////////////
        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override;
        //////////////////////////////////////////////////////////////////////////
        
        // Helper function for unit tests
        static bool GatherProductDependencies(
            AZ::SerializeContext& serializeContext,
            AZ::Data::Asset<ScriptCanvas::RuntimeAsset>& runtimeAsset,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& productPathDependencySet);

        void Activate();
        void Deactivate();

        static AZ::Uuid GetUUID();

    private:
        Worker(const Worker&) = delete;
        bool m_isShuttingDown = false;
        
        AZStd::unique_ptr<AZ::Data::AssetHandler> m_editorAssetHandler;
        AZStd::unique_ptr<AZ::Data::AssetHandler> m_runtimeAssetHandler;

        // cached on first time query
        mutable AZStd::string m_fingerprintString;
    };
    
    class FunctionWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        static AZ::Outcome<AZ::Data::Asset<ScriptCanvas::RuntimeAsset>, AZStd::string> CreateRuntimeAsset(AZStd::string_view graphPath);
        static AZ::Data::Asset<ScriptCanvas::RuntimeFunctionAsset> ProcessFunctionAsset(
            AZ::Data::AssetHandler& functionAssetHandler,
            AZStd::shared_ptr<AZ::Data::AssetDataStream>  editorAssetStream);

        FunctionWorker() = default;
        ~FunctionWorker() override = default;

        int GetVersionNumber() const { return 1; }
        const char* GetFingerprintString() const;

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

        //////////////////////////////////////////////////////////////////////////
        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override;
        //////////////////////////////////////////////////////////////////////////

        void Activate();
        void Deactivate();

        static AZ::Uuid GetUUID();

    private:
        FunctionWorker(const FunctionWorker&) = delete;
        bool m_isShuttingDown = false;

        AZStd::unique_ptr<AZ::Data::AssetHandler> m_functionAssetHandler;
        AZStd::unique_ptr<AZ::Data::AssetHandler> m_runtimeAssetHandler;

        // cached on first time query
        mutable AZStd::string m_fingerprintString;
    };
}
