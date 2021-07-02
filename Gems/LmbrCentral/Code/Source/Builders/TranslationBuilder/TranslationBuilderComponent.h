/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace TranslationBuilder
{
    //! Here is an example of a builder worker that actually performs the building of assets.
    //! In this example, we only register one, but you can have as many different builders in a single builder module as you want.
    class TranslationBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler // this will deliver you the "shut down!" message on another thread.
    {
    public:

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

        //////////////////////////////////////////////////////////////////////////
        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override; // if you get this you must fail all existing jobs and return.
        //////////////////////////////////////////////////////////////////////////

        static AZ::Uuid GetUUID();
    private:
        AZStd::string FindLReleaseTool() const;

    private:
        bool m_isShuttingDown = false;
    };

    //! Here's an example of the lifecycle Component you must implement.
    //! You must have at least one component to handle the lifecycle of your module.
    //! You could make this also be a builder if you also listen to the builder bus, and register itself as a builder
    //! But for the purposes of clarity, we will make it just be the lifecycle component in this example plugin
    class BuilderPluginComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BuilderPluginComponent, "{61560B47-39B8-43DD-ACBE-956ECFF9C414}")
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override; // reach out to the outside world and connect up to what you need to, register things, etc.
        void Deactivate() override; // unregister things, disconnect from the outside world
        //////////////////////////////////////////////////////////////////////////

    private:
        TranslationBuilderWorker m_builderWorker;
    };
}
