/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace GraphCanvas
{

    class TranslationAssetWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:

        TranslationAssetWorker() = default;
        ~TranslationAssetWorker() override = default;

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
        TranslationAssetWorker(const TranslationAssetWorker&) = delete;
        bool m_isShuttingDown = false;

        AZStd::unique_ptr<AZ::Data::AssetHandler> m_assetHandler;

        // cached on first time query
        mutable AZStd::string m_fingerprintString;
    };
}
