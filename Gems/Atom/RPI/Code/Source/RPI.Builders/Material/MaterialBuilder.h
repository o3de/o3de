/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <AzCore/JSON/document.h>

namespace AZ
{
    namespace RPI
    {
        class MaterialBuilder final
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            AZ_TYPE_INFO(MaterialBuilder, "{861C0937-7671-40DC-8E44-6D432ABB9932}");

            static const char* JobKey;

            MaterialBuilder() = default;
            ~MaterialBuilder();

            // Asset Builder Callback Functions
            void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
            void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

            // AssetBuilderSDK::AssetBuilderCommandBus interface
            void ShutDown() override;

            /// Register to builder and listen to builder command
            void RegisterBuilder();

        private:

            bool ShouldReportMaterialAssetWarningsAsErrors() const;
            AZStd::string GetBuilderSettingsFingerprint() const;
            
            bool m_isShuttingDown = false;
        };

    } // namespace RPI
} // namespace AZ
