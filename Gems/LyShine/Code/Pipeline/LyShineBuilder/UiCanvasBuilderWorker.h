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

#include <AzCore/Component/Component.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <LyShine/Bus/Tools/UiSystemToolsBus.h>
#include <AzToolsFramework/Slice/SliceCompilation.h>

namespace LyShine
{
    class UiCanvasBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        UiCanvasBuilderWorker() = default;
        ~UiCanvasBuilderWorker() = default;

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

        //////////////////////////////////////////////////////////////////////////
        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override;
        //////////////////////////////////////////////////////////////////////////

        static AZ::Uuid GetUUID();

        bool ProcessUiCanvasAndGetDependencies(AZ::IO::GenericStream& stream,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& productPathDependencySet,
            UiSystemToolsInterface::CanvasAssetHandle*& canvasAsset,
            AZ::Entity*& sourceCanvasEntity,
            AZ::Entity& exportCanvasEntity) const;

    private:

        UiCanvasBuilderWorker(const UiCanvasBuilderWorker&) = delete;

        bool m_isShuttingDown = false;

        //! Since uicanvases can currently have the same entity ID across multiple canvas files, we need to process the canvases one at a time to avoid
        //! an assert about duplicate entities.  This has no noticeable effect on performance right now
        mutable AZStd::mutex m_processingMutex;
    };
}