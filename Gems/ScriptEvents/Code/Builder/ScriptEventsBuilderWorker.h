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

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Asset/AssetCommon.h>

namespace ScriptEventsEditor { class ScriptEventAssetHandler; }

namespace ScriptEventsBuilder
{
    class Worker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:

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
        
        void Activate();
        void Deactivate();

        static AZ::Uuid GetUUID();

    private:
        Worker(const Worker&) = delete;
        bool m_isShuttingDown = false;

        // cached on first time query
        mutable AZStd::string m_fingerprintString;
    };
}
