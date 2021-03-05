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

#include <AzCore/RTTI/RTTI.h>

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace CustomAssetExample
{
    //! Here is an example of a builder worker that actually performs the building of assets.
    //! In this example, we only register one, but you can have as many different builders in a single builder module as you want.
    class ExampleBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler // this will deliver you the "shut down!" message on another thread.
    {
    public:
        AZ_RTTI(ExampleBuilderWorker, "{C163F950-BF25-4D60-90D7-8E181E25A9EA}");

        ExampleBuilderWorker() = default;
        ~ExampleBuilderWorker() = default;

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        //////////////////////////////////////////////////////////////////////////
        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override; // if you get this you must fail all existing jobs and return.
        //////////////////////////////////////////////////////////////////////////

    private:
        bool m_isShuttingDown = false;
    };
}
