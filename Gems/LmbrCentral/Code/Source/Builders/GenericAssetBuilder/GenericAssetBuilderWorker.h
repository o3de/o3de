/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <Builders/GenericAssetBuilder/GenericAssetBuilderCommon.h>

namespace LmbrCentral::GenericAssetBuilder
{
    // The generic Asset Builder Worker takes any registered generic asset types
    // and converts them into data in the cache by copying there.
    // It also extracts dependencies and registers them, if requested.
    // It can also convert the generic asset into a more compact binary format if requested.
    class GenericAssetBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler // this will deliver you the "shut down!" message on another thread.
    {
    public:
        AZ_RTTI(GenericAssetBuilderWorker, "{37C1FD99-D693-40D5-B0A4-71D542268B21}");

        GenericAssetBuilderWorker() = default;
        ~GenericAssetBuilderWorker() = default;

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        //////////////////////////////////////////////////////////////////////////
        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override; // if you get this you must fail all existing jobs and return.
        //////////////////////////////////////////////////////////////////////////

        void SetRegistrations(AZStd::vector<GenericAssetBuilderRegistration>&& registrations);
    private:
        bool m_isShuttingDown = false;
        AZStd::vector<GenericAssetBuilderRegistration> m_registrations;
    };
}
