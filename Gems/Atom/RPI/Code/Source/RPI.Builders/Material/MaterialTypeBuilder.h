/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <AzCore/JSON/document.h>

namespace AZ
{
    namespace RPI
    {
        class MaterialTypeBuilder final
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            AZ_TYPE_INFO(MaterialTypeBuilder, "{0D2D104F-9CC6-456E-88D9-24BCDA6C0465}");

            static const char* JobKey;

            MaterialTypeBuilder() = default;
            ~MaterialTypeBuilder();

            // Asset Builder Callback Functions
            void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
            void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

            // AssetBuilderSDK::AssetBuilderCommandBus interface
            void ShutDown() override;

            /// Register to builder and listen to builder command
            void RegisterBuilder();

        private:

            enum class MaterialTypeProductSubId : u32
            {
                MaterialTypeAsset = 0,
                AllPropertiesMaterialSourceFile = 1
            };

            bool ShouldOutputAllPropertiesMaterial() const;
            AZStd::string GetBuilderSettingsFingerprint() const;
            
            bool m_isShuttingDown = false;
        };

    } // namespace RPI
} // namespace AZ
