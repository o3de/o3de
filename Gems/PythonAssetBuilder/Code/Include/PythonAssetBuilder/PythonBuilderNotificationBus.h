/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace PythonAssetBuilder
{
    class PythonBuilderWorker;

    //! A notification bus for Python asset builder to hook into to operate asset building events
    class PythonBuilderNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::Uuid; //< JobId
        //////////////////////////////////////////////////////////////////////////

        //! creates jobs response using a request input
        virtual AssetBuilderSDK::CreateJobsResponse OnCreateJobsRequest(const AssetBuilderSDK::CreateJobsRequest& request) = 0;

        //! processes a source asset using a job request input
        virtual AssetBuilderSDK::ProcessJobResponse OnProcessJobRequest(const AssetBuilderSDK::ProcessJobRequest& request) = 0;

        //! signals when the entire asset building system is shutting down
        virtual void OnShutdown() = 0;

        //! signals the current job being processed should be canceled
        virtual void OnCancel() = 0;
    };
    using PythonBuilderNotificationBus = AZ::EBus<PythonBuilderNotifications>;
}

