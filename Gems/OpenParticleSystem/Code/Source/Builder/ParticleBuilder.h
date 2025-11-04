/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>

namespace OpenParticle
{
    class ParticleBuilder final
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_TYPE_INFO(ParticleBuilder, "{da977692-0525-4cda-9462-fef10a50f94c}");

        static const char* jobKey;

        ParticleBuilder() = default;
        ~ParticleBuilder();

        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

        void ShutDown() override;

        void RegisterBuilder();

    private:
        bool m_isShuttingDown = false;
    };
} // namespace OpenParticle
