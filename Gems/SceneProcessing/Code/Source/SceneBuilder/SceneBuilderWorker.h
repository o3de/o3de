/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AssetBuilderSDK
{
    struct CreateJobsRequest;
    struct CreateJobsResponse;
    struct ProcessJobRequest;
    struct ProcessJobResponse;
    struct JobProduct;
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace Events
        {
            struct ExportProduct;
            class ExportProductList;
        }
    }
}

namespace SceneBuilder
{
    class SceneBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        ~SceneBuilderWorker() override = default;

        
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        void ShutDown() override;
        const char* GetFingerprint() const;
        static void PopulateSourceDependencies(
            const AZStd::string& manifestJson, AZStd::vector<AssetBuilderSDK::SourceFileDependency>& sourceFileDependencies);
        static bool ManifestDependencyCheck(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        static AZ::Uuid GetUUID();

        void PopulateProductDependencies(const AZ::SceneAPI::Events::ExportProduct& exportProduct, const char* watchFolder, AssetBuilderSDK::JobProduct& jobProduct) const;

    protected:
        bool LoadScene(AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& result, 
            const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        // @brief Execute runtime modifications to the Scene graph
        //
        // This step is run after the scene is loaded, but before the scene
        // is exported. It emits events with the GenerateEventContext.
        // Event handlers bound to that event can apply arbitrary
        // transformations to the Scene, adding new nodes, replacing nodes,
        // or removing nodes.
        bool GenerateScene(AZ::SceneAPI::Containers::Scene* result,
            const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);
        bool ExportScene(const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene, 
            const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        AZ::u32 BuildSubId(const AZ::SceneAPI::Events::ExportProduct& product) const;

        bool m_isShuttingDown = false;
        mutable AZStd::string m_cachedFingerprint;
    };
} // namespace SceneBuilder
