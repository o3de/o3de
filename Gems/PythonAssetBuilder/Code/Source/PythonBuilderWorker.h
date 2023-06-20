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

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>

#include <EditorPythonBindings/EditorPythonBindingsBus.h>
#include <PythonAssetBuilder/PythonAssetBuilderBus.h>

namespace PythonAssetBuilder
{
    //! A delegate asset build worker for Python scripts
    class PythonBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        , public AssetBuilderSDK::JobCommandBus::Handler
    {
    public:
        AZ_TYPE_INFO(PythonBuilderWorker, "{F27E64FB-A7FF-47F2-80DB-7E1371B014DD}");
        AZ_CLASS_ALLOCATOR(PythonBuilderWorker, AZ::SystemAllocator);

        PythonBuilderWorker() = default;
        virtual ~PythonBuilderWorker() = default;

        static void Reflect(AZ::ReflectContext* context);

        //! Configure the Python builder using an asset builder description; should only be done once
        bool ConfigureBuilderInformation(const AssetBuilderSDK::AssetBuilderDesc& assetBuilderDesc);

    protected:
        //! AssetBuilder callback functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        //! AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override;

        //! AssetBuilderSDK::JobCommandBus interface
        void Cancel() override;

    private:
        AZ::Uuid m_busId = AZ::Uuid::CreateNull();
        bool m_isShuttingDown = false;
        AZStd::shared_ptr<AssetBuilderSDK::AssetBuilderDesc> m_assetBuilderDesc;
    };
}
