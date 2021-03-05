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

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/ComponentApplication.h>

#include <EditorPythonBindings/EditorPythonBindingsSymbols.h>

#include "Source/PythonAssetBuilderSystemComponent.h"
#include <PythonAssetBuilder/PythonAssetBuilderBus.h>
#include <PythonAssetBuilder/PythonBuilderNotificationBus.h>

namespace UnitTest
{
    struct MockJobHandler final
        : public PythonAssetBuilder::PythonBuilderNotificationBus::Handler
    {
        int m_onShutdownCount = 0;
        int m_onCancelCount = 0;

        AssetBuilderSDK::CreateJobsResponse OnCreateJobsRequest(const AssetBuilderSDK::CreateJobsRequest& request) override
        {
            if (request.m_sourceFileUUID.IsNull())
            {
                return AssetBuilderSDK::CreateJobsResponse{};
            }

            AssetBuilderSDK::CreateJobsResponse response;
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return response;
        }

        AssetBuilderSDK::ProcessJobResponse OnProcessJobRequest(const AssetBuilderSDK::ProcessJobRequest& request)
        {
            if (request.m_sourceFileUUID.IsNull())
            {
                return AssetBuilderSDK::ProcessJobResponse{};
            }

            AssetBuilderSDK::ProcessJobResponse response;
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            return response;
        }

        void OnShutdown()
        {
            ++m_onShutdownCount;
        }

        void OnCancel()
        {
            ++m_onCancelCount;
        }
    };

    template <typename App, typename EntityType>
    AZ::Uuid RegisterAssetBuilder(App* app, EntityType* systemEntity)
    {
        using namespace PythonAssetBuilder;
        using namespace AssetBuilderSDK;

        app->RegisterComponentDescriptor(PythonAssetBuilderSystemComponent::CreateDescriptor());
        systemEntity->template CreateComponent<PythonAssetBuilderSystemComponent>();
        systemEntity->Init();
        systemEntity->Activate();

        AssetBuilderPattern buildPattern;
        buildPattern.m_pattern = "*.mock";
        buildPattern.m_type = AssetBuilderPattern::Wildcard;

        AssetBuilderDesc builderDesc;
        builderDesc.m_busId = AZ::Uuid::CreateRandom();
        builderDesc.m_name = "Mock Builder";
        builderDesc.m_patterns.push_back(buildPattern);
        builderDesc.m_version = 0;

        AZ::Outcome<bool, AZStd::string> result;
        PythonAssetBuilderRequestBus::BroadcastResult(result, &PythonAssetBuilderRequestBus::Events::RegisterAssetBuilder, builderDesc);
        EXPECT_TRUE(result.IsSuccess());

        return builderDesc.m_busId;
    }
}
